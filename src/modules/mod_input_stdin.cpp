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

#include "alpha4/common/linescanner.hpp"
#include "core/basemodule_api.h"
#include "core/module_api.h"
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <thread>
#include <unistd.h>
#include <vector>

extern "C" {
modno_t SingletonInstance = INVALID_MODULE;

static std::vector<std::string> _PendingLines;
static std::mutex               _Mutex;
static std::thread              _Thread;
static int                      _FDCloseEvent = -1;

static void _threadMain() {
	std::string line;

	alp::MultilineScanner mls;
	mls.callback = [&](const std::string &code) {
		std::unique_lock<std::mutex> lock(_Mutex);
		_PendingLines.emplace_back(code);
	};
	mls.delimiter = ' ';

	fd_set fds_master;
	FD_ZERO(&fds_master);
	FD_SET(STDIN_FILENO, &fds_master);
	FD_SET(_FDCloseEvent, &fds_master);

	const int fdmax = std::max(STDIN_FILENO, _FDCloseEvent) + 1;

	try {
		while (1) {
			fd_set    fds = fds_master;
			const int res = select(fdmax, &fds, nullptr, nullptr, nullptr);

			if (FD_ISSET(_FDCloseEvent, &fds)) { break; }

			if (
				(res > 0) && FD_ISSET(STDIN_FILENO, &fds)
				&& std::getline(std::cin, line)) {
				mls.processLine(line);
			}
		}
		mls.flush();
	} catch (...) {}
}

void init(modno_t, const char *, void **) {
	_FDCloseEvent = eventfd(0, 0);
	_Thread       = std::thread(_threadMain);
}

void deinit(modno_t, void *) {
	{
		uint64_t buf = 1;
		write(_FDCloseEvent, &buf, sizeof(uint64_t));
	}

	_Thread.join();
	close(_FDCloseEvent);

	_FDCloseEvent = -1;
	_PendingLines.clear();
}

void flush(modno_t, void *) {
	std::vector<std::string> lines;
	{
		std::unique_lock<std::mutex> lock(_Mutex);
		if (_PendingLines.empty()) return;
		_PendingLines.swap(lines);
	}

	for (const auto &ln : lines) {
		command_run(ln.c_str(), "stdin");
	}
}
}
