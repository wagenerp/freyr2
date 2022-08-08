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

#include <freertos/FreeRTOS.h>

#include <freertos/task.h>

#include "alpha4/common/logger.hpp"
#include "core/animation.hpp"
#include "core/animation_api.h"
#include "core/egress.hpp"
#include "core/egress_api.h"
#include "core/frame.hpp"
#include "core/frame_api.h"
#include "core/basemodule.hpp"
#include "core/module.hpp"
#include "core/module_api.h"
#include "esp_event.h"
#include "main/static-module-registry.hpp"
#include <chrono>
#include <esp_err.h>
#include <esp_log.h>
#include <mutex>
#include <nvs_flash.h>
#include <vector>

struct Drummer {
public:
	using duration   = std::chrono::duration<double>;
	using clock      = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock, duration>;

protected:
	time_point _tNext;
	duration   _interval;

public:
	Drummer(const duration &interval) : _interval(interval) {}
	size_t sync() {
		auto t = clock::now();
		if (t < _tNext) {
			std::this_thread::sleep_until(_tNext);
			t = clock::now();
		}
		size_t advances = 0;
		while (_tNext < t) {
			_tNext += _interval;
			advances++;
		}
		return advances;
	}
};

struct FPSCounter {
	using duration   = AnimatorPool::duration;
	using clock      = AnimatorPool::clock;
	using time_point = AnimatorPool::time_point;

	time_point t0;
	size_t     n = 0;

	double estimate = 0;

	FPSCounter() : t0(std::chrono::time_point_cast<duration>(clock::now())) {}

	void iterate() {
		n++;
		if ((n % 33) == 0) {
			auto t1 = std::chrono::time_point_cast<duration>(clock::now());

			estimate = 33.0 / (t1 - t0).count();
			t0       = t1;
		}
	}
};

static std::mutex        _MutexCommands;
std::vector<std::string> _QueuedCommands;
extern "C" {

void enqueue_command(const char *cmd, unsigned size) {
	std::unique_lock<std::mutex> lock(_MutexCommands);
	_QueuedCommands.emplace_back(cmd, size);
}

void device_init_freyr();

// led_t *frame_raw_egress() { return frame.data(); }
void egress_esp32_rmt_init(egressno_t, const char *argstr, void **);
void egress_esp32_rmt_deinit(void *);
void egress_esp32_rmt_flush(led_i_t firstLED, led_i_t count, void *);
void mod_esp32_control_init(modno_t modno, const char *, void **);
void mod_esp32_control_deinit(modno_t, void *);
void mod_esp32_control_flush(modno_t, void *);
}
static void task_main(void *arg) {

	AnimatorPool::Get().setup(1);
	ModuleRegistry::Install();

	BaseModule::DefineSymbol(
		"egress_esp32_rmt", "init", (void *)egress_esp32_rmt_init);
	BaseModule::DefineSymbol(
		"egress_esp32_rmt", "deinit", (void *)egress_esp32_rmt_deinit);
	BaseModule::DefineSymbol(
		"egress_esp32_rmt", "flush", (void *)egress_esp32_rmt_flush);

	BaseModule::DefineSymbol(
		"mod_esp32_control", "init", (void *)mod_esp32_control_init);
	BaseModule::DefineSymbol(
		"mod_esp32_control", "deinit", (void *)mod_esp32_control_deinit);
	BaseModule::DefineSymbol(
		"mod_esp32_control", "flush", (void *)mod_esp32_control_flush);

	module_instantiate("bootstrap", nullptr, "");
	module_instantiate("esp32_control", nullptr, "");
	module_instantiate("display", nullptr, "");
	module_instantiate("grouping", nullptr, "");
	// module_instantiate("streams", nullptr, "");

	device_init_freyr();

	{
		using namespace std::chrono_literals;
		Drummer    drummer(16.666ms);
		FPSCounter fpsCounter;
		auto &     animPool = AnimatorPool::Get();

		Module::Flush();
		animPool.flush();
		Frame::FlushAnim();

		unsigned iFrame = 0;
		while (1) {
			iFrame++;
			Frame::FlushEgress();
			EgressInstance::Flush();
			{
				std::unique_lock<std::mutex> lock(_MutexCommands);
				for (const auto &cmd : _QueuedCommands)
					command_run(cmd.c_str(), "");
				_QueuedCommands.clear();
			}
			Module::Flush();
			animPool.flush();
			if ((iFrame % 30) == 0) { printf("fps: %12.2f\n", fpsCounter.estimate); }
			Frame::FlushAnim();
			drummer.sync();
			fpsCounter.iterate();

			animPool.renderFrame(0);
		}
	}
}

void freyr_init() {
	xTaskCreatePinnedToCore(&task_main, "task_main", 8192, NULL, 1, NULL, 0);
}