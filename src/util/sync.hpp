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

#ifndef CORE_SYNC_HPP
#define CORE_SYNC_HPP

#include "core/animation.hpp"
#include <condition_variable>
#include <mutex>
#include <unordered_map>

struct AnimBarrier {
public:
	enum struct State { Ready, Animating };
	enum struct AnimatorState { Animating = 0, Ready, Pending };

	class CollectorGuard {
	protected:
		AnimBarrier &_barrier;
		bool         _active = true;

	public:
		CollectorGuard(AnimBarrier &barrier) : _barrier(barrier) {}
		CollectorGuard(const CollectorGuard &) = delete;
		CollectorGuard(CollectorGuard &&g) : _barrier(g._barrier) {
			g._active = false;
		}

		~CollectorGuard() {
			std::unique_lock<std::mutex> lock(_barrier._mutex);
			_barrier._activeCollectors--;
			if (_barrier._readyNeedsNotifying()) _barrier._cond_ready.notify_all();
		}
	};

protected:
	std::mutex              _mutex;
	std::condition_variable _cond_activate;
	std::condition_variable _cond_ready;

	size_t _animatorCount = 1;

	size_t _activeCollectors = 0;

	bool _locked = false;

	std::unordered_map<std::thread::id, AnimatorState> _animatorStates;

	size_t _waitingAnimators() const {
		size_t res = 0;
		for (auto it : _animatorStates)
			if (it.second == AnimatorState::Ready) res++;
		return res;
	}
	bool _readyNeedsNotifying() const {
		return _locked && (_activeCollectors < 1)
					 && (_waitingAnimators() >= _animatorCount);
	}

	AnimBarrier() {}

public:
	static AnimBarrier &Get();

	void           waitForFrame();
	CollectorGuard lockCollector();
	void           waitForAnimators(size_t animatorCount);
	void           startFrame();
};

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

#endif