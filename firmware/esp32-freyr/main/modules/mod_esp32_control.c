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

#include "alpha4c/common/linescanner.h"
#include "unicorn/idl.h"
#include "core/animation_api.h"
#include "core/frame_api.h"
#include "core/module_api.h"
#include <stddef.h>
#include <stdio.h>

extern color_c_t egress_esp32_rmt_brightness;

static int       _UpdateBrightness = 0;
static color_c_t _NewBrightness;

static void _cmd_brightness(modno_t, const char *argstr, void *) {
	lscan_t *ln = lscan_new(argstr, 0);

	float v = 2;
	lscan_float(ln, &v, LSCAN_MANY);
	if (((v >= 0) | (v <= 1))) {
		_NewBrightness              = v;
		egress_esp32_rmt_brightness = v;
		_UpdateBrightness           = 1;
	}

	lscan_free(ln);
}
static uidl_node_t *_desc_brightness(void *) {
	return uidl_float(NULL, UIDL_LIMIT_RANGE, 0, 1);
}
void mod_display_status();

static void _cmd_status(modno_t, const char *, void *) {
	anim_status();
	mod_display_status();
	fflush(stdout);
}

void mod_esp32_control_init(modno_t modno, const char *, void **) {
	module_register_command(
		modno, "brightness", _cmd_brightness, _desc_brightness);
	module_register_command(modno, "status", _cmd_status, NULL);
}

void mod_esp32_control_flush(modno_t, void *) {
	if (_UpdateBrightness) {
		egress_esp32_rmt_brightness = _NewBrightness;
		_UpdateBrightness           = 0;
	}
}
void mod_esp32_control_deinit(modno_t, void *) {}