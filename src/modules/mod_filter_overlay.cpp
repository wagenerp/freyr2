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
#include "alpha4/types/vector.hpp"
#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/ledset.hpp"
#include "core/module_api.h"
#include "display_api.hpp"
#include "modules/coordinates_api.h"
#include "util/module.hpp"
#include <cstdlib>
#include <iterator>
#include <stdio.h>
#include <vector>

extern "C" {
#include "alpha4c/common/math.h"
}
struct leda_t : public led_t {
	float a;
};
static std::vector<leda_t> _Overlay;

extern "C" {

modno_t             SingletonInstance = INVALID_MODULE;
static void         _cmd_overlay(modno_t, const char *argstr, void *);
static uidl_node_t *_desc_overlay(void *);
static void         _hook_ledsAdded(hook_t, modno_t, void *) {
  _Overlay.resize(_Overlay.size() + egress_leds_added_count(), {{0, 0, 0}, 1});
}
static void _hook_ledsRemoved(hook_t, modno_t, void *) {
	_Overlay.erase(
		_Overlay.begin() + egress_leds_removed_offset(),
		_Overlay.begin()
			+ (egress_leds_removed_offset() + egress_leds_removed_count()));
}

static void _hook_applyFilter(hook_t, modno_t, void *) {
	auto       leds = frame_raw_egress();
	const auto ce   = frame_size();
	for (size_t i = 0; i < ce; i++) {
		float a   = _Overlay[i].a;
		leds[i].r = leds[i].r * a + _Overlay[i].r;
		leds[i].g = leds[i].g * a + _Overlay[i].g;
		leds[i].b = leds[i].b * a + _Overlay[i].b;
	}
}

void init(modno_t modno, const char *, void **) {
	module_register_command(modno, "overlay", _cmd_overlay, _desc_overlay);
	module_hook(modno, hook_resolve("ledsAdded"), _hook_ledsAdded);
	module_hook(modno, hook_resolve("ledsRemoved"), _hook_ledsRemoved);
	module_hook(modno, hook_resolve("applyFilter"), _hook_applyFilter);
}
void deinit(modno_t, void *) { _Overlay.clear(); }

static void _cmd_overlay(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);
	while (!ln.eof()) {
		LEDSet leds;
		if (!display_processSelector(leds, ln)) return;

		std::string raw;
		for (auto it = leds.begin(), end = leds.end(); it != end;) {
			if (!ln.get(raw)) return;

			unsigned long count = 1;

			if (raw == "clear") {
				for (; it != end; ++it) {
					_Overlay[*it] = {{0, 0, 0}, 1};
				}
				break;
			} else if (raw.front() == 'x') {
				count = std::strtoul(raw.c_str() + 1, nullptr, 0);
				if (!ln.get(raw)) return;
			} else if (raw == "skip") {
				unsigned long skipCount = 0;
				if (!ln.get(skipCount)) return;
				for (; (skipCount > 0) && (it != end); --skipCount, ++it) {}
				continue;
			}

			leda_t v;
			{
				uint32_t rgba8 = std::strtoull(raw.c_str(), nullptr, 16);
				if (raw.size() == 8) {
					const float a = fclamp(((rgba8)&0xff) * (1.0f / 255.f));
					v.r           = ((rgba8 >> 24) & 0xff) * (1.0f / 255.f) * a;
					v.g           = ((rgba8 >> 16) & 0xff) * (1.0f / 255.f) * a;
					v.b           = ((rgba8 >> 8) & 0xff) * (1.0f / 255.f) * a;
					v.a           = 1.0 - a;
				} else if (raw.size() == 6) {
					v.r = ((rgba8 >> 16) & 0xff) * (1.0f / 255.f);
					v.g = ((rgba8 >> 8) & 0xff) * (1.0f / 255.f);
					v.b = ((rgba8 >> 0) & 0xff) * (1.0f / 255.f);
					v.a = 0;
				} else if (raw.size() == 4) {
					const float a = fclamp(((rgba8)&0xf) * (1.0f / 15.f));
					v.r           = ((rgba8 >> 12) & 0xf) * (1.0f / 15.f) * a;
					v.g           = ((rgba8 >> 8) & 0xf) * (1.0f / 15.f) * a;
					v.b           = ((rgba8 >> 4) & 0xf) * (1.0f / 15.f) * a;
					v.a           = 1 - a;
				} else if (raw.size() == 3) {
					v.r = ((rgba8 >> 8) & 0xf) * (1.0f / 15.f);
					v.g = ((rgba8 >> 4) & 0xf) * (1.0f / 15.f);
					v.b = ((rgba8 >> 0) & 0xf) * (1.0f / 15.f);
					v.a = 0;
				}
			}

			for (; (count > 0) && (it != end); --count, ++it) {
				_Overlay[*it] = v;
			}
		}
	}
}

static uidl_node_t *_desc_overlay(void *) {
	return uidl_repeat(
		0,
		uidl_sequence(
			0, 2, display_describeSelector(0), uidl_repeat(0, uidl_string(0, 0), 0)),
		0);
}
}