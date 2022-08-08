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

#ifndef UTIL_MODULE_HPP
#define UTIL_MODULE_HPP

#include "alpha4/common/logger.hpp"
#include "alpha4/types/token.hpp"
#include "core/module_api.h"
#include "types/stringlist.h"
#include <sstream>
struct Responder {
	response_type_t   type;
	std::string       source;
	std::stringstream ss;

	Responder(response_type_t type, const std::string &source) :
		type(type), source(source) {}

	template<typename T> Responder &operator<<(const T &v) {
		ss << v;
		return *this;
	}

	Responder &operator<<(const ::alp::over_t &) {
		command_respond(type, source.c_str(), ss.str().c_str());
		return *this;
	}
};

inline uidl_node_t *stringlist_to_idl(stringlist_t *list, bool destroy) {
	uidl_node_t *res = uidl_keyword(nullptr, 0);

	for (size_t i = 0; i < list->count; i++) {
		uidl_keyword_set(res, list->modules[i], 0);
	}

	if (destroy) { stringlist_free(list); }

	return res;
}

#define TOSTRING(x) TOSTRING2(x)
#define TOSTRING2(x) #x

#define RESPOND(type) \
	Responder(::response_type_t::type, __FILE__ TOSTRING(__LINE__))

#define MODULE_SAFECALL(loc, code)                                     \
	try {                                                                \
		code                                                               \
	} catch (const std::exception &e) {                                  \
		RESPOND(W) << "error in " << loc << ": " << e.what() << alp::over; \
	}

#endif