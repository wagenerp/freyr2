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
#include "alpha4/common/logger.hpp"
#include "alpha4/common/math.hpp"
#include "alpha4/types/vector.hpp"
#include "blend_api.h"
#include "core/animation_api.h"
#include "core/basemodule_api.h"
#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/ledset.hpp"
#include "core/module_api.h"
#include "display_api.hpp"
#include "grouping_api.h"
#include "modules/coordinates_api.h"
#include "types/stringlist.h"
#include "util/module.hpp"
#include <compare>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <sstream>
#include <stdio.h>
#include <string_view>
#include <vector>

struct Anim;
struct BlendAnimation;

static bool _AnimationsDirty = false;
static bool _Dirty           = false;

struct Anim {
	animno_t animno;
	animno_t newAnimno = INVALID_ANIMATION;

	LEDSet leds;
	LEDSet ledsActual;

	bool ledsDirty   = false;
	bool animnoDirty = false;

	void replaceAnimno(animno_t no) {
		newAnimno        = no;
		animnoDirty      = true;
		_AnimationsDirty = true;
		_Dirty           = true;
	}

	Anim(animno_t animno, LEDSet &&leds) :
		animno(animno), leds(std::move(leds)) {}
	Anim(animno_t animno, const LEDSet &leds) : animno(animno), leds(leds) {}
};

struct Blender {
	basemodno_t    blend_basemodno;
	blend_init_f   blend_init;
	blend_deinit_f blend_deinit;
	blend_mix_f    blend_mix;
	void *         blend_userdata = nullptr;

	Blender(basemodno_t blend_basemodno, const std::string &argstr) :
		blend_basemodno(blend_basemodno) {
		basemodule_grab(blend_basemodno);
		blend_init = reinterpret_cast<blend_init_f>(
			basemodule_resolve(blend_basemodno, "init"));
		blend_deinit = reinterpret_cast<blend_deinit_f>(
			basemodule_resolve(blend_basemodno, "deinit"));
		blend_mix =
			reinterpret_cast<blend_mix_f>(basemodule_resolve(blend_basemodno, "mix"));

		if (!blend_mix) {
			RESPOND(E) << "bad blend module - has no mix function" << alp::over;
		}

		if (blend_init) {
			blend_init(argstr.c_str(), &blend_userdata);
			if (blend_userdata && !blend_deinit) {
				RESPOND(E) << "bad blend module - allocated userdata without deinit"
									 << alp::over;
			}
		}
	}

	~Blender() { basemodule_drop(blend_basemodno); }
};

struct BlendAnimation {
	std::shared_ptr<Anim> anim1;
	std::shared_ptr<Anim> anim2;

	std::shared_ptr<Blender> blender;

	std::shared_ptr<Anim> self = nullptr;

	std::vector<led_t> buffer = {};

	BlendAnimation(

		std::shared_ptr<Anim>    anim1,
		std::shared_ptr<Anim>    anim2,
		std::shared_ptr<Blender> blender) :
		anim1(anim1), anim2(anim2), blender(blender) {}

	~BlendAnimation() {}

	void
	iterate(const led_i_t *ledv, size_t ledn, frame_time_t dt, frame_time_t t) {
		led_t *raw = frame_raw_anim();
		anim_render(anim1->animno, ledv, ledn, dt, t);

		// save op1 result
		buffer.resize(ledn);
		for (size_t i = 0; i < ledn; i++) {
			buffer[i] = raw[ledv[i]];
		}

		// compute op2
		anim_render(anim2->animno, ledv, ledn, dt, t);

		if (blender->blend_mix) {
			if (blender->blend_mix(
						ledv, ledn, raw, buffer.data(), dt, t, blender->blend_userdata)) {
				self->replaceAnimno(anim2->animno);
			}
		}
	}

	static void iterate(
		const led_i_t *ledv,
		size_t         ledn,
		void *         ud,
		frame_time_t   dt,
		frame_time_t   t) {
		((BlendAnimation *)ud)->iterate(ledv, ledn, dt, t);
	}
	static void deinit(void *ud) {
		_AnimationsDirty = true;

		delete (BlendAnimation *)ud;
	}
};

