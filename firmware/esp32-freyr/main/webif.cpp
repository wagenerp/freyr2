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

#include "freertos/FreeRTOS.h"

#include "freertos/task.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include <esp_http_server.h>
#include <string>

static httpd_handle_t _Server;

static std::string _AP_SSID;
static std::string _AP_PSK;

extern "C" {

void enqueue_command(const char *cmd, unsigned len);

const
#include "webif-index.c"
}

void webif_setup(const std::string &ssid, const std::string &psk) {
	_AP_SSID = ssid;
	_AP_PSK  = psk;
	if (_AP_SSID.size() > 31) _AP_SSID.resize(31);
	if (_AP_PSK.size() > 63) _AP_PSK.resize(63);
}

static void _Handle_event_wifi(
	void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t *event =
			(wifi_event_ap_staconnected_t *)event_data;
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t *event =
			(wifi_event_ap_stadisconnected_t *)event_data;
	}
}

static esp_err_t _Handle_get_index(httpd_req_t *req) {
	httpd_resp_set_hdr(req, "content-encoding", "gzip");

	httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
	return ESP_OK;
}

static esp_err_t _Handle_post_command(httpd_req_t *req) {
	char buf[512];
	int  res;

	res = httpd_req_recv(req, buf, sizeof(buf) - 1);
	if (res < 0) return ESP_FAIL;

	enqueue_command(buf, res);

	httpd_resp_send_chunk(req, NULL, 0);

	return ESP_OK;
}

static const httpd_uri_t _Handle_get_index_struct = {
	.uri = "/", .method = HTTP_GET, .handler = _Handle_get_index};
static const httpd_uri_t _Handle_post_command_struct = {
	.uri = "/", .method = HTTP_POST, .handler = _Handle_post_command};

void webif_init() {
	{ // wifi AP
		ESP_ERROR_CHECK(esp_netif_init());
		esp_netif_create_default_wifi_ap();

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		ESP_ERROR_CHECK(esp_wifi_init(&cfg));

		ESP_ERROR_CHECK(esp_event_handler_instance_register(
			WIFI_EVENT, ESP_EVENT_ANY_ID, &_Handle_event_wifi, NULL, NULL));

		wifi_config_t wifi_config = {
			.ap =
				{
					.ssid           = "",
					.password       = "",
					.ssid_len       = 9,
					.channel        = 0,
					.authmode       = WIFI_AUTH_WPA_WPA2_PSK,
					.ssid_hidden    = 1,
					.max_connection = 2,
					.pmf_cfg =
						{
							.required = false,
						},
				},
		};

		strcat((char *)&wifi_config.ap.ssid, _AP_SSID.c_str());
		strcat((char *)&wifi_config.ap.password, _AP_PSK.c_str());

		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
		ESP_ERROR_CHECK(esp_wifi_start());
	}

	{ // web server

		httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
		config.lru_purge_enable = true;
		if (httpd_start(&_Server, &config) == ESP_OK) {
			httpd_register_uri_handler(_Server, &_Handle_get_index_struct);
			httpd_register_uri_handler(_Server, &_Handle_post_command_struct);
		}
	}
}
