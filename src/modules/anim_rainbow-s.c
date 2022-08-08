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

#include "anim_common.h"

typedef struct ud_t {
	coord_t c;
	float   d;
	float   k;
	float   phase;
} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud          = (ud_t *)malloc(sizeof(ud_t));
	(**pud).c     = vec3f_set(1, 1, 1);
	(**pud).d     = 0;
	(**pud).k     = 120;
	(**pud).phase = 0;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "c") == 0) {
			lscan_float(ln, &(**pud).c.x, LSCAN_MANY);
			lscan_float(ln, &(**pud).c.y, LSCAN_MANY);
			lscan_float(ln, &(**pud).c.z, LSCAN_MANY);
		} else if (strcmp(cmd, "cx") == 0) {
			lscan_float(ln, &(**pud).c.x, LSCAN_MANY);
		} else if (strcmp(cmd, "cy") == 0) {
			lscan_float(ln, &(**pud).c.y, LSCAN_MANY);
		} else if (strcmp(cmd, "cz") == 0) {
			lscan_float(ln, &(**pud).c.z, LSCAN_MANY);
		} else if (strcmp(cmd, "d") == 0) {
			lscan_float(ln, &(**pud).d, LSCAN_MANY);
		} else if (strcmp(cmd, "k") == 0) {
			lscan_float(ln, &(**pud).k, LSCAN_MANY);
		} else if (strcmp(cmd, "phase") == 0) {
			lscan_float(ln, &(**pud).phase, LSCAN_MANY);
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

	for (size_t i = 0; i < ledn; i++) {
		const float phi = t * ud->k + vec3f_dot(&coords[ledv[i]].pos, &ud->c)
											+ i * ud->d + ud->phase;
		hsv((led_t *)leds + ledv[i], phi, 1, 1);
	}
}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		7,
		uidl_pair(
			"c",
			uidl_sequence(
				0,
				3,
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0))),
		uidl_pair("cx", uidl_float(0, 0, 0, 0)),
		uidl_pair("cy", uidl_float(0, 0, 0, 0)),
		uidl_pair("cz", uidl_float(0, 0, 0, 0)),
		uidl_pair("d", uidl_float(0, 0, 0, 0)),
		uidl_pair("k", uidl_float(0, 0, 0, 0)),
		uidl_pair("phase", uidl_float(0, 0, 0, 0))

	);
}