static std::vector<std::shared_ptr<Anim>> _Animations;

struct Tier {
	std::string                        name;
	unsigned                           priority_major = 0;
	unsigned                           priority_minor = 0;
	std::vector<std::shared_ptr<Anim>> anims;

	Tier(const std::string &name, unsigned priority_major = 0) :
		name(name), priority_minor(priority_major) {}

	std::weak_ordering operator<=>(const Tier &b) const {
		if (priority_major < b.priority_major) return std::weak_ordering::less;
		if (priority_major > b.priority_major) return std::weak_ordering::greater;
		if (priority_minor < b.priority_minor) return std::weak_ordering::less;
		if (priority_minor > b.priority_minor) return std::weak_ordering::greater;
		if (name < b.name) return std::weak_ordering::less;
		if (name > b.name) return std::weak_ordering::greater;
		return std::weak_ordering::equivalent;
	}

	void clear(const LEDSet &leds) {
		for (auto it = anims.begin(); it != anims.end();) {
			Anim &anim = **it;
			{
				const size_t n0 = anim.leds.size();
				anim.leds -= leds;
				if (n0 == anim.leds.size()) {
					++it;
					continue;
				};
			}
			_AnimationsDirty = true;
			anim.ledsDirty   = true;

			if (anim.leds.empty()) {
				it = anims.erase(it);
			} else {
				++it;
			}
		}
	}

	void install(std::shared_ptr<Anim> targetAnim) {
		clear(targetAnim->leds);
		anims.push_back(targetAnim);
		_Dirty = true;
	}

	void blendTo(
		std::shared_ptr<Anim> targetAnim,
		basemodno_t           blend_basemodno,
		const std::string &   blendArgs) {
		auto                               leds = targetAnim->leds;
		std::vector<std::shared_ptr<Anim>> newAnims;
		auto blender = std::make_shared<Blender>(blend_basemodno, blendArgs);

		for (auto it = anims.begin(); it != anims.end();) {
			Anim & anim = **it;
			LEDSet ledsBlending;
			{
				const size_t n0 = anim.leds.size();
				ledsBlending    = anim.leds;
				anim.leds -= leds;
				if (n0 == anim.leds.size()) {
					++it;
					continue;
				};
				ledsBlending %= leds;
			}
			_AnimationsDirty = true;
			anim.ledsDirty   = true;

			leds -= ledsBlending;
			{
				auto     blendAnim = new BlendAnimation(*it, targetAnim, blender);
				animno_t animno    = anim_define(
          "blend",
          BlendAnimation::iterate,
          BlendAnimation::deinit,
          (void *)blendAnim,
          ledsBlending.data(),
          ledsBlending.size());
				if (INVALID_ANIMATION == animno) {
					RESPOND(E) << "unable to define blending animation" << alp::over;
					delete blendAnim;
				} else {
					auto anim = std::make_shared<Anim>(animno, std::move(ledsBlending));
					blendAnim->self = anim;
					_Animations.push_back(anim);
					anim_grab(animno);
					newAnims.push_back(anim);
					_Dirty = true;
				}
			}
			if (anim.leds.empty()) {
				it = anims.erase(it);
			} else {
				++it;
			}
		}
		if (!leds.empty()) {
			auto anim = std::make_shared<Anim>(targetAnim->animno, std::move(leds));
			_Animations.push_back(anim);
			anim_grab(targetAnim->animno);
			newAnims.push_back(anim);
			_Dirty = true;
		}
		anims.insert(anims.end(), newAnims.begin(), newAnims.end());
	}
};

static std::vector<Tier> _Tierset;
static void              _ElevateTier(size_t iTier) {
  auto &target = _Tierset[iTier];
  for (size_t i = 0; i < _Tierset.size(); i++) {
    if (i == iTier) continue;
    auto &tier = _Tierset[i];
    if (tier.priority_major != target.priority_major) continue;
    if (tier.priority_minor >= target.priority_minor) {
      target.priority_minor = tier.priority_minor + 1;
      _Dirty                = true;
    }
  }
}

