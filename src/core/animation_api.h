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

#ifndef ANIMATION_API_H
#define ANIMATION_API_H
#include "frame_api.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <unicorn/idl.h>
#include <stdint.h>
#include <stdlib.h>

typedef size_t animno_t;
#define INVALID_ANIMATION ((animno_t)0)

typedef double frame_time_t;

typedef void (*animation_init_f)(
	const led_i_t *ledv, size_t ledn, const char *argstr, void **puserdata);
typedef void (*animation_deinit_f)(void *userdata);

typedef void (*animation_iterate_t)(
	const led_i_t *ledv,
	size_t         ledn,
	void *         userdata,
	frame_time_t   dt,
	frame_time_t   t);

typedef struct animation_prototype_t {
	animation_init_f    init;
	animation_deinit_f  deinit;
	animation_iterate_t iterate;
} animation_prototype_t;

void anim_status();

animno_t anim_init(
	const char *ident, const led_i_t *ledv, size_t ledn, const char *argstr);

animno_t anim_define(
	const char *        ident,
	animation_iterate_t iterate,
	animation_deinit_f  deinit,
	void *              userdata,
	const led_i_t *     ledv,
	size_t              ledn);

void anim_restrict(animno_t anim, const led_i_t *ledv, size_t ledn);

void anim_install(animno_t anim);
void anim_uninstall(animno_t anim);
void anim_clear(const led_i_t *ledv, size_t ledn);
void anim_clearAll();
void anim_grab(animno_t anim);
void anim_drop(animno_t anim);

void anim_render(
	animno_t       anim,
	const led_i_t *ledv,
	size_t         ledn,
	frame_time_t   dt,
	frame_time_t   t);

void anim_cleanup();
#ifdef __cplusplus
}
#endif

#endif