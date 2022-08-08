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
	float hue;
	float speed;
	float center;
	float size;
	float t0;
	float start;
} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud           = (ud_t *)malloc(sizeof(ud_t));
	(**pud).hue    = 0;
	(**pud).speed  = 1;
	(**pud).center = 1.3;
	(**pud).size   = 1.5;
	(**pud).start  = -1.2;
	(**pud).t0     = -1;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "hue") == 0) {
			lscan_float(ln, &(**pud).hue, LSCAN_MANY);
		} else if (strcmp(cmd, "speed") == 0) {
			lscan_float(ln, &(**pud).speed, LSCAN_MANY);
		} else if (strcmp(cmd, "center") == 0) {
			lscan_float(ln, &(**pud).center, LSCAN_MANY);
		} else if (strcmp(cmd, "size") == 0) {
			lscan_float(ln, &(**pud).size, LSCAN_MANY);
		} else if (strcmp(cmd, "start") == 0) {
			lscan_float(ln, &(**pud).start, LSCAN_MANY);
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

	if (ud->t0 < 0) { ud->t0 = t + ud->start; }

	float phase = fmod((t - ud->t0) * ud->speed, 1) * 3;

	for (size_t i = 0; i < ledn; i++) {
		float verticality = fabs(coords[ledv[i]].normal.y) > 0.9;

		if (verticality > 0.9) {
			hsv(leds + ledv[i], ud->hue, 1, (verticality - 0.9) / 0.5);
		} else {
			float p = fabs(coords[ledv[i]].pos.y - ud->center) / ud->size + phase;
			// float p = phase;
			if ((p < 1) || (p > 2)) {
				hsv(leds + ledv[i], ud->hue, 1, 0);
			} else {
				p = (1.5 - p) * 2;
				p = 1 - p * p;
				hsv(leds + ledv[i], ud->hue, 1, p * p * p * p);
			}
		}
	}
}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		5,
		uidl_pair("hue", uidl_float(0, 0, 0, 0)),
		uidl_pair("speed", uidl_float(0, 0, 0, 0)),
		uidl_pair("center", uidl_float(0, 0, 0, 0)),
		uidl_pair("size", uidl_float(0, 0, 0, 0)),
		uidl_pair("start", uidl_float(0, 0, 0, 0))

	);
}