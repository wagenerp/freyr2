/* Copyright 2022 Peter Wagener <mail@peterwagener.net>

This file is part of Freyr2.

Freyr2 is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Freyr2 is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Freyr2. If not, see <https://www.gnu.org/licenses/>.
*/

#include "alpha4/common/guard.hpp"
#include "alpha4/common/linescanner.hpp"
#include "alpha4/common/logger.hpp"
#include "core/module_api.h"
#include "util/module.hpp"
#include <cstring>
#include <memory>
#include <mosquitto.h>
#include <mosquittopp.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

extern "C" {
#include "unicorn/idl.h"
}
class MQTTInterface : public mosqpp::mosquittopp {
public:
	struct CommandHandler {
		std::string    command;
		std::string    source;
		std::string    topic_stdout;
		std::string    topic_stderr;
		MQTTInterface &iface;

		CommandHandler(
			std::string    command,
			std::string    source,
			std::string    topic_stdout,
			std::string    topic_stderr,
			MQTTInterface &iface) :
			command(command),
			source(source),
			topic_stdout(topic_stdout),
			topic_stderr(topic_stderr),
			iface(iface) {}

		static void Respond(
			response_type_t type,
			const char *,
			const char *response,
			void *      userdata) {
			CommandHandler *crh = static_cast<CommandHandler *>(userdata);

			const std::string &topic =
				((type == E) | (type == W)) ? crh->topic_stderr : crh->topic_stdout;
			crh->iface.publish(nullptr, topic.c_str(), strlen(response), response);
		}

		void run() {
			command_response_push(CommandHandler::Respond, this);
			alp::Guard g(command_response_pop);

			command_run(command.c_str(), source.c_str());
		}
	};

protected:
	std::string _host = "127.0.0.1";
	int         _port = 1883;

	std::unique_ptr<uidl_t, void (*)(uidl_t *)> _idl{nullptr, nullptr};

	std::string _device_ident;
	std::string _topic_idl;
	std::string _topic_main;

	std::thread _thread;

	std::vector<std::unique_ptr<CommandHandler>> _pendingCommands;
	std::mutex                                   _mutexCommands;
	std::mutex                                   _mutexIDL;

public:
	struct KWArgs {
		int  port = 1883;
		bool flat = false;
	};
	MQTTInterface(
		const std::string host, const std::string &prefix, const KWArgs &kwargs) :
		_host(host), _port(kwargs.port) {
		{
			std::stringstream ss;
			ss << "freyr2:" << prefix << ":" << getpid();
			reinitialise(ss.str().c_str(), true);
		}

		_idl = std::unique_ptr<uidl_t, void (*)(uidl_t *)>(uidl_new(), uidl_free);
		_device_ident = prefix;
		_topic_main   = prefix;
		_topic_idl    = "/unicorn/idl/" + prefix;
		uidl_set_completion_metadata(
			_idl.get(),
			UIDL_COMPL_ADHOC | (kwargs.flat ? UIDL_COMPL_FLAT : 0),
			(prefix + "/stdout").c_str(),
			(prefix + "/stderr").c_str());
		{
			auto node = commands_describe();
			uidl_set_completion(_idl.get(), node);
		}

		{
			std::string msg = _device_ident + ".disconnect";
			will_set("/unicorn", msg.size(), msg.c_str());
		}

		LOG(I) << "connecting to mqtt server " << host << ":" << kwargs.port
					 << " under device name " << prefix << alp::over;
		_thread = std::thread(std::bind(&MQTTInterface::loop_forever, this, -1, 1));
		connect();
	}

	virtual ~MQTTInterface() {
		disconnect();
		_thread.join();
	}

	inline void publish_idl(bool update) {
		auto sb = sbuilder_new();

		std::unique_lock<std::mutex> lock(_mutexIDL);

		if (update) {
			auto node = commands_describe();
			uidl_set_completion(_idl.get(), node);
		}

		uidl_to_json(_idl.get(), sb);
		alp::Guard g([sb] { sbuilder_free(sb); });

		sbuilder_str(sb);
		publish(
			NULL, _topic_idl.c_str(), sbuilder_size(sb), sbuilder_str(sb), 2, true);
	}

