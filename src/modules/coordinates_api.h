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

#ifndef COORDINATES_API_H
#define COORDINATES_API_H

#include "alpha4c/types/vector.h"
#include "core/module_api.h"

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef float   coord_c_t;
typedef vec3f_t coord_t;

typedef struct led_coord_data_t {
	coord_t pos;
	coord_t normal;
} led_coord_data_t;

led_coord_data_t *      coordinates_raw_preanim();
const led_coord_data_t *coordinates_raw_anim();

#ifdef __cplusplus
}
#endif

#endif
