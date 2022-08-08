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

#include "alpha4/common/cli.hpp"
#include "alpha4/common/guard.hpp"
#include "alpha4/common/linescanner.hpp"
#include "alpha4/common/logger.hpp"
#include "core/animation.hpp"
#include "core/animation_api.h"
#include "core/basemodule.hpp"
#include "core/egress.hpp"
#include "core/egress_api.h"
#include "core/frame.hpp"
#include "core/frame_api.h"
#include "core/ledset.hpp"
#include "core/module.hpp"
#include "core/module_api.h"
#include "modules/coordinates_api.h"
#include "util/sync.hpp"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <regex>
#include <signal.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#ifndef OPT_DYNAMIC
#include "static-module-registry.hpp"
#else
#include <elf.h>
#include <link.h>
#endif

static size_t _ThreadCount = 0;
static double _FPSTarget   = 60.0;

static void handle_sigint(int) { main_stop(); }

static std::atomic_bool _Running = true;
static void             _AnimThread(size_t iAnimator) {
  auto &barrier  = AnimBarrier::Get();
  auto &animPool = AnimatorPool::Get();
  while (_Running) {
    barrier.waitForFrame();
    animPool.renderFrame(iAnimator);
  }
}

void orchestrate() {
	using namespace std::chrono_literals;
	Drummer    drummer(1s / _FPSTarget);
	FPSCounter fpsCounter;

	auto & barrier  = AnimBarrier::Get();
	auto & animPool = AnimatorPool::Get();
	size_t iframe   = 0;

	Module::Flush();
	animPool.flush();
	Frame::FlushAnim();

	const auto hook_applyFilter = hook_resolve("applyFilter");
	main_start();
	{
		signal(SIGINT, handle_sigint);
		alp::Guard guard1([] { signal(SIGINT, nullptr); });
		if (_ThreadCount < 1) {
			while (main_running()) {
				iframe++;
				Frame::FlushEgress();
				hook_trigger(hook_applyFilter);
				EgressInstance::Flush();
				Module::Flush();
				animPool.flush();

				Frame::FlushAnim();
				drummer.sync();
				fpsCounter.iterate();

				animPool.renderFrame(0);
			}
		} else {
			_Running = true;
			std::vector<std::thread> threads;

			for (size_t i = 0; i < _ThreadCount; i++) {
				threads.emplace_back(_AnimThread, i);
			}

			while (main_running()) {
				iframe++;

				barrier.waitForAnimators(_ThreadCount);
				Frame::FlushEgress();
				hook_trigger(hook_applyFilter);
				EgressInstance::Flush();
				Module::Flush();
				animPool.flush();

				Frame::FlushAnim();
				drummer.sync();
				fpsCounter.iterate();

				barrier.startFrame();
			}

			barrier.waitForAnimators(_ThreadCount);

			_Running = false;
			barrier.startFrame();

			for (auto &thread : threads) {
				thread.join();
			}
		}
	}
}

std::regex expr_command_filter("[ \\t]*(#.*)?(.*?)[ \\t]*");

alp::CLI cli{
	.switches = {
		{'h',
		 "help",
		 "print this help text and exit normally",
		 []() {
			 cli.printHelp(std::cout);
			 exit(0);
		 }},
		{'l',
		 "load",
		 "open a config file and process it line-by-line",
		 [](const std::filesystem::path &p) {
			 if (
				 !std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
				 alp::thrower<alp::CLEX>()
					 << "config file " << p << " does not exist" << alp::over;
			 }

			 LOG(I) << "processing config file " << p << alp::over;

			 std::ifstream         f(p);
			 std::string           line;
			 std::smatch           sm;
			 size_t                lidx = 0;
			 std::string           sourcePointer;
			 alp::MultilineScanner mls;
			 mls.callback = [&](const std::string &code) {
				 sourcePointer = p.string() + ":" + std::to_string(lidx);

				 command_run(code.c_str(), sourcePointer.c_str());
			 };

			 mls.delimiter = ' ';

			 while (std::getline(f, line)) {
				 lidx++;

				 mls.processLine(line);
			 }

			 mls.flush();
		 }},
		{'c',
		 "command",
		 "process a single command as-is",
		 [](const std::string &v) { command_run(v.c_str(), "cmdline"); }},

		{'t',
		 "thread-count",
		 "set the number of animation threads to run in parallel, 0 for in-loop "
		 "animation",
		 [](const size_t &n) { _ThreadCount = n; }},

		{'r',
		 "frame-rate",
		 "set the target frame rate to achieve, default: 60 Hz",
		 [](double fps) { _FPSTarget = fps; }},

	}};

int main(int argn, char **argv) {
#ifndef OPT_DYNAMIC
	ModuleRegistry::Install();
#else

	BaseModule::ScanDirectory(std::filesystem::current_path() / "modules");
	{
		const ElfW(Dyn) *rpath = nullptr;
		const char *strtab     = nullptr;
		for (const auto *dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn) {
			if (dyn->d_tag == DT_RPATH) {
				rpath = dyn;
			} else if (dyn->d_tag == DT_STRTAB) {
				strtab = (const char *)dyn->d_un.d_val;
			}
		}

		if ((nullptr != strtab) & (nullptr != rpath)) {
			BaseModule::ScanDirectory(strtab + rpath->d_un.d_val);
		}
	}
#endif

	AnimatorPool::Get().setup(1);
	module_instantiate("bootstrap", nullptr, "");

	std::srand(time(0));

	if (cli.process(argn, argv)) { orchestrate(); }

	module_cleanup();
	anim_cleanup();
	egress_cleanup();
	AnimatorPool::Get().clear();
	AnimatorPool::Get().setup(0);

	return 0;
}