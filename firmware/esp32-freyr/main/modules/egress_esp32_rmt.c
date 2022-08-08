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

#include <freertos/FreeRTOS.h>

#include <freertos/task.h>

#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/module_api.h"
#include "esp_err.h"
#include "util/egress.h"
#include <alpha4c/common/linescanner.h>
#include <led_strip.h>

#define MAX_STRIPS 8
static led_strip_handle_t _Strips[MAX_STRIPS];
static led_strip_config_t _Configs[MAX_STRIPS];

static size_t _NStrips = 0;

color_c_t egress_esp32_rmt_brightness = 1.0;

static void _ReadConfig(lscan_t *ln) {
	_NStrips = MAX_STRIPS;
	for (unsigned i = 0; i < MAX_STRIPS; i++) {
		if (
			!lscan_ulong(ln, &_Configs[i].strip_gpio_num, LSCAN_MANY)
			|| !lscan_ulong(ln, &_Configs[i].max_leds, LSCAN_MANY)) {
			_NStrips = i;
			return;
		}
		ESP_ERROR_CHECK(led_strip_new_rmt_device(&_Configs[i], &_Strips[i]));
	}
}

void egress_esp32_rmt_init(
	egressno_t, const char *argstr [[maybe_unused]], void **) {
	lscan_t *ln = lscan_new(argstr, 0);
	_ReadConfig(ln);
	lscan_free(ln);
}

void egress_esp32_rmt_deinit(void *) {}

void egress_esp32_rmt_flush(
	led_i_t firstLED [[maybe_unused]], led_i_t count [[maybe_unused]], void *) {
	led_t *  led    = frame_raw_egress() + firstLED;
	unsigned iStrip = 0;
	while ((iStrip < _NStrips) && (count > 0)) {
		int lim = _Configs[iStrip].max_leds;
		if ((int)count < lim) lim = (int)count;

		uint32_t pix = 0;

		for (int i = 0; i < lim; i++) {
			led_strip_set_pixel(
				_Strips[iStrip],
				pix,
				ctou8(egress_esp32_rmt_brightness * led->r),
				ctou8(egress_esp32_rmt_brightness * led->g),
				ctou8(egress_esp32_rmt_brightness * led->b));
			pix++;
			led++;
		}
		iStrip++;
		count -= lim;
	}

	for (size_t i = 0; i < _NStrips; i++) {
		led_strip_refresh(_Strips[i]);
	}
}