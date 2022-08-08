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

#ifndef CORE_BASEMODULE_HPP
#define CORE_BASEMODULE_HPP

#include "alpha4/common/error.hpp"
#include "alpha4/common/logger.hpp"
#include "basemodule_api.h"
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#ifdef ENABLE_DYNAMIC_MODULES
#include <filesystem>
#endif

class ModuleLoadError : public alp::Exception {
public:
	using alp::Exception::Exception;
	using alp::Exception::what;
};

class BaseModule {
public:
#ifdef ENABLE_DYNAMIC_MODULES
	static constexpr const bool EnableDynamic = true;
#else
	static constexpr const bool EnableDynamic = false;
#endif
protected:
#ifdef ENABLE_DYNAMIC_MODULES
	void *_hlib = nullptr;
#endif
	basemodno_t       _basemodno;
	std::string       _idModule;
	module_describe_f _describe   = nullptr;
	size_t            _usageCount = 0;

public:
	static void DefineSymbol(
		const std::string &idModule, const std::string &idSymbol, void *loc);

#ifdef ENABLE_DYNAMIC_MODULES
	static void ScanDirectory(const std::filesystem::path &fn);
#endif

public:
	BaseModule(const std::string &idModule);
	~BaseModule();

	size_t usageCount() const { return _usageCount; }
	void   grab();
	bool   drop();

	void *resolveFunction(const std::string &ident, bool silent = true) const;

	basemodno_t        basemodno() const { return _basemodno; }
	const std::string &idModule() const { return _idModule; }

	module_describe_f describe() const { return _describe; }
};

#endif