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
#include <stdio.h>
#include <vector>

static std::vector<float> _Brightness;

extern "C" {

modno_t             SingletonInstance = INVALID_MODULE;
static void         _cmd_brightness(modno_t, const char *argstr, void *);
static uidl_node_t *_desc_brightness(void *);
static void         _hook_ledsAdded(hook_t, modno_t, void *) {
  _Brightness.resize(_Brightness.size() + egress_leds_added_count(), 1);
}
static void _hook_ledsRemoved(hook_t, modno_t, void *) {
	_Brightness.erase(
		_Brightness.begin() + egress_leds_removed_offset(),
		_Brightness.begin()
			+ (egress_leds_removed_offset() + egress_leds_removed_count()));
}

static void _hook_applyFilter(hook_t, modno_t, void *) {
	auto       leds = frame_raw_egress();
	const auto ce   = frame_size();
	for (size_t i = 0; i < ce; i++) {
		leds[i].r *= _Brightness[i];
		leds[i].g *= _Brightness[i];
		leds[i].b *= _Brightness[i];
	}
}

void init(modno_t modno, const char *, void **) {
	module_register_command(
		modno, "brightness", _cmd_brightness, _desc_brightness);
	module_hook(modno, hook_resolve("ledsAdded"), _hook_ledsAdded);
	module_hook(modno, hook_resolve("ledsRemoved"), _hook_ledsRemoved);
	module_hook(modno, hook_resolve("applyFilter"), _hook_applyFilter);
}
void deinit(modno_t, void *) { _Brightness.clear(); }

static void _cmd_brightness(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);
	while (!ln.eof()) {
		LEDSet leds;
		float  brightness = 1;
		if (!display_processSelector(leds, ln) || !ln.get(brightness)) return;

		for (auto i : leds) {
			_Brightness[i] = brightness;
		}
	}
}

static uidl_node_t *_desc_brightness(void *) {
	return uidl_repeat(
		0,
		uidl_sequence(0, 2, display_describeSelector(0), uidl_float(0, 0, 0, 0)),
		0);
}
}