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

#include "alpha4c/types/vector.h"
#include "anim_common.h"
#include "modules/coordinates_api.h"
#include "modulation_congress.h"

void iterate(
	const led_i_t *ledv, size_t ledn, void *, frame_time_t, frame_time_t t) {
	led_t *leds = frame_raw_anim();

	for (size_t i = 0; i < ledn; i++) {
		led_t *led = leds + ledv[i];
		modulation_congress_apply(i, t, 0, led);
	}
}
