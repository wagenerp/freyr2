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

#include "alpha4c/common/math.h"
#include "alpha4c/types/vector.h"
#include "anim_common.h"
#include "modulation_scalar.h"
#include "modules/coordinates_api.h"

typedef struct ud_t {
	modulation_scalar_t modulation_scalar;
	int                 size;
	float               dist;

	double velocity;

	double t0;
	double t0_actual;

	vec3f_t center;

	float *values;

} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud = (ud_t *)malloc(sizeof(ud_t));
	modulation_scalar_init(&(**pud).modulation_scalar);
	(**pud).size      = 256;
	(**pud).dist      = 0.25;
	(**pud).velocity  = 0.2;
	(**pud).t0        = 0;
	(**pud).t0_actual = 0;
	(**pud).center    = vec3f_set(0.852, 2.154, 0.016);

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (modulation_scalar_config(&(**pud).modulation_scalar, cmd, ln)) {
		} else if (strcmp(cmd, "velocity") == 0) {
			lscan_double(ln, &(**pud).velocity, LSCAN_MANY);
		} else if (strcmp(cmd, "dist") == 0) {
			lscan_float(ln, &(**pud).dist, LSCAN_MANY);
		} else if (strcmp(cmd, "size") == 0) {
			lscan_int(ln, &(**pud).size, LSCAN_MANY);
		}

		free(cmd);
	}
	lscan_free(ln);

	{
		(**pud).values = (float *)malloc((**pud).size * sizeof(float));

		float *pv = (**pud).values;
		for (int i = 0; i < (**pud).size; i++) {
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
		if (ud->modulation_scalar.scaling_factor > 1) {
			double dt = t - ud->t0_actual;
			ud->modulation_scalar.scaling_factor *= exp(-666 * dt * dt);
			if (ud->modulation_scalar.scaling_factor < 1)
				ud->modulation_scalar.scaling_factor = 1;
		}
		ud->t0_actual = t;

		if (t >= ud->t0 + ud->dist / ud->velocity) {
			ud->t0 += ud->dist / ud->velocity;
			if (t >= ud->t0 + ud->dist / ud->velocity) { ud->t0 = t; }

			memmove(values + 1, values, sizeof(float) * (ud->size - 1));

			values[0] = randf();
		}
	}

	for (size_t i = 0; i < ledn; i++) {
		led_t *                 led   = leds + ledv[i];
		const led_coord_data_t *coord = coords + ledv[i];

		float stream_pos = 1;

		{
			vec3f_t tmp = vec3f_sub(&coord->pos, &ud->center);
			stream_pos += (vec3f_norm(&tmp) - ud->velocity * (t - ud->t0)) / ud->dist;
		}
		if (stream_pos < 0) stream_pos = 0;
		if (stream_pos >= ud->size) {
			led->r = led->g = led->b = 0;
			continue;
		}
		int   i0 = (int)stream_pos;
		float f  = stream_pos - i0;
		float v  = values[i0] * (1 - f) + values[i0 + 1] * f;
		modulation_scalar_apply(&ud->modulation_scalar, v, t, led);
	}
}

uidl_node_t *describe() {
	uidl_node_t *res = modulation_scalar_describe();
	uidl_keyword_set(res, "velocity", uidl_float(0, 0, 0, 0));
	uidl_keyword_set(res, "dist", uidl_float(0, 0, 0, 0));
	uidl_keyword_set(res, "size", uidl_integer(0, 0, 0, 0));
	return res;
}