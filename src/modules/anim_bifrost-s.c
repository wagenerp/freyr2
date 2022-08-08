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
	int   fixed_hue;
	int   global_hue;
	int   limit_intensity;
	float hue;
	float intensity;

	float cx, cy, cz;
	float fx, fy, fz;
} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud                    = (ud_t *)malloc(sizeof(ud_t));
	(**pud).fixed_hue       = 0;
	(**pud).global_hue      = 0;
	(**pud).limit_intensity = 0;
	(**pud).cx              = 0.976;
	(**pud).cy              = 1.119;
	(**pud).cz              = 0;
	(**pud).fx              = 1.2;
	(**pud).fy              = 2.3;
	(**pud).fz              = 0.5;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "hue") == 0) {
			char *value = lscan_str(ln, LSCAN_MANY, 0);
			if (value) {
				if (strcmp(value, "global") == 0) {
					(*pud)->global_hue = 1;
				} else {
					(*pud)->fixed_hue = 1;
					(*pud)->hue       = strtod(value, 0);
				}
				free(value);
			}
		} else if (strcmp(cmd, "intensity") == 0) {
			(**pud).limit_intensity = 1;
			lscan_float(ln, &(**pud).intensity, LSCAN_MANY);
		} else if (strcmp(cmd, "x") == 0) {
			lscan_float(ln, &(**pud).cx, LSCAN_MANY);
		} else if (strcmp(cmd, "y") == 0) {
			lscan_float(ln, &(**pud).cy, LSCAN_MANY);
		} else if (strcmp(cmd, "z") == 0) {
			lscan_float(ln, &(**pud).cz, LSCAN_MANY);
		} else if (strcmp(cmd, "fx") == 0) {
			lscan_float(ln, &(**pud).fx, LSCAN_MANY);
		} else if (strcmp(cmd, "fy") == 0) {
			lscan_float(ln, &(**pud).fy, LSCAN_MANY);
		} else if (strcmp(cmd, "fz") == 0) {
			lscan_float(ln, &(**pud).fz, LSCAN_MANY);
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

	vec3f_t center = {ud->cx, ud->cy, ud->cz};
	vec3f_t dir    = {cos(t * ud->fx), cos(t * ud->fy), cos(t * ud->fz)};

	dir = vec3f_normal(&dir);

	{
		vec3f_t tmp = vec3f_mul(&dir, 0.65f);
		center      = vec3f_add(&center, &tmp);
	}
	float d;
	for (size_t i = 0; i < ledn; i++) {
		led_t *                 led   = leds + ledv[i];
		const led_coord_data_t *coord = coords + ledv[i];

		vec3f_t tmp = vec3f_sub(&coord->pos, &center);
		d           = vec3f_dot(&tmp, &dir);

		int idx = (int)(d * (1.0 / 0.03));

		{ // congress modulation

			double period = 5;
			double phase_pre, phase;

			phase_pre = t * 4 + idx * 0.3;
			phase     = fmod(phase_pre, period) / period;

			phase = phase * 7 - 3;
			if ((phase < 0) || (phase > 1)) {
				led->r = led->g = led->b = 0;

			} else {
				double intensity = 1 - fabs(0.5 - phase) * 2;
				double hue;

				if (ud->global_hue) {
					hue = t * 10;
				} else if (ud->fixed_hue) {
					hue = ud->hue;
				} else {
					hue = t * 10 + idx * 4;
				}

				if (ud->limit_intensity) intensity *= ud->limit_intensity;

				hsv(led, hue, 1, intensity);
			}
		}
	}
}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		8,
		uidl_pair("hue", uidl_float(0, 0, 0, 0)),
		uidl_pair("intensity", uidl_float(0, 0, 0, 0)),
		uidl_pair("x", uidl_float(0, 0, 0, 0)),
		uidl_pair("y", uidl_float(0, 0, 0, 0)),
		uidl_pair("z", uidl_float(0, 0, 0, 0)),
		uidl_pair("fx", uidl_float(0, 0, 0, 0)),
		uidl_pair("fy", uidl_float(0, 0, 0, 0)),
		uidl_pair("fz", uidl_float(0, 0, 0, 0))

	);
}