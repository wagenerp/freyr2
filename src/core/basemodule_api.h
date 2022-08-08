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

#ifndef BASEMODULE_API_H
#define BASEMODULE_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "unicorn/idl.h"
#include "types/stringlist.h"

#if defined(__linux__) && defined(OPT_DYNAMIC)
#define ENABLE_DYNAMIC_MODULES
#endif

#ifdef ENABLE_DYNAMIC_MODULES
void basemodule_scan(const char *fn);
#endif

typedef size_t basemodno_t;
#define INVALID_BASEMOD ((basemodno_t)0)

typedef uidl_node_t *(*module_describe_f)();

void basemodule_status();

stringlist_t *basemodule_list_get();

basemodno_t  basemodule_init(const char *basename);
void         basemodule_grab(basemodno_t basemodno);
void         basemodule_drop(basemodno_t basemodno);
uidl_node_t *basemodule_describe(basemodno_t basemodno);
const char * basemodule_name(basemodno_t basemodno);
void *       basemodule_resolve(basemodno_t basemodno, const char *idSymbol);

#ifdef __cplusplus
}
#endif

#endif