	using mosqpp::mosquittopp::connect;
	int connect(int keepalive = 60) {
		return connect(_host.c_str(), _port, keepalive);
	}

	virtual void on_connect(int rc) override {
		LOG(I) << "mqtt connected. rc:" << rc << alp::over;

		const auto mysubscribe = [&](const std::string &topic) {
			int r = subscribe(nullptr, topic.c_str());
			if (r != MOSQ_ERR_SUCCESS) {
				LOG(E) << "mqtt subscribe failed on " << topic << ": "
							 << mosqpp::strerror(r) << alp::over;
			}
		};

		{
			std::string msg = _device_ident + ".connect";
			publish(nullptr, "/unicorn", msg.size(), msg.c_str());
		}

		mysubscribe(_topic_main);
		mysubscribe(_topic_main + "/+");

		publish_idl(false);
	}

	virtual void on_message(const struct mosquitto_message *msg) override {
		const std::string topic(msg->topic);
		if (topic == _topic_main) {
			std::unique_lock<std::mutex> lock(_mutexCommands);
			_pendingCommands.emplace_back(std::make_unique<CommandHandler>(
				std::string((char *)msg->payload),
				topic,
				_topic_main + "/stdout",
				_topic_main + "/stderr",
				*this));

		} else if (topic.substr(0, _topic_main.size() + 1) == (_topic_main + "/")) {
			const std::string channel = topic.substr(_topic_main.size() + 1);

			std::unique_lock<std::mutex> lock(_mutexCommands);
			_pendingCommands.emplace_back(std::make_unique<CommandHandler>(
				std::string((char *)msg->payload),
				topic,
				_topic_main + "/stdout/" + channel,
				_topic_main + "/stderr/" + channel,
				*this));
		}
	}

	void flushCommands() {
		std::vector<std::unique_ptr<CommandHandler>> work;
		{
			std::unique_lock<std::mutex> lock(_mutexCommands);
			_pendingCommands.swap(work);
		}

		for (auto &cmd : work) {
			cmd->run();
		}
	}
};

extern "C" {

modno_t SingletonInstance = INVALID_MODULE;

static void _hook_idlChanged(hook_t, modno_t, void *ud) {
	MQTTInterface *iface = (MQTTInterface *)ud;

	iface->publish_idl(true);
}

void init(modno_t modno, const char *argstr, void **pud) {
	std::string host   = "127.0.0.1";
	std::string prefix = "freyr";

	MQTTInterface::KWArgs kwargs;

	{
		alp::LineScanner ln(argstr);
		std::string      cmd;
		while (ln.get(cmd)) {
			if (cmd == "host") {
				if (!ln.get(host)) { RESPOND(E) << "missing host name" << alp::over; }
			} else if (cmd == "prefix") {
				if (!ln.get(prefix)) {
					RESPOND(E) << "missing prefix topic" << alp::over;
				}
			} else if (cmd == "port") {
				if (!ln.get(kwargs.port)) {
					RESPOND(E) << "missing port number" << alp::over;
				}
			} else if (cmd == "flat") {
				kwargs.flat = true;
			}
		}
	}
	*pud = new MQTTInterface(host, prefix, kwargs);
	module_hook(modno, hook_resolve("idlChanged"), _hook_idlChanged);
}

void deinit(modno_t, void *ud) { delete (MQTTInterface *)ud; }

void flush(modno_t, void *ud) {
	MQTTInterface *iface = (MQTTInterface *)ud;
	iface->flushCommands();
}

uidl_node_t *describe() {
	return uidl_repeat(
		0,
		uidl_keyword(
			0,
			4,

			uidl_pair("host", uidl_string(0, 0)),
			uidl_pair("prefix", uidl_string(0, 0)),
			uidl_pair("port", uidl_integer(0, UIDL_LIMIT_RANGE, 0, 0xffff)),
			uidl_pair("flat", 0)

				),
		0);
}
}