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

#ifndef CORE_MODULE_HPP
#define CORE_MODULE_HPP

#include "alpha4/common/error.hpp"
#include "core/basemodule_api.h"
#include "module_api.h"
#include <filesystem>
#include <memory>
#include <optional>

class Module {
protected:
	modno_t     _modno;
	basemodno_t _basemodno;
	std::string _ident;
	void *      _userdata = nullptr;

	module_init_f   _init;
	module_deinit_f _deinit;
	module_flush_f  _flush;
	modno_t *       _singletonInstance;

	std::optional<std::string> _instanceName;

public:
	static void Flush();

public:
	Module(
		const std::string &        ident,
		basemodno_t                basemodno,
		std::optional<std::string> instanceName);
	~Module();

	modno_t            modno() const { return _modno; }
	basemodno_t        basemodno() const { return _basemodno; }
	const std::string &ident() const { return _ident; }

	void *         userdata() const { return _userdata; }
	module_flush_f flush() const { return _flush; };
	const modno_t *singletonInstance() const { return _singletonInstance; }
	const std::optional<std::string> &instanceName() const {
		return _instanceName;
	}

	void init(const std::string &argstring);
};

#endif