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

#ifndef MODULATION_CONGRESS_H
#define MODULATION_CONGRESS_H
#include "alpha4c/common/inline.h"
#include "core/frame_api.h"
#include <math.h>

ALPHA4C_INLINE(void modulation_congress_apply)
(float p, float t, int reverse, led_t *led) {
	double period = 11;
	double phase_pre, phase;

	if (reverse) {
		phase_pre = -t * 4 + p * 0.3;
		phase     = fmod(fmod(phase_pre, period) + period, period) / period;
	} else {
		phase_pre = t * 4 + p * 0.3;
		phase     = fmod(phase_pre, period) / period;
	}

	phase = phase * 7 - 3;
	if ((phase < 0) || (phase > 1)) {
		led->r = led->g = led->b = 0;
	} else {
		double intensity = 1 - fabs(0.5 - phase) * 2;
		double hue;

		hue = t * 10 + p * 4;

		hsv(led, hue, 1, intensity);
	}
}
#endif