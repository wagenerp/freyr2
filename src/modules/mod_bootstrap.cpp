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
#include "core/animation_api.h"
#include "core/basemodule_api.h"
#include "core/egress_api.h"
#include "core/module_api.h"
#include "types/stringlist.h"
#include "util/module.hpp"
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <string>

extern "C" {
modno_t SingletonInstance = INVALID_MODULE;

#ifdef ENABLE_DYNAMIC_MODULES

static void _cmd_basemodule_scan(
	modno_t, const char *argstr, void *, command_response_f, void *) {
	MODULE_SAFECALL(
		"basemodule_scan", { alp::LineScanner::Call(basemodule_scan, argstr); })
}
static uidl_node_t *_desc_basemodule_scan(void *) { return uidl_string(0, 0); }
#endif

#define SIMPLE_FUNCTIONS_DECL(ident)                                    \
	static void _cmd_##ident(                                             \
		modno_t, const char *argstr, void *, command_response_f, void *) {  \
		MODULE_SAFECALL(#ident, { alp::LineScanner::Call(ident, argstr); }) \
	}
#define SIMPLE_FUNCTIONS_IMPL(ident) \
	module_register_command(modno, #ident, _cmd_##ident, nullptr);

#ifdef ENABLE_DYNAMIC_MODULES
#define SIMPLE_FUNCTIONS_DYN(stage)
#else
#define SIMPLE_FUNCTIONS_DYN(stage)
#endif

#define SIMPLE_FUNCTIONS(stage) SIMPLE_FUNCTIONS_DYN(stage)

SIMPLE_FUNCTIONS(DECL)

static void _cmd_module_instantiate(modno_t, const char *argstr, void *) {
	modno_t                        modno = INVALID_MODULE;
	std::string                    modname;
	std::string                    ident;
	std::string                    argstr_sub;
	alp::LineScanner::DecodeBuffer buf;

	MODULE_SAFECALL("module_instantiate", {
		alp::LineScanner ln(argstr);
		modname = ln.decode<std::string>(buf);
		ident   = ln.decode<std::string>(buf);
		(ln.get<std::string, alp::LineScanner::Remainder>(argstr_sub));

		RESPOND(I) << "instantiating modname:" << modname << " ident:" << ident
							 << alp::over;

		modno =
			module_instantiate(modname.c_str(), ident.c_str(), argstr_sub.c_str());
		RESPOND(I) << "instantiated module '" << ident << "' (" << modname << "): #"
							 << modno << alp::over;
	});

	if (modno == INVALID_MODULE) {
		RESPOND(W) << "initialization failed for module " << modname << alp::over;
	}

	hook_trigger(hook_resolve("idlChanged"));
}

static uidl_node_t *_desc_module_instantiate(void *) {
	auto mods = basemodule_list_get();

	uidl_node_t *res = uidl_keyword(nullptr, 0);

	for (size_t i = 0; i < mods->count; i++) {
		if (strncmp(mods->modules[i], "mod_", 4) == 0) {
			std::string ident     = mods->modules[i] + 4;
			basemodno_t basemodno = basemodule_init(mods->modules[i]);
			if (INVALID_BASEMOD == basemodno) continue;
			uidl_node_t *subidl = basemodule_describe(basemodno);

			uidl_keyword_set(
				res, ident.c_str(), uidl_sequence(0, 2, uidl_string(0, 0), subidl));
		}
	}
	stringlist_free(mods);
	return res;
}

static void _cmd_module_remove(modno_t, const char *argstr, void *) {
	modno_t                        modno = INVALID_MODULE;
	std::string                    instanceName;
	alp::LineScanner::DecodeBuffer buf;
	MODULE_SAFECALL(
		"module_remove", alp::LineScanner::DecodeBuffer buf; {
			alp::LineScanner ln(argstr);
			instanceName = ln.decode<std::string>(buf);
			modno        = module_find(instanceName.c_str());
		});

	if (modno == INVALID_MODULE) {
		RESPOND(W) << "module instance " << instanceName
							 << " not found - cannot delete" << alp::over;
		return;
	}

	module_remove(modno);
	hook_trigger(hook_resolve("idlChanged"));
}

static uidl_node_t *_desc_module_remove(void *) {
	return stringlist_to_idl(module_list_get(), true);
}

static void _cmd_egress_init(modno_t, const char *argstr, void *) {
	egressno_t                     egressno = INVALID_EGRESS;
	std::string                    modname;
	std::string                    ident;
	size_t                         count;
	std::string                    argstr_sub;
	alp::LineScanner::DecodeBuffer buf;
	MODULE_SAFECALL("egress_init", {
		alp::LineScanner ln(argstr);
		modname = ln.decode<std::string>(buf);
		ident   = ln.decode<std::string>(buf);
		count   = ln.decode<size_t>(buf);
		(ln.get<std::string, alp::LineScanner::Remainder>(argstr_sub));
		egressno =
			egress_init(modname.c_str(), ident.c_str(), count, argstr_sub.c_str());
		RESPOND(I) << "instantiated egress '" << ident << "' (" << modname << "): #"
							 << egressno << alp::over;
	});

	if (egressno == INVALID_EGRESS) {
		RESPOND(W) << "initialization failed for egress module " << modname
							 << alp::over;
	}
	hook_trigger(hook_resolve("idlChanged"));
}

static uidl_node_t *_desc_egress_init(void *) {
	auto mods = basemodule_list_get();

	uidl_node_t *res = uidl_keyword(nullptr, 0);

	for (size_t i = 0; i < mods->count; i++) {
		if (strncmp(mods->modules[i], "egress_", 7) == 0) {
			std::string ident     = mods->modules[i] + 7;
			basemodno_t basemodno = basemodule_init(mods->modules[i]);
			if (INVALID_BASEMOD == basemodno) continue;
			uidl_node_t *subidl = basemodule_describe(basemodno);

			uidl_keyword_set(
				res,
				ident.c_str(),
				uidl_sequence(
					0,
					3,
					uidl_string(0, 0),
					uidl_integer(0, UIDL_LIMIT_LOWER, 0, 0),
					subidl));
		}
	}
	stringlist_free(mods);
	return res;
}

static void _cmd_egress_remove(modno_t, const char *argstr, void *) {
	egressno_t                     egressno = INVALID_EGRESS;
	std::string                    instanceName;
	alp::LineScanner::DecodeBuffer buf;
	MODULE_SAFECALL(
		"egress_remove", alp::LineScanner::DecodeBuffer buf; {
			alp::LineScanner ln(argstr);
			instanceName = ln.decode<std::string>(buf);
			egressno     = egress_find(instanceName.c_str(), nullptr);
		});

	if (egressno == INVALID_EGRESS) {
		RESPOND(W) << "egress module instance " << instanceName
							 << " not found - cannot delete" << alp::over;
		return;
	}

	egress_remove(egressno);
	hook_trigger(hook_resolve("idlChanged"));
}

static uidl_node_t *_desc_egress_remove(void *) {
	return stringlist_to_idl(egress_list_get(), true);
}

static void _cmd_egress_set_active(modno_t, const char *argstr, void *) {
	egressno_t                     egressno = INVALID_EGRESS;
	std::string                    instanceName;
	alp::LineScanner::DecodeBuffer buf;
	int                            active = 1;
	MODULE_SAFECALL(
		"egress_set_active", alp::LineScanner::DecodeBuffer buf; {
			alp::LineScanner ln(argstr);
			instanceName = ln.decode<std::string>(buf);
			active       = ln.decode<int>(buf);
			egressno     = egress_find(instanceName.c_str(), nullptr);
		});

	if (egressno == INVALID_EGRESS) {
		RESPOND(W) << "egress module instance " << instanceName
							 << " not found - cannot set active" << alp::over;
		return;
	}

	egress_set_active(egressno, active);
}

static uidl_node_t *_desc_egress_set_active(void *) {
	return uidl_sequence(
		0, 2, stringlist_to_idl(egress_list_get(), true), uidl_integer(0, 0, 0, 0));
}

void        mod_display_status();
static void _cmd_status(modno_t, const char *, void *) {
	basemodule_status();
	module_status();
	egress_status();
	anim_status();
	mod_display_status();
}

static void _cmd_idl(modno_t, const char *, void *) {
	auto node = commands_describe();

	auto sb = sbuilder_new();

	uidl_node_to_json(node, sb);

	RESPOND(I) << "idl:\n" << sbuilder_str(sb) << alp::over;

	sbuilder_free(sb);
	uidl_node_free(node);
}

static void _cmd_quit(modno_t, const char *, void *) { main_stop(); }

void init(modno_t modno, const char *, void **) {
	{SIMPLE_FUNCTIONS(IMPL)};
#ifdef ENABLE_DYNAMIC_MODULES
	module_register_command(
		modno, "basemodule_scan", _cmd_basemodule_scan, _desc_basemodule_scan);
#endif
	module_register_command(
		modno,
		"module_instantiate",
		_cmd_module_instantiate,
		_desc_module_instantiate);
	module_register_command(
		modno, "module_remove", _cmd_module_remove, _desc_module_remove);
	module_register_command(
		modno, "egress_init", _cmd_egress_init, _desc_egress_init);
	module_register_command(
		modno, "egress_remove", _cmd_egress_remove, _desc_egress_remove);
	module_register_command(
		modno,
		"egress_set_active",
		_cmd_egress_set_active,
		_desc_egress_set_active);
	module_register_command(modno, "status", _cmd_status, nullptr);
	module_register_command(modno, "idl", _cmd_idl, nullptr);
	module_register_command(modno, "quit", _cmd_quit, nullptr);
}

void deinit(modno_t, void *) {}
}
