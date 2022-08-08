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

#ifndef CORE_ANIMATION_HPP
#define CORE_ANIMATION_HPP
#include "alpha4/common/error.hpp"
#include "animation_api.h"
#include "basemodule_api.h"
#include "core/ledset.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <ratio>
#include <string>
#include <thread>
#include <vector>

class AnimationInitError : public alp::Exception {
public:
	using alp::Exception::Exception;
	using alp::Exception::what;
};

class Animation {
protected:
	animno_t    _animno;
	basemodno_t _basemodno = INVALID_BASEMOD;
	std::string _ident;

	animation_init_f    _init;
	animation_deinit_f  _deinit;
	animation_iterate_t _iterate;

	size_t _usageCount  = 0;
	bool   _initialized = false;

	void * _userdata = nullptr;
	LEDSet _leds;

public:
	static void LEDsRemoved(led_i_t offset, led_i_t count);

public:
	Animation(const std::string &ident, basemodno_t basemodno);
	Animation(
		const std::string & ident,
		animation_iterate_t iterate,
		animation_deinit_f  deinit,
		void *              userdata);
	Animation(const Animation &) = delete;
	Animation(Animation &&)      = default;
	~Animation();

	animno_t            animno() const { return _animno; }
	const std::string & ident() const { return _ident; }
	animation_init_f    init() const { return _init; }
	animation_deinit_f  deinit() const { return _deinit; }
	animation_iterate_t iterate() const { return _iterate; }

	size_t usageCount() const { return _usageCount; }
	bool   initialized() const { return _initialized; }

	void *        userdata() const { return _userdata; }
	const LEDSet &leds() const { return _leds; }

	void restrict(const LEDSet &envelope) { _leds %= envelope; }

	void setLEDs(const led_i_t *ledv, size_t ledn);
	void initialize(const std::string &argstring);
	void doIterate(frame_time_t dt, frame_time_t t) const;

	void grab() { _usageCount++; }
	void drop() { _usageCount--; }
};

class AnimatorPool {
public:
	using duration   = std::chrono::duration<frame_time_t, std::ratio<1, 1>>;
	using clock      = std::chrono::steady_clock;
	using time_point = std::chrono::time_point<clock, duration>;

	struct SubAnimation {
		std::shared_ptr<Animation> animation;
		LEDSet                     leds;
	};

	struct Animator {
		AnimatorPool &            owner;
		std::vector<SubAnimation> animations;
		std::vector<SubAnimation> nextAnimations;
		time_point                tEpoch;
		time_point                tLast;
		bool                      running = true;
		Animator(AnimatorPool &owner);

		void renderFrame();
	};

protected:
	time_point                             _tEpoch;
	bool                                   _dirty = false;
	std::vector<std::unique_ptr<Animator>> _animators;
	AnimatorPool();

public:
	static AnimatorPool &Get();
	void                 setup(size_t animatorCount);
	void                 flush();
	size_t               animatorCount() const { return _animators.size(); }

	void renderFrame(size_t iAnimator) {
		if (iAnimator < _animators.size()) _animators[iAnimator]->renderFrame();
	}

	void ledsRemoved(led_i_t offset, led_i_t count);
	void clear();
	void clear(const LEDSet &leds);
	void install(std::shared_ptr<Animation> animation);
};
#endif