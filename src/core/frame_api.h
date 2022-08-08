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

#ifndef FRAME_API_H
#define FRAME_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef uint32_t led_i_t;

typedef float color_c_t;

typedef struct [[gnu::packed]] led_t {
	color_c_t r;
	color_c_t g;
	color_c_t b;
}
led_t;

size_t frame_size();
led_t *frame_raw_preanim();
led_t *frame_raw_anim();
led_t *frame_raw_egress();

#ifdef __cplusplus
}
#endif

#endif