Tier *getTier(const std::string &ident, bool create = true) {
	for (auto &tier : _Tierset) {
		if (tier.name == ident) return &tier;
	}
	if (create) {
		_Tierset.emplace_back(ident);
		_Dirty = true;

		_ElevateTier(_Tierset.size() - 1);
		hook_trigger(hook_resolve("idlChanged"));

		return &_Tierset.back();
	}
	return nullptr;
}

extern "C" {

modno_t SingletonInstance = INVALID_MODULE;

void mod_display_status() {
	auto msg = std::move(RESPOND(I) << "anims: " << _Animations.size() << "\n");
	for (auto anim : _Animations) {
		msg << "  anim " << anim->animno << " (" << anim->ledsActual.size() << "/"
				<< anim->leds.size() << " leds)\n";
	}
	for (auto &tier : _Tierset) {
		msg << "tier " << tier.name << " (" << tier.priority_major << "."
				<< tier.priority_minor << ")\n";
		for (auto &anim : tier.anims) {
			msg << "  anim " << anim->animno << " (" << anim->ledsActual.size() << "/"
					<< anim->leds.size() << " leds)\n";
		}
	}
	msg << alp::over;
}

static void         _cmd_display(modno_t, const char *argstr, void *);
static uidl_node_t *_desc_display(void *);
static void         _cmd_float(modno_t, const char *argstr, void *);
static uidl_node_t *_desc_float(void *);
static void         _cmd_tier(modno_t, const char *argstr, void *);
static uidl_node_t *_desc_tier(void *);
static void         _hook_ledsRemoved(hook_t, modno_t, void *) {
  for (auto it : _Animations) {
    it->leds.adjustRemovedLEDs(
      egress_leds_removed_offset(), egress_leds_removed_count());
    it->ledsDirty = true;
  }
  _AnimationsDirty = true;
}

void init(modno_t modno, const char *, void **) {
	module_register_command(modno, "display", _cmd_display, _desc_display);
	module_register_command(modno, "float", _cmd_float, _desc_float);
	module_register_command(modno, "tier", _cmd_tier, _desc_tier);
	module_hook(modno, hook_resolve("ledsRemoved"), _hook_ledsRemoved);
}
void deinit(modno_t, void *) {
	_AnimationsDirty= false;
	_Dirty = false;
	_Animations.clear();
	_Tierset.clear();

}
void flush(modno_t, void *) {
	if (_AnimationsDirty) {
		// cleanup anims and update restricted leds
		for (auto it = _Animations.begin(); it != _Animations.end();) {
			if (it->use_count() == 1) {
				anim_drop((**it).animno);
				it = _Animations.erase(it);
			} else {
				if ((**it).ledsDirty) {
					anim_restrict((**it).animno, (**it).leds.data(), (**it).leds.size());
					(**it).ledsDirty = false;
				}
				if ((**it).animnoDirty) {
					anim_grab((**it).newAnimno);
					anim_drop((**it).animno);
					(**it).animno      = (**it).newAnimno;
					(**it).animnoDirty = false;
				}
				++it;
			}
		}
		_AnimationsDirty = false;
	}
	if (_Dirty) {
		anim_clearAll();
		std::sort(_Tierset.begin(), _Tierset.end());

		std::vector<size_t> led_tierset_map;

		led_tierset_map.resize(frame_size());
		{
			size_t iTier = 0;
			for (auto &tier : _Tierset) {
				for (auto anim : tier.anims) {
					for (auto i : anim->leds.storage()) {
						led_tierset_map[i] = iTier;
					}
				}
				iTier++;
			}
		}

		{
			size_t iTier = 0;
			for (auto &tier : _Tierset) {
				for (auto anim : tier.anims) {
					anim->ledsActual = anim->leds;
					{
						auto guard = anim->ledsActual.beginModification();

						anim->ledsActual.clear();

						for (auto i : anim->leds.storage()) {
							if (led_tierset_map[i] == iTier) {
								anim->ledsActual.append(&i, 1);
							}
						}
					}

					if (anim->ledsActual.empty()) continue;

					anim_install(anim->animno);
				}
				iTier++;
			}
		}
	}
}
static uidl_node_t *_descTiers(const char *ident) {
	uidl_node_t *common_tier = uidl_keyword(ident, 0);
	for (const auto &tier : _Tierset) {
		uidl_keyword_set(common_tier, tier.name.c_str(), 0);
	}
	return common_tier;
}

static void _cmd_display(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);

	std::stringstream animArgs;
	std::string       animName;
	if (!ln.get(animName)) {
		RESPOND(E) << "incomplete display command - animation name expected"
							 << alp::over;
		return;
	}
	bool              blend = false;
	std::stringstream blendArgs;
	std::string       blendName;

	size_t      pos = ln.tell();
	std::string cmd;

	LEDSet leds;

	std::string             tierName = "default";
	std::optional<unsigned> priority = std::nullopt;

	{
		auto ledguard = leds.beginModification();
		while (ln.get(cmd)) {
			if (cmd == "on") {
				if (!display_processSelector(leds, ln)) return;
			} else if (cmd == "tier") {
				if (!ln.get(tierName)) {
					RESPOND(E) << "incomplete display command - tier name expected after "
												"'tier'"
										 << alp::over;
					return;
				}

			} else if (cmd == "priority") {
				if (!ln.get(priority)) {
					RESPOND(E)
						<< "incomplete display command - priority value expected after "
							 "'priority'"
						<< alp::over;
					return;
				}
			} else if (cmd == "blend") {
				blend = true;
				if (!ln.get(blendName)) {
					RESPOND(E)
						<< "incomplete display command - blend module name expected"
						<< alp::over;
				}
			} else if (blend) {
				blendArgs << std::string_view(argstr + pos, ln.tell() - pos);
			} else {
				animArgs << std::string_view(argstr + pos, ln.tell() - pos);
			}
			pos = ln.tell();
		}
	}

	animno_t animno = anim_init(
		animName.c_str(), leds.data(), leds.size(), animArgs.str().c_str());
	if (INVALID_ANIMATION == animno) {
		RESPOND(E) << "unable to init animation '" << animName << "'" << alp::over;
		return;
	}

	auto anim = std::make_shared<Anim>(animno, std::move(leds));

	auto tier = getTier(tierName, true);
	if (nullptr == tier) {
		RESPOND(E) << "unknown error creating tier '" << tierName << "'"
							 << alp::over;
		return;
	}

	_Animations.push_back(anim);
	anim_grab(animno);
	if (blend) {
		basemodno_t bmod = basemodule_init(("blend_" + blendName).c_str());
		if (INVALID_BASEMOD == bmod) {
			RESPOND(E) << "unable to find blend module " << blendName << alp::over;
			tier->install(anim);
		} else {
			tier->blendTo(anim, bmod, blendArgs.str());
		}
		// std::shared_ptr<BlendModule> blendModule;
		// if (!(blendModule = BlendRegistry().get(blendName))) {
		// 	RESPOND(E) << "unable to init blend module '" << blendName << "'"
		// 						 << alp::over;
		// }
		// tier->blendTo(anim, blendModule, blendArgs.str());
		// tier->blendTo(anim, blendName, blendArgs.str());
	} else {
		tier->install(anim);
	}

	if (priority) { tier->priority_major = *priority; }
}

