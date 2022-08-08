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

#include "basemodule.hpp"

#include "alpha4/common/logger.hpp"
#include "core/basemodule_api.h"
#include "util/module.hpp"
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#ifdef ENABLE_DYNAMIC_MODULES
#include "core/animation.hpp"
#include "core/egress.hpp"
#include "core/module.hpp"
#include <dlfcn.h>
#else
#endif

static std::unordered_map<basemodno_t, std::shared_ptr<BaseModule>> _ModuleMap;
static basemodno_t _ModuleNumberHigh = 0;

#ifdef ENABLE_DYNAMIC_MODULES
static std::unordered_map<std::string, std::filesystem::path> _DynamicRegistry;
#endif
#ifdef ENABLE_DYNAMIC_MODULES
void BaseModule::DefineSymbol(
	const std::string &, const std::string &, void *) {}
#else
static std::unordered_map<std::string, std::unordered_map<std::string, void *>>
		 _SymbolTable;
void BaseModule::DefineSymbol(
	const std::string &idModule, const std::string &idSymbol, void *loc) {
	_SymbolTable[idModule][idSymbol] = loc;
}
#endif

#ifdef ENABLE_DYNAMIC_MODULES

void BaseModule::ScanDirectory(const std::filesystem::path &fn) {
	if (!std::filesystem::exists(fn)) return;
	if (!std::filesystem::is_directory(fn)) return;
	LOG(I) << "scanning animation directory " << fn << alp::over;
	for (std::filesystem::directory_iterator it(fn), end; it != end; ++it) {
		if (!it->is_regular_file()) continue;
		if (it->path().extension().string() != ".so") continue;
		auto ident              = it->path().stem().string();
		_DynamicRegistry[ident] = it->path();
	}
}

#endif

BaseModule::BaseModule(const std::string &idModule) : _idModule(idModule) {
#ifdef ENABLE_DYNAMIC_MODULES

	auto it = _DynamicRegistry.find(idModule);
	if (it == _DynamicRegistry.end()) {
		alp::thrower<ModuleLoadError>()
			<< "module " << idModule << " not found" << alp::over;
	}

	// unsigned flags = RTLD_LOCAL | RTLD_LAZY;
	// if (idModule=="mod_coordinates") flags = RTLD_GLOBAL | RTLD_LAZY;
	// if (idModule=="mod_grouping") flags = RTLD_GLOBAL | RTLD_LAZY;
	std::string name = it->second.filename();
	_hlib            = dlopen(
    it->second.c_str(),
    (idModule.substr(0, 4) == "mod_") ? (RTLD_GLOBAL | RTLD_LAZY)
                                      : (RTLD_LOCAL | RTLD_LAZY));
	if (!_hlib) {
		alp::thrower<ModuleLoadError>()
			<< "module " << idModule << " (" << it->second
			<< ") load error: " << dlerror() << alp::over;
	}
#endif

	_basemodno = ++_ModuleNumberHigh;

	_describe = reinterpret_cast<module_describe_f>(resolveFunction("describe"));
}

BaseModule::~BaseModule() {
#ifdef ENABLE_DYNAMIC_MODULES
	if (_hlib) { dlclose(_hlib); }
#endif
}

void BaseModule::grab() { _usageCount++; }
bool BaseModule::drop() {
	if (_usageCount < 2) {
		_usageCount = 0;
		return true;
	}
	_usageCount--;
	return false;
}

void *BaseModule::resolveFunction(
	const std::string &ident, bool silent [[maybe_unused]]) const {
#ifdef ENABLE_DYNAMIC_MODULES
	void *raw = dlsym(_hlib, ident.c_str());
	if (char *error = dlerror(); nullptr != error) {
		if (!silent) {
			alp::thrower<ModuleLoadError>()
				<< "unable to resolve module " << _idModule << " function " << ident
				<< ": " << error << alp::over;
		}
		return nullptr;
	}
	return raw;
#else
	if (auto itMod = _SymbolTable.find(_idModule); itMod != _SymbolTable.end()) {
		auto &mod = itMod->second;
		if (auto itFunc = mod.find(ident); itFunc != mod.end()) {
			return itFunc->second;
		}
	}
	return nullptr;
#endif
}

extern "C" {

#ifdef ENABLE_DYNAMIC_MODULES
void scan_module_directory(const char *fn) { BaseModule::ScanDirectory(fn); }
#endif

void basemodule_status() {
	auto msg =
		std::move(RESPOND(I) << "basemodules: " << _ModuleMap.size() << "\n");
	for (auto it : _ModuleMap) {
		msg << "  #" << it.first << " (" << it.second->idModule()
				<< ") - uc:" << it.second->usageCount() << "\n";
	}
	msg << alp::over;
}

stringlist_t *basemodule_list_get() {
	auto res = stringlist_new();
#ifdef ENABLE_DYNAMIC_MODULES
	res->count = _DynamicRegistry.size();
	if (res->count > 0) {
		res->modules = (char **)malloc(res->count * sizeof(char *));
		char **p     = res->modules;
		for (auto it : _DynamicRegistry) {
			*p++ = strdup(it.first.c_str());
		}
	} else {
		res->modules = nullptr;
	}
#else
	res->count = _SymbolTable.size();
	if (res->count > 0) {
		res->modules = (char **)malloc(res->count * sizeof(char *));
		char **p     = res->modules;
		for (auto it : _SymbolTable) {
			*p++ = strdup(it.first.c_str());
		}
	} else {
		res->modules = nullptr;
	}
#endif
	return res;
}

basemodno_t basemodule_init(const char *basename) {
	std::shared_ptr<BaseModule> mod = nullptr;
	for (auto it : _ModuleMap) {
		if (it.second->idModule() == basename) return it.second->basemodno();
	}

	try {
		mod = std::make_shared<BaseModule>(basename);
	} catch (const ModuleLoadError &e) {
		LOG(E) << "error registering class module " << basename << ": " << e.what()
					 << alp::over;
		return INVALID_BASEMOD;
	}

	_ModuleMap[mod->basemodno()] = mod;
	return mod->basemodno();
}

void basemodule_grab(basemodno_t basemodno) {
	auto it = _ModuleMap.find(basemodno);
	if (it == _ModuleMap.end()) return;
	it->second->grab();
}
void basemodule_drop(basemodno_t basemodno) {
	auto it = _ModuleMap.find(basemodno);
	if (it == _ModuleMap.end()) return;
	if (it->second->drop()) {
		LOG(I) << "dropping unused base module " << it->second->idModule()
					 << alp::over;
		_ModuleMap.erase(it);
	}
}
uidl_node_t *basemodule_describe(basemodno_t basemodno) {
	auto it = _ModuleMap.find(basemodno);
	if (it == _ModuleMap.end()) return nullptr;

	if (!it->second->describe()) return nullptr;
	return it->second->describe()();
}
const char *basemodule_name(basemodno_t basemodno) {
	auto it = _ModuleMap.find(basemodno);
	if (it == _ModuleMap.end()) return nullptr;

	return it->second->idModule().c_str();
}
void *basemodule_resolve(basemodno_t basemodno, const char *idSymbol) {
	auto it = _ModuleMap.find(basemodno);
	if (it == _ModuleMap.end()) return nullptr;

	return it->second->resolveFunction(idSymbol, true);
}
}
