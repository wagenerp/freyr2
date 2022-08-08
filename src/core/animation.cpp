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

#include "animation.hpp"

#include "alpha4/common/logger.hpp"
#include "core/animation_api.h"
#include "core/frame_api.h"
#include "util/module.hpp"
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>

static bool _AnimationDropped = false;
static std::unordered_map<animno_t, std::shared_ptr<Animation>> _AnimationMap;
static animno_t _AnimationCount = 0;
static animno_t AllocateAnimationNumber() { return ++_AnimationCount; }

void Animation::LEDsRemoved(led_i_t offset, led_i_t count) {
	for (auto it : _AnimationMap) {
		it.second->_leds.adjustRemovedLEDs(offset, count);
	}
}

Animation::Animation(const std::string &ident, basemodno_t basemodno) :
	_animno(AllocateAnimationNumber()), _basemodno(basemodno), _ident(ident) {
	_init =
		reinterpret_cast<animation_init_f>(basemodule_resolve(_basemodno, "init"));
	_deinit = reinterpret_cast<animation_deinit_f>(
		basemodule_resolve(_basemodno, "deinit"));
	_iterate = reinterpret_cast<animation_iterate_t>(
		basemodule_resolve(_basemodno, "iterate"));

	if (!_iterate) {
		alp::thrower<AnimationInitError>()
			<< "bad animation module " << basemodule_name(basemodno)
			<< " - has no iteration function" << alp::over;
	}
	if (_deinit && !_init) {
		alp::thrower<AnimationInitError>()
			<< "bad animation module " << basemodule_name(basemodno)
			<< " - has deinit without init" << alp::over;
	}
	basemodule_grab(_basemodno);
}

Animation::Animation(
	const std::string & ident,
	animation_iterate_t iterate,
	animation_deinit_f  deinit,
	void *              userdata) :
	_animno(AllocateAnimationNumber()),
	_ident(ident),
	_init(nullptr),
	_deinit(deinit),
	_iterate(iterate),
	_userdata(userdata) {}

Animation::~Animation() {
	if (_deinit) { _deinit(_userdata); }
	basemodule_drop(_basemodno);
}
void Animation::setLEDs(const led_i_t *ledv, size_t ledn) {
	if (_initialized) {
		alp::thrower<AnimationInitError>()
			<< "attempted to configure LEDs after animation initialization"
			<< alp::over;
	}
	_leds.append(ledv, ledn);
}

void Animation::initialize(const std::string &argstring) {
	if (_initialized) {
		alp::thrower<AnimationInitError>()
			<< "attempted to initialize animation twice" << alp::over;
	}
	_initialized = true;

	_usageCount++;
	if (_init) {
		_init(_leds.data(), _leds.size(), argstring.c_str(), &_userdata);

		if ((nullptr != _userdata) && (!_deinit)) {
			LOG(E) << "animation #" << _animno << " (" << basemodule_name(_basemodno)
						 << ") allocated userdata without destructor" << alp::over;
		}
	}
}

void Animation::doIterate(frame_time_t dt, frame_time_t t) const {
	_iterate(_leds.data(), _leds.size(), _userdata, dt, t);
}

AnimatorPool::Animator::Animator(AnimatorPool &owner) :
	owner(owner), tEpoch(owner._tEpoch), tLast(owner._tEpoch) {}

void AnimatorPool::Animator::renderFrame() {
	auto tNow = std::chrono::time_point_cast<duration>(clock::now());

	frame_time_t t  = (tNow - tEpoch).count();
	frame_time_t dt = (tNow - tLast).count();
	tLast           = tNow;

	for (const auto &sa : animations) {
		sa.animation->iterate()(
			sa.leds.data(), sa.leds.size(), sa.animation->userdata(), dt, t);
	}
}

AnimatorPool::AnimatorPool() :
	_tEpoch(std::chrono::time_point_cast<duration>(clock::now())) {}

AnimatorPool &AnimatorPool::Get() {
	static AnimatorPool pool;
	return pool;
}

void AnimatorPool::setup(size_t animatorCount) {
	_animators.clear();
	for (size_t i = 0; i < animatorCount; i++) {
		_animators.emplace_back(std::make_unique<Animator>(*this));
	}
}

void AnimatorPool::flush() {
	if (_AnimationDropped) {
		for (auto it = _AnimationMap.begin(); it != _AnimationMap.end();) {
			if (it->second->usageCount() < 2) {
				it = _AnimationMap.erase(it);
			} else {
				++it;
			}
		}
		_AnimationDropped = false;
	}
	if (_dirty) {
		for (auto &animator : _animators) {
			animator->animations = animator->nextAnimations;
		}
		_dirty = false;
	}
}

