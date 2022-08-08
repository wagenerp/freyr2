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
#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/module_api.h"
#include "util/egress.h"
#include <stdio.h>
#include <string.h>

typedef struct userdata_t {
	unsigned width;
	FILE *   f;
} userdata_t;

void init(egressno_t, const char *argstr [[maybe_unused]], userdata_t **pud) {
	*pud          = (userdata_t *)malloc(sizeof(userdata_t));
	(**pud).width = 32;
	(**pud).f     = stdout;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;
		if (strcmp(cmd, "width") == 0) {
			lscan_uint(ln, &(**pud).width, LSCAN_MANY);
		} else if (strcmp(cmd, "path") == 0) {
			char *fn = lscan_str(ln, LSCAN_MANY, 0);
			if (0 != fn) {
				(**pud).f = fopen(fn, "a");
				free(fn);
			}
		}

		free(cmd);
	}

	fprintf((**pud).f, "\x1b[2J\x1b[H\x1b[3J");

	lscan_free(ln);
}

void deinit(userdata_t *userdata) { free((void *)userdata); }

void flush(
	led_i_t     firstLED [[maybe_unused]],
	led_i_t     count [[maybe_unused]],
	userdata_t *userdata [[maybe_unused]]) {
	led_t *        led   = frame_raw_egress() + firstLED;
	const unsigned width = userdata->width;
	fprintf(userdata->f, "\x1b[H");
	for (led_i_t i = 0; i < count; i++) {
		fprintf(
			userdata->f,
			"\x1b[48;2;%u;%u;%um ",
			ctou8(led->r),
			ctou8(led->g),
			ctou8(led->b));
		if (((i + 1) % (width)) == 0) fprintf(userdata->f, "\x1b[40;0m\r\n");
		led++;
	}
	fflush(userdata->f);
}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		2,
		uidl_pair("width", uidl_integer(0, UIDL_LIMIT_LOWER, 1, 0)),
		uidl_pair("path", uidl_string(0, 0))

	);
}
