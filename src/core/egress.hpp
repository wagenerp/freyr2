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

#ifndef CORE_EGRESS_HPP
#define CORE_EGRESS_HPP
#include "alpha4/common/error.hpp"
#include "core/basemodule_api.h"
#include "core/egress_api.h"
#include "core/frame_api.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class EgressInstance {
protected:
	egressno_t      _egressno;
	std::string     _ident;
	basemodno_t     _basemodno;
	egress_init_f   _init;
	egress_deinit_f _deinit;
	egress_flush_t  _flush;

	void *_userdata = nullptr;

	led_i_t     _count;
	std::string _instanceName;

public:
	static void Flush();

public:
	bool active = true;

	EgressInstance(
		const std::string &ident,
		basemodno_t        basemodno,
		led_i_t            count,
		const std::string &argstring,
		const std::string &instanceName);
	EgressInstance(const EgressInstance &) = delete;
	EgressInstance(EgressInstance &&)      = default;
	~EgressInstance();

	egressno_t         egressno() const { return _egressno; }
	const std::string &ident() const { return _ident; }
	basemodno_t        basemodno() const { return _basemodno; }
	egress_init_f      init() const { return _init; }
	egress_deinit_f    deinit() const { return _deinit; }
	egress_flush_t     flush() const { return _flush; }

	void *userdata() const { return _userdata; }

	led_i_t count() const { return _count; }

	const std::string &instanceName() const { return _instanceName; }
};

#endif