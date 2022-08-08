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
#include "anim_common.h"

typedef enum sparkle_mode_t {
	FULL,
	LIMITED_HUE,
	LIMITED_INTENSITY,
	LIMITED_SATURATION,
	HUE,
	INTENSITY,
	SATURATION,
} sparkle_mode_t;

typedef struct ud_t {
	float hue;
	float saturation;
	float intensity;
	float frequency;
	float threshold;
	float deviation;
	int   mode;
	int   cycle_hue;
	int   base_color;
} ud_t;

void init(const led_i_t *, size_t, const char *argstr, ud_t **pud) {
	*pud = (ud_t *)malloc(sizeof(ud_t));

	(**pud).hue        = 0;
	(**pud).saturation = 1;
	(**pud).intensity  = 1;
	(**pud).frequency  = 10;
	(**pud).threshold  = 0.95;
	(**pud).deviation  = 0.1;
	(**pud).mode       = FULL;
	(**pud).cycle_hue  = 0;
	(**pud).base_color = 0;

	lscan_t *ln = lscan_new(argstr, 0);

	while (!lscan_eof(ln)) {
		char *cmd = lscan_str(ln, LSCAN_MANY, 0);
		if (0 == cmd) break;

		if (strcmp(cmd, "hue") == 0) {
			lscan_float(ln, &(**pud).hue, LSCAN_MANY);
		} else if (strcmp(cmd, "saturation") == 0) {
			lscan_float(ln, &(**pud).saturation, LSCAN_MANY);
		} else if (strcmp(cmd, "intensity") == 0) {
			lscan_float(ln, &(**pud).intensity, LSCAN_MANY);
		} else if (strcmp(cmd, "frequency") == 0) {
			lscan_float(ln, &(**pud).frequency, LSCAN_MANY);
		} else if (strcmp(cmd, "threshold") == 0) {
			lscan_float(ln, &(**pud).threshold, LSCAN_MANY);
		} else if (strcmp(cmd, "deviation") == 0) {
			lscan_float(ln, &(**pud).deviation, LSCAN_MANY);
		} else if (strcmp(cmd, "cycle") == 0) {
			lscan_int(ln, &(**pud).cycle_hue, LSCAN_MANY);
		} else if (strcmp(cmd, "base") == 0) {
			lscan_int(ln, &(**pud).base_color, LSCAN_MANY);
		} else if (strcmp(cmd, "full") == 0) {
			(**pud).mode = FULL;
		} else if (strcmp(cmd, "limhue") == 0) {
			(**pud).mode      = LIMITED_HUE;
			(**pud).deviation = 30;
		} else if (strcmp(cmd, "limint") == 0) {
			(**pud).mode = LIMITED_INTENSITY;
		} else if (strcmp(cmd, "limsat") == 0) {
			(**pud).mode = LIMITED_SATURATION;
		} else if (strcmp(cmd, "huemode") == 0) {
			(**pud).mode = HUE;
		} else if (strcmp(cmd, "int") == 0) {
			(**pud).mode = INTENSITY;
		} else if (strcmp(cmd, "sat") == 0) {
			(**pud).mode = SATURATION;
		}

		free(cmd);
	}
	lscan_free(ln);
}

void deinit(ud_t *ud) { free((void *)ud); }

void iterate(
	const led_i_t *ledv, size_t ledn, ud_t *ud, frame_time_t, frame_time_t t) {
	led_t *leds = frame_raw_anim();

	float hue = ud->cycle_hue ? ud->hue + ud->frequency * t : ud->hue;

	for (size_t i = 0; i < ledn; i++) {
		led_t *led = leds + ledv[i];

		if (randf() > ud->threshold) {
			switch (ud->mode) {
				case FULL: hsv(led, randf() * 360, 1, 1); break;
				case LIMITED_HUE:
					hsv(
						led,
						hue + (randf() - 0.5) * 2 * ud->deviation,
						ud->saturation,
						ud->intensity);
					break;
				case LIMITED_INTENSITY:
					hsv(
						led,
						hue,
						ud->saturation,
						ud->intensity + (randf() - 0.5) * 2 * ud->deviation);
					break;
				case LIMITED_SATURATION:
					hsv(
						led,
						hue,
						ud->saturation + (randf() - 0.5) * 2 * ud->deviation,
						ud->intensity);
					break;
				case HUE: hsv(led, randf() * 360, ud->saturation, ud->intensity); break;
				case INTENSITY: hsv(led, hue, ud->saturation, randf()); break;
				case SATURATION: hsv(led, hue, randf(), ud->intensity); break;
			}
		} else if (ud->base_color) {
			hsv(led, hue, ud->saturation, ud->intensity);
		} else {
			led->r = led->g = led->b = 0;
		}
	}


}

uidl_node_t *describe() {
	return uidl_keyword(
		0,
		13,
		uidl_pair("hue", uidl_float(0, 0, 0, 0)),
		uidl_pair("saturation", uidl_float(0, 0, 0, 0)),
		uidl_pair("intensity", uidl_float(0, 0, 0, 0)),
		uidl_pair("frequency", uidl_float(0, 0, 0, 0)),
		uidl_pair("threshold", uidl_float(0, 0, 0, 0)),
		uidl_pair("deviation", uidl_float(0, 0, 0, 0)),

		uidl_pair("full", 0),
		uidl_pair("limhue", 0),
		uidl_pair("limint", 0),
		uidl_pair("limsat", 0),
		uidl_pair("huemode", 0),
		uidl_pair("int", 0),
		uidl_pair("sat", 0)

	);
}