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


#include "core/animation_api.h"
class Orchestrator {
public:
	using duration   = std::chrono::duration<frame_time_t>;
	using clock      = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock, duration>;

	struct SubAnimation {
		std::shared_ptr<Animation> animation;
		std::vector<led_i_t>       leds;
	};

	struct Animator {
		Orchestrator &               owner;
		std::shared_ptr<AnimBarrier> barrier;
		std::vector<SubAnimation>    animations;
		std::vector<SubAnimation>    nextAnimations;
		std::thread                  thrd;
		bool                         running = true;
		void                         main() {
      const time_point tEpoch = owner._tEpoch;
      time_point       tLast  = tEpoch;
      while (1) {
        barrier->waitForFrame();
        if (!running) break;
        auto tNow = clock::now();

        frame_time_t t  = (tNow - tEpoch).count();
        frame_time_t dt = (tNow - tLast).count();

        for (const auto &sa : animations) {
          sa.animation->iterate(
            sa.leds.data(), sa.leds.size(), sa.animation->userdata(), dt, t);
        }
      }
		}
	};

protected:
	time_point                             _tEpoch;
	bool                                   _dirty = false;
	std::vector<std::unique_ptr<Animator>> _animators;

  std::vector<

public:
	void flush() {
		if (_dirty) {
			for (auto &animator : _animators) {
				animator->animations = animator->nextAnimations;
			}
			_dirty = false;
		}
	}
};