static uidl_node_t *_desc_display(void *) {
	auto mods = basemodule_list_get();

	uidl_node_t *res = uidl_keyword(nullptr, 0);

	uidl_node_t *common_selector = display_describeSelector("display.selector");
	uidl_node_t *common_tier     = _descTiers("display.tier");
	uidl_node_t *common_priority =
		uidl_integer("display.priority", UIDL_LIMIT_LOWER, 0, 0);
	uidl_node_t *common_blend = uidl_keyword("display.blend", 0);

	bool first = true;

	for (size_t i = 0; i < mods->count; i++) {
		if (strncmp(mods->modules[i], "anim_", 5) == 0) {
			std::string ident     = mods->modules[i] + 5;
			basemodno_t basemodno = basemodule_init(mods->modules[i]);
			if (INVALID_BASEMOD == basemodno) continue;
			uidl_node_t *subkw = basemodule_describe(basemodno);
			if (nullptr == subkw) subkw = uidl_keyword(nullptr, 0);

			if (first) {
				uidl_keyword_set(subkw, "on", common_selector);
				uidl_keyword_set(subkw, "tier", common_tier);
				uidl_keyword_set(subkw, "priority", common_priority);
				uidl_keyword_set(subkw, "blend", common_blend);
				first = false;
			} else {
				uidl_keyword_set(subkw, "on", uidl_reference("display.selector"));
				uidl_keyword_set(subkw, "tier", uidl_reference("display.tier"));
				uidl_keyword_set(subkw, "priority", uidl_reference("display.priority"));
				uidl_keyword_set(subkw, "blend", uidl_reference("display.blend"));
			}

			uidl_keyword_set(res, ident.c_str(), uidl_repeat(nullptr, subkw, 0));
		} else if (strncmp(mods->modules[i], "blend_", 6) == 0) {
			std::string  ident     = mods->modules[i] + 6;
			uidl_node_t *subkw     = nullptr;
			basemodno_t  basemodno = basemodule_init(mods->modules[i]);
			subkw                  = basemodule_describe(basemodno);

			uidl_keyword_set(
				common_blend,
				ident.c_str(),
				(nullptr != subkw) ? uidl_repeat(nullptr, subkw, 0) : nullptr);
		}
	}
	stringlist_free(mods);

	if (first) {
		uidl_node_free(common_selector);
		uidl_node_free(common_tier);
		uidl_node_free(common_priority);
		uidl_node_free(common_blend);
	}
	return res;
}