void AnimatorPool::ledsRemoved(led_i_t offset, led_i_t count) {
	for (auto &animator : _animators) {
		auto &anims = animator->nextAnimations;
		for (auto it = anims.begin(); it != anims.end();) {
			it->leds.adjustRemovedLEDs(offset, count);
			if (it->leds.empty()) {
				it = anims.erase(it);
			} else {
				++it;
			}
		}
	}
	_dirty = true;
}

void AnimatorPool::clear(const LEDSet &leds) {
	for (auto &animator : _animators) {
		auto &anims = animator->nextAnimations;
		for (auto it = anims.begin(); it != anims.end();) {
			it->leds -= leds;
			if (it->leds.empty()) {
				it = anims.erase(it);
			} else {
				++it;
			}
		}
	}
	_dirty = true;
}

void AnimatorPool::clear() {
	for (auto &animator : _animators) {
		auto &anims = animator->nextAnimations;

		if (anims.empty()) continue;
		anims.clear();
		_dirty = true;
	}
}

void AnimatorPool::install(std::shared_ptr<Animation> animation) {
	clear(animation->leds());
	// we put all animations into the first animator here. A call to flush()
	// redistributes for load balancing.
	_animators.back()->nextAnimations.emplace_back(animation, animation->leds());
	_dirty = true;
}

extern "C" {

void anim_status() {
	auto msg =
		std::move(RESPOND(I) << "animations: " << _AnimationMap.size() << "\n");
	for (auto it : _AnimationMap) {
		msg << "  #" << it.second->animno() << ": " << it.second->ident()
				<< " uc:" << it.second->usageCount()
				<< " leds:" << it.second->leds().size() << "\n";
	}
	msg << alp::over;
}

animno_t anim_init(
	const char *ident, const led_i_t *ledv, size_t ledn, const char *argstr) {
	std::string moduleName = "anim_" + std::string(ident);
	basemodno_t bmod       = basemodule_init(moduleName.c_str());
	if (INVALID_BASEMOD == bmod) {
		RESPOND(W) << "unable to find animation " << ident << alp::over;
		return INVALID_ANIMATION;
	}

	{
		auto animation = std::make_shared<Animation>(ident, bmod);
		animation->setLEDs(ledv, ledn);
		animation->initialize(argstr);
		_AnimationMap[animation->animno()] = animation;
		return animation->animno();
	}
}

animno_t anim_define(
	const char *        ident,
	animation_iterate_t iterate,
	animation_deinit_f  deinit,
	void *              userdata,
	const led_i_t *     ledv,
	size_t              ledn) {
	auto animation =
		std::make_shared<Animation>(ident, iterate, deinit, userdata);
	animation->setLEDs(ledv, ledn);
	animation->initialize("");
	_AnimationMap[animation->animno()] = animation;
	return animation->animno();
}

void anim_restrict(animno_t anim, const led_i_t *ledv, size_t ledn) {
	if (auto it = _AnimationMap.find(anim); it != _AnimationMap.end()) {
		LEDSet envelope;
		envelope.append(ledv, ledn);
		it->second->restrict(envelope);

		AnimatorPool::Get().install(it->second);
	}
}

void anim_install(animno_t anim) {
	if (auto it = _AnimationMap.find(anim); it != _AnimationMap.end()) {
		AnimatorPool::Get().install(it->second);
	}
}
void anim_uninstall(animno_t anim) {
	if (auto it = _AnimationMap.find(anim); it != _AnimationMap.end()) {
		AnimatorPool::Get().clear(it->second->leds());
	}
}
void anim_clear(const led_i_t *ledv, size_t ledn) {
	LEDSet leds;
	leds.append(ledv, ledn);
	AnimatorPool::Get().clear(leds);
}
void anim_clearAll() { AnimatorPool::Get().clear(); }

void anim_grab(animno_t anim) {
	auto it = _AnimationMap.find(anim);
	if (it == _AnimationMap.end()) {
		LOG(W) << "attempted to grab non-existing animation #" << anim << alp::over;
		return;
	}
	it->second->grab();
}
void anim_drop(animno_t anim) {
	auto it = _AnimationMap.find(anim);
	if (it == _AnimationMap.end()) {
		LOG(W) << "attempted to drop non-existing animation #" << anim << alp::over;
		return;
	}
	it->second->drop();
	_AnimationDropped = true;
}

void anim_render(
	animno_t       anim,
	const led_i_t *ledv,
	size_t         ledn,
	frame_time_t   dt,
	frame_time_t   t) {
	if (auto it = _AnimationMap.find(anim); it != _AnimationMap.end()) {
		it->second->iterate()(ledv, ledn, it->second->userdata(), dt, t);
	}
}

void anim_cleanup() {
	AnimatorPool::Get().clear();
	_AnimationMap.clear();
	_AnimationCount = 0;
}
}