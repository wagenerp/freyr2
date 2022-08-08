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

#include "alpha4c/common/inline.h"
#include "core/frame_api.h"
#ifndef UTIL_ANIM_H
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

ALPHA4C_INLINE(void hsv)(led_t *led, float h, float s, float v) {
	h = fmod(h, 360.0f);
	if (h < 0) h += 360;
	float c = v * s;
	float m = v - c;
	float x = c * (1 - fabs(fmod(h / 60.0f, 2.0f) - 1)) + m;
	if (h < 60) {
		led->r = v;
		led->g = x;
		led->b = m;
	} else if (h < 120) {
		led->r = x;
		led->g = v;
		led->b = m;
	} else if (h < 180) {
		led->r = m;
		led->g = v;
		led->b = x;
	} else if (h < 240) {
		led->r = m;
		led->g = x;
		led->b = v;
	} else if (h < 300) {
		led->r = x;
		led->g = m;
		led->b = v;
	} else {
		led->r = v;
		led->g = m;
		led->b = x;
	}
}

#ifdef __cplusplus
}
#endif

#endif