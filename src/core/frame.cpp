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



#include "frame.hpp"

#include "alpha4/common/logger.hpp"
#include "alpha4/types/token.hpp"
#include "core/frame_api.h"

#include <iostream>
#include <vector>

static std::vector<led_t> _frame_preanim;
static std::vector<led_t> _frame_anim;
static std::vector<led_t> _frame_egress;

void Frame::LEDsAdded(led_i_t count) {
	_frame_preanim.resize(_frame_preanim.size() + count, {0, 0, 0});
}
void Frame::LEDsRemoved(led_i_t offset, led_i_t count) {
	if (offset >= _frame_preanim.size()) return;
	if (offset + count > _frame_preanim.size())
		count = _frame_preanim.size() - offset;

	_frame_preanim.erase(
		_frame_preanim.begin() + offset, _frame_preanim.begin() + (offset + count));
}

void Frame::FlushAnim() { _frame_anim = _frame_preanim; }
void Frame::FlushEgress() {
	_frame_egress  = _frame_anim;
	_frame_preanim = _frame_anim;
}

extern "C" {

size_t frame_size() { return _frame_preanim.size(); }

led_t *frame_raw_preanim() { return _frame_preanim.data(); }
led_t *frame_raw_anim() { return _frame_anim.data(); }
led_t *frame_raw_egress() { return _frame_egress.data(); }
}