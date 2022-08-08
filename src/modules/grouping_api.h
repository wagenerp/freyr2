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

#ifndef GROUPING_API_H
#define GROUPING_API_H

#include "core/animation_api.h"
#include "types/stringlist.h"
#ifdef __cplusplus
extern "C" {
#endif

stringlist_t *group_list_get();

led_i_t group_get(const char *ident, const led_i_t **pbuffer);
void    group_set(const char *ident, led_i_t first, led_i_t count);

#ifdef __cplusplus
}
#endif

#endif
