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
#include "alpha4/types/vector.hpp"
#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/module_api.h"
#include "modules/coordinates_api.h"
#include "util/module.hpp"
#include <cstdlib>
#include <stdio.h>
#include <vector>

static std::vector<led_coord_data_t> _Coordinates_preanim;
static std::vector<led_coord_data_t> _Coordinates_anim;

extern "C" {

modno_t SingletonInstance = INVALID_MODULE;

static void _cmd_coordinates_set(modno_t, const char *argstr, void *);

static void _hook_ledsAdded(hook_t, modno_t, void *) {
	_Coordinates_preanim.resize(
		_Coordinates_preanim.size() + egress_leds_added_count(),
		{{0, 0, 0}, {0, 0, 0}});
}
static void _hook_ledsRemoved(hook_t, modno_t, void *) {
	_Coordinates_preanim.erase(
		_Coordinates_preanim.begin() + egress_leds_removed_offset(),
		_Coordinates_preanim.begin()
			+ (egress_leds_removed_offset() + egress_leds_removed_count()));
}

void init(modno_t modno, const char *, void **) {
	module_register_command(
		modno, "coordinates_set", _cmd_coordinates_set, nullptr);
	module_hook(modno, hook_resolve("ledsAdded"), _hook_ledsAdded);
	module_hook(modno, hook_resolve("ledsRemoved"), _hook_ledsRemoved);
}
void deinit(modno_t, void *) {
	_Coordinates_preanim.clear();
	_Coordinates_anim.clear();
}
void flush(modno_t, void *) { _Coordinates_anim = _Coordinates_preanim; }

led_coord_data_t *coordinates_raw_preanim() {
	return _Coordinates_preanim.data();
}
const led_coord_data_t *coordinates_raw_anim() {
	return _Coordinates_anim.data();
}

static void _cmd_coordinates_set(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);

	std::string egressName;
	size_t      offset =0;

	if (!ln.getAll(egressName, offset)) {
		RESPOND(E)
			<< "incomplete coordinates_set command - missing egress module name "
				 "or offset"
			<< alp::over;
		return;
	}

	if (egressName.empty()) {
	} else if (egressno_t egressno = egress_find(egressName.c_str(), nullptr);
						 egressno != INVALID_EGRESS) {
		offset += egress_offset(egressno);
	} else {
		RESPOND(E) << "egress instance '" << egressName << "' does not exist"
							 << alp::over;
		return;
	}

	const size_t     end = frame_size();
	led_coord_data_t tmp;

	for (size_t i = offset; i < end; i++) {
		if (!ln.get(tmp.pos.x)) break;
		if (!ln.getAll(
					tmp.pos.y, tmp.pos.z, tmp.normal.x, tmp.normal.y, tmp.normal.z)) {
			RESPOND(E) << "error decoding pos / normal - incomplete command?"
								 << alp::over;
			return;
		}

		_Coordinates_preanim[i] = tmp;
	}
}

uidl_node_t *_desc_module_remove(void *) {
	auto mods = egress_list_get();

	uidl_node_t *idents = uidl_keyword(nullptr, 0);
	for (size_t i = 0; i < mods->count; i++) {
		uidl_keyword_set(idents, mods->modules[i], 0);
	}
	stringlist_free(mods);

	return uidl_sequence(
		0,
		3,
		idents,
		uidl_integer(0, UIDL_LIMIT_LOWER, 0, 0),
		uidl_repeat(
			0,
			uidl_sequence(
				0,
				3,
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0)),
			0)

	);
}
}