static void _cmd_float(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);
	LEDSet           leds;
	if (!display_processSelector(leds, ln)) return;
	if (leds.empty()) return;

	std::optional<std::string> tierName;
	{
		std::string cmd;

		while (ln.get(cmd)) {
			if (cmd == "tier") {
				if (!ln.get(tierName)) {
					RESPOND(E) << "incomplete float command - tier name expected after "
												"'tier'"
										 << alp::over;
					return;
				}
			}
		}
	}
	for (auto &tier : _Tierset) {
		if (tierName && (tier.name != *tierName)) continue;
		tier.clear(leds);
	}
}

static uidl_node_t *_desc_float(void *) {
	return uidl_sequence(
		0,
		2,
		display_describeSelector(0),
		uidl_repeat(
			0,

			uidl_keyword(0, 1, uidl_pair("tier", _descTiers(0)))

				,
			0)

	);
}

static void _cmd_tier(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);
	std::string      tierName;

	if (!ln.get(tierName)) {
		RESPOND(E) << "incomplete tier command - tier name expected" << alp::over;
		return;
	}

	std::vector<Tier>::iterator itTier;

	for (itTier = _Tierset.begin(); itTier != _Tierset.end(); ++itTier) {
		if (itTier->name == tierName) break;
	}

	if (itTier == _Tierset.end()) {
		RESPOND(E) << "tier '" << tierName << "' not found" << alp::over;
		return;
	}

	{
		std::string cmd;

		while (ln.get(cmd)) {
			if (cmd == "elevate") {
				_ElevateTier(itTier - _Tierset.begin());

			} else if (cmd == "priority") {
				unsigned newMajor = 0;
				if (!ln.get(newMajor)) {
					RESPOND(E) << "incomplete tier command - priority number expected "
												"after 'priority'"
										 << alp::over;
					return;
				}
				if (itTier->priority_major != newMajor) _Dirty = true;
				itTier->priority_major = newMajor;
				_ElevateTier(itTier - _Tierset.begin());
			} else if (cmd == "remove") {
				_Tierset.erase(itTier);
				_Dirty = true;
				hook_trigger(hook_resolve("idlChanged"));
				return;
			}
		}
	}
}

static uidl_node_t *_desc_tier(void *) {
	return uidl_sequence(
		0,
		2,
		_descTiers(0),
		uidl_repeat(
			0,

			uidl_keyword(
				0,
				3,

				uidl_pair("elevate", 0),
				uidl_pair("priority", uidl_integer(0, 0, 0, 0)),
				uidl_pair("remove", 0)

					),
			0)

	);
}
}