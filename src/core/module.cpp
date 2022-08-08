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

#include "module.hpp"

#include "alpha4/common/error.hpp"
#include "alpha4/common/linescanner.hpp"
#include "alpha4/common/logger.hpp"
#include "core/basemodule_api.h"
#include "module_api.h"
#include "types/stringlist.h"
#include "util/module.hpp"
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

static std::unordered_map<std::string, std::shared_ptr<Module>> _ModuleNames;
static std::unordered_map<modno_t, std::shared_ptr<Module>>     _ModuleMap;
static std::unordered_map<std::string, std::shared_ptr<Module>> _Singletons;
static modno_t _ModuleCount = 0;
static modno_t AllocateModuleNumber() { return ++_ModuleCount; }

struct CommandRecord {
	modno_t            modno;
	command_func_f     func;
	command_describe_f describe;
};

static std::recursive_mutex                           _MutexCommands;
static std::unordered_map<std::string, CommandRecord> _CommandRegistry;

struct CommandResponder {
	command_response_f re_func;
	void *             re_userdata;
};

static std::vector<CommandResponder> _CommandResponders;

struct Hooker {
	modno_t               modno;
	hook_func_f           func;
	std::weak_ptr<Module> module;
};
static std::vector<std::unique_ptr<std::vector<Hooker>>> _Hookers;
static std::unordered_map<std::string, hook_t>           _HookMap;

void Module::Flush() {
	for (auto it : _ModuleMap) {
		auto mod = it.second;
		if (mod->_flush) mod->_flush(it.first, mod->_userdata);
	}
}

Module::Module(
	const std::string &        ident,
	basemodno_t                basemodno,
	std::optional<std::string> instanceName) :
	_modno(AllocateModuleNumber()),
	_basemodno(basemodno),
	_ident(ident),
	_instanceName(instanceName) {
	_init =
		reinterpret_cast<module_init_f>(basemodule_resolve(_basemodno, "init"));
	_deinit =
		reinterpret_cast<module_deinit_f>(basemodule_resolve(_basemodno, "deinit"));
	_flush =
		reinterpret_cast<module_flush_f>(basemodule_resolve(_basemodno, "flush"));
	_singletonInstance = reinterpret_cast<modno_t *>(
		basemodule_resolve(_basemodno, "SingletonInstance"));
	if (_singletonInstance) *_singletonInstance = _modno;

	basemodule_grab(_basemodno);
}

Module::~Module() {
	if (_deinit) { _deinit(_modno, _userdata); }

	{
		std::lock_guard<std::recursive_mutex> lock(_MutexCommands);

		for (auto it = _CommandRegistry.begin(); it != _CommandRegistry.end();) {
			if (it->second.modno == _modno) {
				it = _CommandRegistry.erase(it);
			} else {
				++it;
			}
		}
	}
	basemodule_drop(_basemodno);
}

void Module::init(const std::string &argstring) {
	if (_init) _init(_modno, argstring.c_str(), &_userdata);
}

