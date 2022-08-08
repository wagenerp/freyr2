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

#ifndef EGRESS_API_H
#define EGRESS_API_H

#include "core/frame_api.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "unicorn/idl.h"
#include "types/stringlist.h"
#include <stdint.h>
#include <stdlib.h>

typedef size_t egressno_t;
#define INVALID_EGRESS ((egressno_t)(0))

typedef struct egress_info_t {
	egressno_t egressno;
	led_i_t    offset;
	led_i_t    count;
} egress_info_t;

typedef void (*egress_init_f)(
	egressno_t egressno, const char *argstr, void **puserdata);
typedef void (*egress_deinit_f)(void *userdata);
typedef void (*egress_flush_t)(led_i_t firstLED, led_i_t count, void *userdata);

void egress_status();

stringlist_t *egress_list_get();

led_i_t egress_leds_added_count();
led_i_t egress_leds_removed_offset();
led_i_t egress_leds_removed_count();


egressno_t egress_init(
	const char * ident,
	const char * instanceName,
	const size_t count,
	const char * argstr);

egressno_t egress_find(const char *instanceName, egress_info_t *inf);
int        egress_info(egressno_t egress, egress_info_t *inf);
led_i_t    egress_offset(egressno_t egress);
void       egress_set_active(egressno_t egress, int active);


void egress_remove(egressno_t egress);
void egress_cleanup();

#ifdef __cplusplus
}
#endif

#endif