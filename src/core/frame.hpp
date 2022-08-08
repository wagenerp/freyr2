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

#ifndef CORE_FRAME_HPP
#define CORE_FRAME_HPP

#include "core/frame_api.h"
class Frame {
public:
	static void LEDsAdded(led_i_t count);
	static void LEDsRemoved(led_i_t offset, led_i_t count);
	static void FlushAnim();
	static void FlushEgress();
};

#endif