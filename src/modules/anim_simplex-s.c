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
#include "modulation_scalar.h"
#include "modules/coordinates_api.h"

typedef struct ud_t {
	int   sizex, sizey, sizez;
	float distx, disty, distz;

	modulation_scalar_t modulation_scalar;

	double interval;

	double t0;
	double t0_actual;

	double scaling_factor;

	float *values;

} ud_t;

ALPHA4C_INLINE(size_t n_values)(const ud_t *ud) {
	return (ud->sizex) * (ud->sizey) * (ud->sizez) * 2;
}

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud = (ud_t *)malloc(sizeof(ud_t));
	modulation_scalar_init(&(**pud).modulation_scalar);
	(**pud).sizex          = 32;
	(**pud).sizey          = 32;
	(**pud).sizez          = 32;
	(**pud).distx          = 0.25;
	(**pud).disty          = 0.25;
	(**pud).distz          = 0.25;
	(**pud).interval       = 1.2;
	(**pud).t0             = 0;
	(**pud).t0_actual      = 0;
	(**pud).scaling_factor = 1;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (modulation_scalar_config(&(**pud).modulation_scalar, cmd, ln)) {
		} else if (strcmp(cmd, "dist") == 0) {
			lscan_float(ln, &(**pud).distx, LSCAN_MANY);
			(**pud).disty = (**pud).distx;
			(**pud).distz = (**pud).distx;
		} else if (strcmp(cmd, "distx") == 0) {
			lscan_float(ln, &(**pud).distx, LSCAN_MANY);
		} else if (strcmp(cmd, "disty") == 0) {
			lscan_float(ln, &(**pud).disty, LSCAN_MANY);
		} else if (strcmp(cmd, "distz") == 0) {
			lscan_float(ln, &(**pud).distz, LSCAN_MANY);
		} else if (strcmp(cmd, "size") == 0) {
			lscan_int(ln, &(**pud).sizex, LSCAN_MANY);
			(**pud).sizey = (**pud).sizex;
			(**pud).sizez = (**pud).sizex;
		} else if (strcmp(cmd, "sizex") == 0) {
			lscan_int(ln, &(**pud).sizex, LSCAN_MANY);
		} else if (strcmp(cmd, "sizey") == 0) {
			lscan_int(ln, &(**pud).sizey, LSCAN_MANY);
		} else if (strcmp(cmd, "sizez") == 0) {
			lscan_int(ln, &(**pud).sizez, LSCAN_MANY);
		}

		free(cmd);
	}
	lscan_free(ln);

	size_t ce_values = n_values(*pud);
	{
		(**pud).values = (float *)malloc(ce_values * sizeof(float));

		float *pv = (**pud).values;
		for (size_t i = 0; i < ce_values; i++) {
			pv[i] = randf();
		}
	}
}

void deinit(ud_t *ud) {
	free((void *)ud->values);
	free((void *)ud);
}

void iterate(
	const led_i_t *ledv, size_t ledn, ud_t *ud, frame_time_t, frame_time_t t) {
	led_t *                 leds   = frame_raw_anim();
	const led_coord_data_t *coords = coordinates_raw_anim();
	float *                 values = ud->values;

	{
		if (ud->scaling_factor > 1) {
			double dt = t - ud->t0_actual;
			ud->scaling_factor *= exp(-666 * dt * dt);
			if (ud->scaling_factor < 1) ud->scaling_factor = 1;
		}
		ud->t0_actual = t;

		if (t >= ud->t0 + ud->interval) {
			ud->t0 += ud->interval;
			if (t >= ud->t0 + ud->interval) { ud->t0 = t; }

			size_t ce_values = n_values(ud);
			memcpy(values, values + ce_values / 2, ce_values / 2 * sizeof(float));

			for (size_t i = ce_values / 2; i < ce_values; i++)
				values[i] = randf();
		}
	}

	for (size_t i = 0; i < ledn; i++) {
		led_t *                 led   = leds + ledv[i];
		const led_coord_data_t *coord = coords + ledv[i];

		int x0 = (((int)floor(coord->pos.x / ud->distx) % ud->sizex) + ud->sizex)
						 % ud->sizex;
		int y0 = (((int)floor(coord->pos.y / ud->disty) % ud->sizey) + ud->sizey)
						 % ud->sizey;
		int z0 = (((int)floor(coord->pos.z / ud->distz) % ud->sizez) + ud->sizez)
						 % ud->sizez;
		int   x1 = (x0 + 1) % ud->sizex;
		int   y1 = (y0 + 1) % ud->sizey;
		int   z1 = (z0 + 1) % ud->sizez;
		float fx = (coord->pos.x / ud->distx - floor(coord->pos.x / ud->distx));
		float fy = (coord->pos.y / ud->disty - floor(coord->pos.y / ud->disty));
		float fz = (coord->pos.z / ud->distz - floor(coord->pos.z / ud->distz));
		if (!(fx > 0)) fx = 0;
		if (!(fx < 1)) fx = 1;
		if (!(fy > 0)) fy = 0;
		if (!(fy < 1)) fy = 1;
		if (!(fz > 0)) fz = 0;
		if (!(fz < 1)) fz = 1;

		float ft = (t - ud->t0) / ud->interval;
		if (!(ft > 0)) ft = 0;
		if (!(ft < 1)) ft = 1;

#define V(a, b, c, t) \
	(values[(x##a) + ((y##b) + (t * ud->sizez + z##c) * ud->sizey) * ud->sizez])
		float v =
			(1 - ft)
				* ((1 - fz) * ((1 - fy) * ((1 - fx) * V(0, 0, 0, 0) + fx * V(1, 0, 0, 0)) + fy * ((1 - fx) * V(0, 1, 0, 0) + fx * V(1, 1, 0, 0))) + fz * ((1 - fy) * ((1 - fx) * V(0, 0, 1, 0) + fx * V(1, 0, 1, 0)) + fy * ((1 - fx) * V(0, 1, 1, 0) + fx * V(1, 1, 1, 0))))
			+ ft * ((1 - fz) * ((1 - fy) * ((1 - fx) * V(0, 0, 0, 1) + fx * V(1, 0, 0, 1)) + fy * ((1 - fx) * V(0, 1, 0, 1) + fx * V(1, 1, 0, 1))) + fz * ((1 - fy) * ((1 - fx) * V(0, 0, 1, 1) + fx * V(1, 0, 1, 1)) + fy * ((1 - fx) * V(0, 1, 1, 1) + fx * V(1, 1, 1, 1))));

#undef V

		modulation_scalar_apply(&ud->modulation_scalar, v, t, led);
	}
}

uidl_node_t *describe() {
	uidl_node_t *res = modulation_scalar_describe();
	uidl_keyword_set(res, "dist", uidl_float(0, 0, 0, 0));
	uidl_keyword_set(res, "distx", uidl_float(0, 0, 0, 0));
	uidl_keyword_set(res, "disty", uidl_float(0, 0, 0, 0));
	uidl_keyword_set(res, "distz", uidl_float(0, 0, 0, 0));
	uidl_keyword_set(res, "size", uidl_integer(0, 0, 0, 0));
	uidl_keyword_set(res, "sizex", uidl_integer(0, 0, 0, 0));
	uidl_keyword_set(res, "sizey", uidl_integer(0, 0, 0, 0));
	uidl_keyword_set(res, "sizez", uidl_integer(0, 0, 0, 0));
	return res;
}