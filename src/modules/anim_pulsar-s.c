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
#include "alpha4c/types/vector.h"
#include "anim_common.h"
#include "modules/coordinates_api.h"
#include "modules/modulation_congress.h"

typedef struct ud_t {
	int reverse;

	vec3f_t center;

} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud            = (ud_t *)malloc(sizeof(ud_t));
	(**pud).reverse = 0;
	(**pud).center  = vec3f_set(0, 0, 0);

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "reverse") == 0) {
			(**pud).reverse = 1;
		} else if (strcmp(cmd, "center") == 0) {
			lscan_float(ln, &(**pud).center.x, LSCAN_MANY);
			lscan_float(ln, &(**pud).center.y, LSCAN_MANY);
			lscan_float(ln, &(**pud).center.z, LSCAN_MANY);
		} else if (strcmp(cmd, "cx") == 0) {
			lscan_float(ln, &(**pud).center.x, LSCAN_MANY);
		} else if (strcmp(cmd, "cy") == 0) {
			lscan_float(ln, &(**pud).center.y, LSCAN_MANY);
		} else if (strcmp(cmd, "cz") == 0) {
			lscan_float(ln, &(**pud).center.z, LSCAN_MANY);
		}

		free(cmd);
	}
	lscan_free(ln);
}

void deinit(ud_t *ud) { free((void *)ud); }

void iterate(
	const led_i_t *ledv, size_t ledn, ud_t *ud, frame_time_t, frame_time_t t) {
	led_t *                 leds   = frame_raw_anim();
	const led_coord_data_t *coords = coordinates_raw_anim();

	double d;
	for (size_t i = 0; i < ledn; i++) {
		led_t *                 led   = leds + ledv[i];
		const led_coord_data_t *coord = coords + ledv[i];

		{
			vec3f_t tmp = vec3f_sub(&coord->pos, &ud->center);
			d           = vec3f_norm(&tmp);
		}
		int idx = -(int)(d * (1.0 / 0.03));

		modulation_congress_apply(idx, t, ud->reverse, led);
	}
}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		5,
		uidl_pair("reverse", 0),
		uidl_pair(
			"center",
			uidl_sequence(
				0,
				3,
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0))),
		uidl_pair("cx", uidl_float(0, 0, 0, 0)),
		uidl_pair("cy", uidl_float(0, 0, 0, 0)),
		uidl_pair("cz", uidl_float(0, 0, 0, 0))

	);
}