extern "C" {

void module_status() {
	auto msg = std::move(RESPOND(I) << "modules: " << _ModuleMap.size() << "\n");
	for (auto it : _ModuleMap) {
		msg << "  #" << it.first << " (" << it.second->ident();
		if (it.second->instanceName()) { msg << " " << *it.second->instanceName(); }
		msg << ")";
		if (it.second->singletonInstance()) msg << " (singleton)";
		msg << "\n";
	}

	msg << alp::over;
}

stringlist_t *module_list_get() {
	auto res = stringlist_new();

	res->count = _ModuleNames.size();
	if (res->count > 0) {
		res->modules = (char **)malloc(res->count * sizeof(char *));
		char **p     = res->modules;
		for (auto it : _ModuleNames) {
			*p++ = strdup(it.first.c_str());
		}
	} else {
		res->modules = nullptr;
	}
	return res;
}

modno_t module_instantiate(
	const char *ident, const char *instanceName, const char *argstring) {
	if (auto it = _Singletons.find(ident); it != _Singletons.end()) {
		return it->second->modno();
	}

	if (instanceName && (0 == *instanceName)) instanceName = nullptr;

	if (instanceName) {
		if (auto it = _ModuleNames.find(instanceName); it != _ModuleNames.end()) {
			return it->second->modno();
		}
	}

	std::string moduleName = "mod_" + std::string(ident);
	basemodno_t bmod       = basemodule_init(moduleName.c_str());
	if (INVALID_BASEMOD == bmod) {
		RESPOND(W) << "unable to find module " << ident << alp::over;
		return INVALID_MODULE;
	}

	{
		std::optional<std::string> instanceNameOpt;
		if (instanceName) instanceNameOpt = instanceName;
		std::shared_ptr<Module> res =
			std::make_shared<Module>(ident, bmod, instanceNameOpt);
		LOG(I) << "instantiating module #" << res->modno() << " (" << ident
					 << ") with argstring: " << argstring << alp::over;
		if (res->singletonInstance()) { _Singletons[ident] = res; }
		_ModuleMap[res->modno()] = res;
		if (instanceName) { _ModuleNames[instanceName] = res; }
		res->init(argstring);
		return res->modno();
	}
}

modno_t module_find(const char *instanceName) {
	if (auto it = _ModuleNames.find(instanceName); it != _ModuleNames.end()) {
		return it->second->modno();
	}

	return INVALID_MODULE;
}

void module_remove(modno_t modno) {
	if (auto it = _ModuleMap.find(modno); it != _ModuleMap.end()) {
		auto mod = it->second;
		if (mod->singletonInstance()) { _Singletons.erase(mod->ident()); }
		if (mod->instanceName()) { _ModuleNames.erase(*mod->instanceName()); }
		_ModuleMap.erase(it);
		for (auto &hook : _Hookers) {
			std::erase_if(
				*hook, [modno](const Hooker &hooker) { return hooker.modno == modno; });
		}
	} else {
		RESPOND(W) << "attempted to remove non-existing module #" << modno
							 << alp::over;
	}
}

void module_cleanup() {
	_ModuleNames.clear();
	_Singletons.clear();
	_ModuleMap.clear();
	_CommandRegistry.clear();
	for (auto &hooker : _Hookers) {
		hooker->clear();
	}
	_ModuleCount = 0;
}

void module_register_command(
	modno_t            modno,
	const char *       ident,
	command_func_f     func,
	command_describe_f describe) {
	std::lock_guard<std::recursive_mutex> lock(_MutexCommands);
	_CommandRegistry[ident] = {modno, func, describe};
}

int command_run(const char *cmdline, const char *source) {
	std::string cmd, argstr;
	{
		alp::LineScanner ln(cmdline);
		std::string_view cmd_v;
		std::string_view argstr_v;
		if (!ln.getLnFirstString(cmd_v)) return 0;
		ln.getLnRemainder(argstr_v);
		cmd    = cmd_v;
		argstr = argstr_v;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(_MutexCommands);

		if (auto it = _CommandRegistry.find(cmd); it != _CommandRegistry.end()) {
			auto module = _ModuleMap[it->second.modno];
			if (!module) return 0;
			it->second.func(it->second.modno, argstr.c_str(), module->userdata());
			return 1;
		} else {
			auto res = std::move(RESPOND(E) << "unknown command: '" << cmd << "'");
			if ((0 != source) && (0 != *source)) { res << " in " << source; }
			res << alp::over;
		}
	}
	return 0;
}

void command_respond_default(
	response_type_t type, const char *source, const char *response, void *) {
	switch (type) {
		case response_type_t::E:
			if (auto logger = ::alp::Logger::GetPointer(); logger)
				(*logger).startMessage<alp::Logger::Type::E>()
					<< ::alp::Logger::SourcePointer{source, -1} << response << alp::over;
			break;
		case response_type_t::W:
			if (auto logger = ::alp::Logger::GetPointer(); logger)
				(*logger).startMessage<alp::Logger::Type::W>()
					<< ::alp::Logger::SourcePointer{source, -1} << response << alp::over;
			break;
		case response_type_t::I:
			if (auto logger = ::alp::Logger::GetPointer(); logger)
				(*logger).startMessage<alp::Logger::Type::I>()
					<< ::alp::Logger::SourcePointer{source, -1} << response << alp::over;
			break;
		case response_type_t::D:
			if (auto logger = ::alp::Logger::GetPointer(); logger)
				(*logger).startMessage<alp::Logger::Type::D>()
					<< ::alp::Logger::SourcePointer{source, -1} << response << alp::over;
			break;
		case response_type_t::T:
			if (auto logger = ::alp::Logger::GetPointer(); logger)
				(*logger).startMessage<alp::Logger::Type::T>()
					<< ::alp::Logger::SourcePointer{source, -1} << response << alp::over;
			break;
		default: break;
	}
}

void command_response_push(command_response_f re_func, void *re_userdata) {
	_CommandResponders.emplace_back(CommandResponder{re_func, re_userdata});
}
void command_response_pop() {
	if (_CommandResponders.empty()) return;
	_CommandResponders.pop_back();
}

void command_respond(
	response_type_t type, const char *source, const char *response) {
	if (_CommandResponders.empty()) {
		command_respond_default(type, source, response, nullptr);
	} else {
		auto &re = _CommandResponders.back();
		re.re_func(type, source, response, re.re_userdata);
	}
}

uidl_node_t *commands_describe() {
	std::lock_guard<std::recursive_mutex> lock(_MutexCommands);

	uidl_node_t *res = uidl_keyword(nullptr, 0);

	for (auto it : _CommandRegistry) {
		auto itMod = _ModuleMap.find((it.second.modno));
		if (itMod == _ModuleMap.end()) continue;

		uidl_node_t *child = nullptr;
		if (it.second.describe) {
			child = it.second.describe(itMod->second->userdata());
		}
		uidl_keyword_set(res, it.first.c_str(), child);
	}

	return res;
}

hook_t hook_resolve(const char *ident) {
	if (auto it = _HookMap.find(ident); it != _HookMap.end()) {
		return it->second;
	}
	const hook_t res = _Hookers.size();
	_HookMap[ident]  = res;
	_Hookers.emplace_back(std::make_unique<std::vector<Hooker>>());
	return res;
}

void module_hook(modno_t modno, hook_t hook, hook_func_f func) {
	if (hook >= _Hookers.size()) return;
	auto itModule = _ModuleMap.find(modno);
	if (itModule == _ModuleMap.end()) return;

	_Hookers[hook]->emplace_back(Hooker{modno, func, itModule->second});
}

void hook_trigger(hook_t hook) {
	if (hook >= _Hookers.size()) return;
	for (const auto &hooker : *_Hookers[hook]) {
		auto module = hooker.module.lock();
		if (!module) continue;
		hooker.func(hook, hooker.modno, module->userdata());
	}
}

extern "C" {
static int _MainRunning = 0;
void       main_start() { _MainRunning = 1; }
void       main_stop() { _MainRunning = 0; }
int        main_running() { return _MainRunning; }
}
}