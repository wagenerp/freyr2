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

#ifndef BLEND_API_H
#define BLEND_API_H
#include "core/animation_api.h"
#include "core/frame_api.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum blend_state_t { BLEND_ACTIVE, BLEND_DONE } blend_state_t;

typedef void (*blend_init_f)(const char *argstr, void **puserdata);
typedef void (*blend_deinit_f)(void *userdata);

typedef blend_state_t (*blend_mix_f)(
	const led_i_t *ledv,
	size_t         ledn,
	led_t *        accum,
	const led_t *  op2,
	frame_time_t   dt,
	frame_time_t   t,
	void *         userdata);

#ifdef __cplusplus
}
#endif

#endif
