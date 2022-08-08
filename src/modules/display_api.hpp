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

#ifndef DISPLAY_API_HPP
#define DISPLAY_API_HPP

#include "alpha4/common/linescanner.hpp"
#include "alpha4/types/vector.hpp"
#include "core/ledset.hpp"
#include "util/module.hpp"
extern "C" {
#include "unicorn/idl.h"
#include "coordinates_api.h"
#include "grouping_api.h"
}

inline bool display_processSelector(LEDSet &leds, alp::LineScanner &ln) {
	std::string cmd;
	if (!ln.get(cmd)) {
		RESPOND(E) << "incomplete led selector - expected expression" << alp::over;
		return false;
	}
	{
		auto guard = leds.beginModification();

		if (cmd == "all") {
			leds.append((led_i_t)0, (led_i_t)frame_size());
		} else if (cmd == "voxel") {
			typedef alp::Vector<3, coord_c_t> vec_t;
			vec_t                             center, extent;
			if (!ln.getAll(center.x(), center.y(), center.z(), extent.x())) {
				RESPOND(E) << "missing center / extent for voxel selector" << alp::over;
				return false;
			}
			if (ln.get(extent.y())) {
				if (!ln.get(extent.z())) {
					RESPOND(E) << "incomplete extent for voxel selector" << alp::over;
					return false;
				}

			} else {
				extent.y() = extent.x();
				extent.z() = extent.x();
			}

			struct coords_ext_t {
				vec_t pos;
				vec_t normal;
			};

			const coords_ext_t *coords =
				(const coords_ext_t *)coordinates_raw_preanim();

			for (size_t i = 0, e = frame_size(); i < e; i++) {
				if (((coords[i].pos - center).abs() - extent).count_positive() < 1) {
					leds.append((led_i_t)i, 1);
				}
			}

		} else {
			const led_i_t *ptr = nullptr;
			size_t         n   = group_get(cmd.c_str(), &ptr);
			if (nullptr == ptr) {
				RESPOND(E) << "group '" << cmd << "' does not exist" << alp::over;
				return false;
			}
			leds.append(ptr, n);
		}
	}

	return true;
}

inline uidl_node_t *display_describeSelector(const char *ident) {
	uidl_node_t *res = uidl_keyword(
		ident,
		2,
		uidl_pair("all", 0),
		uidl_pair(
			"voxel",
			uidl_sequence(
				0,
				6,
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0),
				uidl_float(0, 0, 0, 0)))

	);

	auto groups = group_list_get();

	for (size_t i = 0; i < groups->count; i++) {
		uidl_keyword_set(res, groups->modules[i], 0);
	}

	stringlist_free(groups);

	return res;
}

#endif