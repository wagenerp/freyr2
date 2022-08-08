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

#include "sync.hpp"

AnimBarrier &AnimBarrier::Get() {
	static AnimBarrier barrier;
	return barrier;
}

void AnimBarrier::waitForFrame() {
	const auto id = std::this_thread::get_id();

	std::unique_lock<std::mutex> lock(_mutex);
	_animatorStates[id] = AnimatorState::Ready;
	if (_readyNeedsNotifying()) _cond_ready.notify_all();
	while (_animatorStates[id] != AnimatorState::Pending) {
		_cond_activate.wait(lock);
	}
	_animatorStates[id] = AnimatorState::Animating;
}

AnimBarrier::CollectorGuard AnimBarrier::lockCollector() {
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while (_locked) {
			_cond_activate.wait(lock);
		}
		_activeCollectors++;
	}
	return CollectorGuard(*this);
}

void AnimBarrier::waitForAnimators(size_t animatorCount) {
	std::unique_lock<std::mutex> lock(_mutex);
	_animatorCount = animatorCount;
	_locked        = true;
	while (_waitingAnimators() < _animatorCount) {
		_cond_ready.wait(lock);
	}
}

void AnimBarrier::startFrame() {
	{
		std::unique_lock<std::mutex> lock(_mutex);
		while (_waitingAnimators() < _animatorCount) {
			_cond_ready.wait(lock);
		}
		for (auto &it : _animatorStates) {
			it.second = AnimatorState::Pending;
		}
		_locked = false;
	}
	_cond_activate.notify_all();
}