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



#include "alpha4/common/linescanner.hpp"
#include "alpha4/common/logger.hpp"
#include "alpha4/types/token.hpp"
#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/ledset.hpp"
#include "core/module_api.h"
#include "modules/coordinates_api.h"
#include "types/stringlist.h"
#include "util/module.hpp"
#include <cstdlib>
#include <stdio.h>
#include <vector>

static std::unordered_map<std::string, LEDSet> _Groups;

extern "C" {

modno_t SingletonInstance = INVALID_MODULE;

static void _cmd_group_add(modno_t, const char *argstr, void *);
static void _cmd_group_remove(modno_t, const char *argstr, void *);
static void _cmd_group_clear(modno_t, const char *argstr, void *);

static void _removeLEDs(led_i_t offset, led_i_t count) {
	for (auto it = _Groups.begin(); it != _Groups.end();) {
		it->second.adjustRemovedLEDs(offset, count);
		if (it->second.empty()) {
			it = _Groups.erase(it);
		} else {
			++it;
		}
	}
}

static void _hook_ledsRemoved(hook_t, modno_t, void *) {
	_removeLEDs(egress_leds_removed_offset(), egress_leds_removed_count());
}

void init(modno_t modno, const char *, void **) {
	module_register_command(modno, "group_add", _cmd_group_add, nullptr);
	module_register_command(modno, "group_remove", _cmd_group_remove, nullptr);
	module_register_command(modno, "group_clear", _cmd_group_clear, nullptr);
	module_hook(modno, hook_resolve("ledsRemoved"), _hook_ledsRemoved);
}
void deinit(modno_t, void *) { _Groups.clear(); }
void flush(modno_t, void *) {}

stringlist_t *group_list_get() {
	auto res = stringlist_new();

	res->count = _Groups.size();
	if (res->count > 0) {
		res->modules = (char **)malloc(res->count * sizeof(char *));
		char **p     = res->modules;
		for (auto it : _Groups) {
			*p++ = strdup(it.first.c_str());
		}
	} else {
		res->modules = nullptr;
	}

	return res;
}

led_i_t group_get(const char *ident, const led_i_t **pbuffer) {
	if (auto it = _Groups.find(ident); it != _Groups.end()) {
		*pbuffer = it->second.data();
		return it->second.size();
	}
	return 0;
}

void group_set(const char *ident, led_i_t first, led_i_t count) {
	_Groups[ident].append(first, count);
}

void group_clear(const char *ident) {
	if (auto it = _Groups.find(ident); it != _Groups.end()) { _Groups.erase(it); }
}

static void _cmd_group_add(modno_t, const char *argstr, void *) {
	MODULE_SAFECALL("group_add", {
		alp::LineScanner::Call(
			[&](
				const std::string &idGroup,
				const char *       idEgress,
				led_i_t            first,
				led_i_t            count) -> void {
				egressno_t egress = egress_find(idEgress, nullptr);
				if (egress == INVALID_EGRESS) {
					RESPOND(E) << "cannot group LEDs - egress '" << idEgress
										 << "' not found" << alp::over;
					return;
				}

				first += egress_offset(egress);

				_Groups[idGroup].append(first, count);
			},
			argstr);
	});
}

static void _cmd_group_remove(modno_t, const char *argstr, void *) {
	MODULE_SAFECALL("group_remove", {
		alp::LineScanner::Call(
			[&](const char *idEgress, led_i_t first, led_i_t count) -> void {
				egressno_t egress = egress_find(idEgress, nullptr);
				if (egress == INVALID_EGRESS) {
					RESPOND(E) << "cannot group LEDs - egress '" << idEgress
										 << "' not found" << alp::over;
					return;
				}

				first += egress_offset(egress);

				_removeLEDs(first, count);
			},
			argstr);
	});
}

static void _cmd_group_clear(modno_t, const char *argstr, void *) {
	MODULE_SAFECALL(
		"group_clear", { alp::LineScanner::Call(group_clear, argstr); });
}
}