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
#include "alpha4c/common/math.h"
#include "alpha4c/types/vector.h"
#include "blend_api.h"
#include "coordinates_api.h"
#include "core/frame_api.h"
#include <string.h>

typedef struct ud_t {
	float   tAnim;
	float   speed;
	vec3f_t direction;
	float   d0;
	float   d1;

	float window;
} ud_t;

void init(const char *argstr, ud_t **pud) {
	*pud              = (ud_t *)malloc(sizeof(ud_t));
	(**pud).tAnim     = 0;
	(**pud).speed     = 0.2f;
	(**pud).direction = vec3f_set(randf() - 0.5f, randf() - 0.5f, randf() - 0.5f);
	(**pud).d0        = -100;
	(**pud).d1        = 100;
	(**pud).window    = 4;

	int d0set = 0;
	int d1set = 0;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "speed") == 0) {
			lscan_float(ln, &(**pud).speed, LSCAN_MANY);
		} else if (strcmp(cmd, "d") == 0) {
			lscan_float(ln, &(**pud).direction.x, LSCAN_MANY);
			lscan_float(ln, &(**pud).direction.y, LSCAN_MANY);
			lscan_float(ln, &(**pud).direction.z, LSCAN_MANY);
		} else if (strcmp(cmd, "dx") == 0) {
			lscan_float(ln, &(**pud).direction.x, LSCAN_MANY);
		} else if (strcmp(cmd, "dy") == 0) {
			lscan_float(ln, &(**pud).direction.y, LSCAN_MANY);
		} else if (strcmp(cmd, "dz") == 0) {
			lscan_float(ln, &(**pud).direction.z, LSCAN_MANY);
		} else if (strcmp(cmd, "d0") == 0) {
			lscan_float(ln, &(**pud).d0, LSCAN_MANY);
			d0set = 1;
		} else if (strcmp(cmd, "d1") == 0) {
			lscan_float(ln, &(**pud).d1, LSCAN_MANY);
			d1set = 1;
		} else if (strcmp(cmd, "window") == 0) {
			lscan_float(ln, &(**pud).window, LSCAN_MANY);
		}
		free(cmd);
	}
	lscan_free(ln);

	(**pud).direction = vec3f_normal(&(**pud).direction);

	if (!d0set | !d1set) {
		const led_coord_data_t *coords    = coordinates_raw_preanim();
		const size_t            ce_coords = frame_size();

		if (ce_coords > 0) {
			float dmin = vec3f_dot(&coords[0].pos, &(**pud).direction);
			float dmax = dmin;

			for (size_t i = 1; i < ce_coords; i++) {
				float d = vec3f_dot(&coords[i].pos, &(**pud).direction);
				if (d < dmin) dmin = d;
				if (d > dmax) dmax = d;
			}

			dmin -= (**pud).window;

			if (!d0set) (**pud).d0 = dmin;
			if (!d1set) (**pud).d1 = dmax;
		}
	}
}

void deinit(ud_t *ud) { free((void *)ud); }

blend_state_t mix(
	const led_i_t *ledv,
	size_t         ledn,
	led_t *        accum,
	const led_t *  op2,
	frame_time_t   dt,
	frame_time_t,
	ud_t *ud) {
	const led_coord_data_t *coords = coordinates_raw_anim();
	ud->tAnim += dt * ud->speed;

	float d = ud->d0 * fclamp(1.0f - ud->tAnim) + ud->d1 * fclamp(ud->tAnim);
	float windowInv = 1.0f / ud->window;
	for (size_t i = 0; i < ledn; i++) {
		float d1 = vec3f_dot(&coords[ledv[i]].pos, &ud->direction);

		float f = fclamp((d1 - d) * windowInv);
		float g = 1.0f - f;

		accum[ledv[i]].r = op2[i].r * f + accum[ledv[i]].r * g;
		accum[ledv[i]].g = op2[i].g * f + accum[ledv[i]].g * g;
		accum[ledv[i]].b = op2[i].b * f + accum[ledv[i]].b * g;
	}

	return (ud->tAnim < 1) ? BLEND_ACTIVE : BLEND_DONE;
}

uidl_node_t *describe() {
#define F uidl_float(0, 0, 0, 0)
	return uidl_keyword(
		0,
		8,

		uidl_pair("speed", F),
		uidl_pair("d", uidl_sequence(0, 3, F, F, F)),
		uidl_pair("dx", F),
		uidl_pair("dy", F),
		uidl_pair("dz", F),
		uidl_pair("d0", F),
		uidl_pair("d1", F),
		uidl_pair("window", F)

	);
}
