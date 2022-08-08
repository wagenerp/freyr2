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

typedef struct ud_t {
	int      divisor;
	uint32_t seed0;
	uint32_t seed1;
	uint32_t seed2;
	uint32_t seed3;
	int      randomize;

	float fmin;
	float frange;
	float irange;
} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud              = (ud_t *)malloc(sizeof(ud_t));
	(**pud).divisor   = 4;
	(**pud).seed0     = rand();
	(**pud).randomize = 0;
	(**pud).fmin      = 0.3;
	(**pud).frange    = 0.2;
	(**pud).irange    = 0.2;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "divisor") == 0) {
			lscan_int(ln, &(**pud).divisor, LSCAN_MANY);
		} else if (strcmp(cmd, "seed") == 0) {
			lscan_uint(ln, &(**pud).seed0, LSCAN_MANY);
		} else if (strcmp(cmd, "fmin") == 0) {
			lscan_float(ln, &(**pud).fmin, LSCAN_MANY);
		} else if (strcmp(cmd, "frange") == 0) {
			lscan_float(ln, &(**pud).frange, LSCAN_MANY);
		} else if (strcmp(cmd, "irange") == 0) {
			lscan_float(ln, &(**pud).irange, LSCAN_MANY);
		} else if (strcmp(cmd, "randomize") == 0) {
			(**pud).randomize = 1;
		}

		free(cmd);
	}

	(**pud).seed1 = (**pud).seed0 * UINT32_C(1664525) + UINT32_C(1013904223);
	(**pud).seed2 = (**pud).seed1 * UINT32_C(1664525) + UINT32_C(1013904223);
	(**pud).seed3 = (**pud).seed2 * UINT32_C(1664525) + UINT32_C(1013904223);

	lscan_free(ln);
}

void deinit(ud_t *ud) { free((void *)ud); }

void iterate(
	const led_i_t *ledv, size_t ledn, ud_t *ud, frame_time_t, frame_time_t t) {
	led_t *leds = frame_raw_anim();

#define HASHF(seed)                                                \
	(((double)(uint32_t)(                                            \
		 ((uint32_t)i + (uint32_t)(ud->seed)) * UINT32_C(2654435761))) \
	 / (double)UINT32_C(0xffffffff))

	for (size_t i = 0; i < ledn; i++) {
		led_t *led = leds + ledv[i];

		uint32_t index = i;
		if (ud->randomize) {
			index = ((index + ud->seed0) * UINT32_C(2654435761)) / 10;
		}

		if ((index % ud->divisor) != 0) {
			led->r = led->g = led->b = 0;
			continue;
		}

		hsv(
			led,
			HASHF(seed1) * 360.0,
			1,
			1
				- ud->irange
						* (1 - 0.5 * cos(3.14159265 * 2 * (t * (HASHF(seed2) * ud->frange + ud->fmin) + HASHF(seed3)))));
	}
}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		6,
		uidl_pair("divisor", uidl_integer(0, 0, 0, 0)),
		uidl_pair("seed", uidl_integer(0, UIDL_LIMIT_LOWER, 0, 0)),
		uidl_pair("fmin", uidl_float(0, 0, 0, 0)),
		uidl_pair("frange", uidl_float(0, 0, 0, 0)),
		uidl_pair("irange", uidl_float(0, 0, 0, 0)),
		uidl_pair("randomize", 0)

	);
}