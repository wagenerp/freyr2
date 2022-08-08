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

#ifndef UTIL_LOCK_HPP
#define UTIL_LOCK_HPP
#include <condition_variable>
#include <mutex>
#include <system_error>

namespace util {

struct StepLock {
public:
	struct TokenGuard {
	protected:
		StepLock &_lock;

	public:
		TokenGuard(StepLock &lock) : _lock(lock) { _lock._grab(); }
		~TokenGuard() { _lock._drop(); }
	};

	struct ExclusionGuard {
	protected:
		StepLock &_lock;

	public:
		ExclusionGuard(StepLock &lock) : _lock(lock) { _lock._exclude(); }
		~ExclusionGuard() { _lock._release(); }
	};

protected:
	std::mutex                   _mutex;
	std::condition_variable      _cond_tokens;
	std::condition_variable      _cond_exclusion;
	std::unique_lock<std::mutex> _tokenLock;

	unsigned _usageCount = 0;
	bool     _exclusion  = false;

	void _grab() {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_exclusion) {
			_cond_exclusion.wait(lock);
		}
		_usageCount++;
	}
	void _drop() {
		bool signal = false;
		{
			std::unique_lock<std::mutex> lock(_mutex);
			_usageCount--;
			if (_usageCount < 1) signal = true;
		}
		if (signal) _cond_tokens.notify_all();
	}
	void _exclude() {
		std::unique_lock<std::mutex> lock(_mutex);
		while (_usageCount > 0) {
			_cond_tokens.wait(lock);
		}
		_exclusion = true;
	}
	void _release() {
		bool signal = false;
		{
			std::unique_lock<std::mutex> lock(_mutex);
			if (_exclusion) {
				signal     = true;
				_exclusion = false;
			}
		}
		if (signal) _cond_exclusion.notify_all();
	}

public:
	StepLock() : _tokenLock(_mutex, std::defer_lock) {}

	TokenGuard acquireToken() {
		try {
			_tokenLock.lock();
		} catch (const std::system_error &e) {
			if (e.code() != std::errc::resource_deadlock_would_occur) throw;
		}
		return TokenGuard(*this);
	}

	std::unique_lock<std::mutex> acquireExclusive() {
		while (_usageCount > 0) {}

		if (_usageCount < 1) {
			try {
				_tokenLock.unlock();
			} catch (const std::system_error &e) {
				if (e.code() != std::errc::operation_not_permitted) throw;
			}
		}
		return std::unique_lock<std::mutex>(_mutex);
	}

};

} // namespace util

#endif