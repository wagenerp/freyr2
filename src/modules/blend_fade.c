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
#include "blend_api.h"
#include <string.h>

typedef struct ud_t {
	float tAnim;
	float speed;
} ud_t;

void init(const char *argstr, ud_t **pud) {
	*pud          = (ud_t *)malloc(sizeof(ud_t));
	(**pud).tAnim = 0;
	(**pud).speed = 1.0f;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "speed") == 0) {
			lscan_float(ln, &(**pud).speed, LSCAN_MANY);
		}

		free(cmd);
	}
	lscan_free(ln);
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
	ud->tAnim += dt * ud->speed;

	float f = fclamp(1.0f - ud->tAnim);
	float g = fclamp(ud->tAnim);

	for (size_t i = 0; i < ledn; i++) {
		accum[ledv[i]].r = op2[i].r * f + accum[ledv[i]].r * g;
		accum[ledv[i]].g = op2[i].g * f + accum[ledv[i]].g * g;
		accum[ledv[i]].b = op2[i].b * f + accum[ledv[i]].b * g;
	}

	return (ud->tAnim < 1) ? BLEND_ACTIVE : BLEND_DONE;
}

uidl_node_t *describe() {
	return uidl_keyword(
		0, 1, uidl_pair("speed", uidl_float(0, 0, 0, 0))

	);
}
