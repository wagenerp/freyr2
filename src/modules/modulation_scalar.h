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

#ifndef MODULATION_SCALAR_H
#define MODULATION_SCALAR_H

#include "alpha4c/common/math.h"
#include "unicorn/idl.h"

typedef enum modulation_scalar_mode_t {
	FIXED_HUE,
	HUE_CYCLE,
	HEATMAP,
	BUBBLEGUM,
	HUEMAP,
	DESATURATE,
	CYCLE_DESATURATE,
	DIRECT_HUE,
	DEVIATE,
	OVERLOAD,
	CYBERKING,
} modulation_scalar_mode_t;

typedef struct modulation_scalar_t {
	float  hue;
	float  saturation;
	float  intensity;
	float  frequency;
	int    mode;
	float  offset;
	double scaling_factor;
} modulation_scalar_t;

ALPHA4C_INLINE(void modulation_scalar_init)(modulation_scalar_t *mod) {
	mod->hue            = 0;
	mod->saturation     = 1;
	mod->intensity      = 0.5;
	mod->frequency      = 10;
	mod->mode           = BUBBLEGUM;
	mod->offset         = 0.5;
	mod->scaling_factor = 1;
}

ALPHA4C_INLINE(int modulation_scalar_config)
(modulation_scalar_t *mod, const char *cmd, lscan_t *ln) {
	if (strcmp(cmd, "cycle") == 0) {
		mod->mode = HUE_CYCLE;
	} else if (strcmp(cmd, "heatmap") == 0) {
		mod->mode = HEATMAP;
	} else if (strcmp(cmd, "bubblegum") == 0) {
		mod->mode = BUBBLEGUM;
	} else if (strcmp(cmd, "huemap") == 0) {
		mod->mode = HUEMAP;
	} else if (strcmp(cmd, "direct") == 0) {
		mod->mode = DIRECT_HUE;
	} else if (strcmp(cmd, "cycle-desaturate") == 0) {
		mod->mode = CYCLE_DESATURATE;
	} else if (strcmp(cmd, "desaturate") == 0) {
		mod->mode = DESATURATE;
	} else if (strcmp(cmd, "deviate") == 0) {
		mod->mode   = DEVIATE;
		mod->offset = 0.5;
	} else if (strcmp(cmd, "overload") == 0) {
		mod->mode      = OVERLOAD;
		mod->frequency = 1440;
	} else if (strcmp(cmd, "cyberking") == 0) {
		mod->mode = CYBERKING;
	} else if (strcmp(cmd, "hue") == 0) {
		lscan_float(ln, &mod->hue, LSCAN_MANY);
		mod->mode = FIXED_HUE;
	} else if (strcmp(cmd, "saturation") == 0) {
		lscan_float(ln, &mod->saturation, LSCAN_MANY);
	} else if (strcmp(cmd, "intensity") == 0) {
		lscan_float(ln, &mod->intensity, LSCAN_MANY);
	} else if (strcmp(cmd, "frequency") == 0) {
		lscan_float(ln, &mod->frequency, LSCAN_MANY);
	} else if (strcmp(cmd, "offset") == 0) {
		lscan_float(ln, &mod->offset, LSCAN_MANY);
	}
	return 0;
}

ALPHA4C_INLINE(void modulation_scalar_apply)
(modulation_scalar_t *ms, float v, float t, led_t *led) {
	v           = v - ms->offset;
	float v_raw = v;
	if (!(v > 0)) v = 0;
	v /= (1.0 - ms->offset);

	v *= ms->scaling_factor;
	if (v > 1) v = 1;

	switch (ms->mode) {
		case FIXED_HUE: hsv(led, ms->hue, ms->saturation, v * ms->intensity); break;
		case HUE_CYCLE:
			hsv(led, ms->frequency * t, ms->saturation, v * ms->intensity);
			break;
		case HEATMAP:
			hsv(led, (1 - v) * 240, ms->saturation, v * ms->intensity);
			break;
		case BUBBLEGUM:
			hsv(led, 360 - v * 240, ms->saturation, v * ms->intensity);
			break;
		case HUEMAP:
			if (v_raw < 0) {
				led->r = led->g = led->b = 0;
			} else {
				hsv(led, v * 360, ms->saturation, ms->intensity);
			}
			break;
		case DESATURATE:
			hsv(led, ms->hue, (1 - v) * ms->saturation, v * ms->intensity);
			break;
		case CYCLE_DESATURATE:
			hsv(led, ms->frequency * t, (1 - v) * ms->saturation, v * ms->intensity);
			break;
		case DIRECT_HUE: hsv(led, v * 360, ms->saturation, ms->intensity); break;
		case DEVIATE:
			hsv(led, ms->frequency * t + v_raw * 280, ms->saturation, ms->intensity);
			break;
		case OVERLOAD:
			if (v <= 0)
				hsv(led, 0, 0, 0);
			else
				hsv(led, v * 720 * 2, ms->saturation, ms->intensity);
			break;
		case CYBERKING: {
			v = v_raw + ms->offset;
			v = (v - 0.5) * 2;

			float offset1 = ms->offset / ms->scaling_factor * 0.5;
			if (v >= 0) {
				v = (v - offset1) / (1 - offset1) * ms->scaling_factor * ms->intensity;
				led->r = fclamp(3.3 * 4 * v);
				led->g = fclamp(0.1 * v);
				led->b = fclamp(1.2 * v);
			} else if (v < 0) {
				v = (-v - offset1) / (1 - offset1) * ms->scaling_factor * ms->intensity;
				led->r = fclamp(0.1 * v);
				led->g = fclamp(1.5 * 4 * v);
				led->b = fclamp(v * 3);
			}
		}
	}
}

ALPHA4C_INLINE(uidl_node_t *modulation_scalar_describe)() {
	return uidl_keyword(
		0,
		15,
		uidl_pair("cycle", 0),
		uidl_pair("heatmap", 0),
		uidl_pair("bubblegum", 0),
		uidl_pair("huemap", 0),
		uidl_pair("direct", 0),
		uidl_pair("cycle-desaturate", 0),
		uidl_pair("desaturate", 0),
		uidl_pair("deviate", 0),
		uidl_pair("overload", 0),
		uidl_pair("cyberking", 0),
		uidl_pair("hue", uidl_float(0, 0, 0, 0)),
		uidl_pair("saturation", uidl_float(0, 0, 0, 0)),
		uidl_pair("intensity", uidl_float(0, 0, 0, 0)),
		uidl_pair("frequency", uidl_float(0, 0, 0, 0)),
		uidl_pair("offset", uidl_float(0, 0, 0, 0))

	);
}
#endif