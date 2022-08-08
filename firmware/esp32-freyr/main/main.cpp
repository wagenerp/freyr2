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

#include "esp_event.h"
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

void webif_init();
void freyr_init();

extern "C" {

extern const char *_binary_index_html_gz_start;
extern const char *_binary_index_html_gz_end;

void app_main() {
	ESP_LOGI("main", "freyr firmware compiled at " __DATE__ " " __TIME__);

	nvs_flash_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	webif_init();
	freyr_init();
}
}
