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

#include "egress.hpp"

#include "alpha4/common/logger.hpp"
#include "core/animation.hpp"
#include "core/animation_api.h"
#include "core/basemodule_api.h"
#include "core/egress_api.h"
#include "core/frame.hpp"
#include "core/frame_api.h"
#include "core/module.hpp"
#include "core/module_api.h"
#include "util/module.hpp"
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_map>

static std::vector<std::shared_ptr<EgressInstance>> _EgressList;
static std::unordered_map<animno_t, std::shared_ptr<EgressInstance>> _EgressMap;
static std::unordered_map<std::string, std::shared_ptr<EgressInstance>>
								_EgressNames;
static animno_t _EgressCount = 0;
static animno_t AllocateEgressNumber() { return ++_EgressCount; }

void EgressInstance::Flush() {
	led_i_t offset = 0;
	for (auto egress : _EgressList) {
		if (egress->active && egress->flush()) {
			egress->flush()(offset, egress->_count, egress->userdata());
		}
		offset += egress->_count;
	}
}

EgressInstance::EgressInstance(
	const std::string &ident,
	basemodno_t        basemodno,
	led_i_t            count,
	const std::string &argstring,
	const std::string &instanceName) :
	_egressno(AllocateEgressNumber()),
	_ident(ident),
	_basemodno(basemodno),
	_count(count),
	_instanceName(instanceName) {
	_init =
		reinterpret_cast<egress_init_f>(basemodule_resolve(basemodno, "init"));
	_deinit =
		reinterpret_cast<egress_deinit_f>(basemodule_resolve(basemodno, "deinit"));
	_flush =
		reinterpret_cast<egress_flush_t>(basemodule_resolve(basemodno, "flush"));

	if (!_flush) {
		LOG(E) << "bad egress module " << _ident << " - has no flush function"
					 << alp::over;
	}

	if (_init) {
		_init(_egressno, argstring.c_str(), &_userdata);

		if ((nullptr != _userdata) && (!_deinit)) {
			LOG(E) << "egress #" << _egressno << " (" << _ident
						 << ") allocated userdata without destructor" << alp::over;
		}
	}

	basemodule_grab(_basemodno);
}

EgressInstance::~EgressInstance() {
	if (_deinit) { _deinit(_userdata); }
	basemodule_drop(_basemodno);
}

extern "C" {

void egress_status() {
	auto msg =
		std::move(RESPOND(I) << "egress modules: " << _EgressMap.size() << "\n");
	for (auto it : _EgressMap) {
		msg << "  #" << it.first << " (" << it.second->ident() << " "
				<< it.second->instanceName() << ") count:" << it.second->count()
				<< " active:" << it.second->active << "\n";
	}
	msg << alp::over;
}

stringlist_t *egress_list_get() {
	auto res = stringlist_new();

	res->count = _EgressNames.size();
	if (res->count > 0) {
		res->modules = (char **)malloc(res->count * sizeof(char *));
		char **p     = res->modules;
		for (auto it : _EgressNames) {
			*p++ = strdup(it.second->instanceName().c_str());
		}
	} else {
		res->modules = nullptr;
	}
	return res;
}

#define HOOK_DATA(type, ident) \
	static type _Egress_##ident; \
	type        egress_##ident() { return _Egress_##ident; }

HOOK_DATA(led_i_t, leds_added_count);
HOOK_DATA(led_i_t, leds_removed_offset);
HOOK_DATA(led_i_t, leds_removed_count);

#define HOOK_NAME(ident)                                 \
	hook_t _Hook_##ident() {                               \
		static hook_t res = INVALID_HOOK;                    \
		if (res == INVALID_HOOK) res = hook_resolve(#ident); \
		return res;                                          \
	}

HOOK_NAME(ledsAdded)
HOOK_NAME(ledsRemoved)

#undef HOOK_DATA

egressno_t egress_init(
	const char * ident,
	const char * instanceName,
	const size_t count,
	const char * argstr) {
	if (auto it = _EgressNames.find(instanceName); it != _EgressNames.end()) {
		return it->second->egressno();
	}

	std::string moduleName = "egress_" + std::string(ident);
	basemodno_t bmod       = basemodule_init(moduleName.c_str());
	if (INVALID_BASEMOD == bmod) {
		LOG(W) << "unable to find egress module " << ident << alp::over;
		return INVALID_EGRESS;
	}

	{
		auto inst = std::make_shared<EgressInstance>(
			ident, bmod, count, argstr, instanceName);
		_EgressNames[instanceName]   = inst;
		_EgressMap[inst->egressno()] = inst;
		_EgressList.push_back(inst);

		_Egress_leds_added_count = count;
		hook_trigger(_Hook_ledsAdded());

		Frame::LEDsAdded(count);
		return inst->egressno();
	}
}
egressno_t egress_find(const char *instanceName, egress_info_t *inf) {
	if (auto it = _EgressNames.find(instanceName); it != _EgressNames.end()) {
		if (inf) { egress_info(it->second->egressno(), inf); }
		return it->second->egressno();
	}
	return INVALID_EGRESS;
}
int egress_info(egressno_t egressno, egress_info_t *inf) {
	led_i_t offset = 0;
	inf->egressno  = INVALID_EGRESS;
	for (auto &egress : _EgressList) {
		if (egress->egressno() == egressno) {
			inf->egressno = egressno;
			inf->offset   = offset;
			inf->count    = egress->count();
			return 1;
		}
		offset += egress->count();
	}
	return 0;
}

led_i_t egress_offset(egressno_t egressno) {
	led_i_t offset = 0;
	for (auto &egress : _EgressList) {
		if (egress->egressno() == egressno) return offset;
		offset += egress->count();
	}
	return offset;
}

void egress_set_active(egressno_t egressno, int active) {
	for (auto &egress : _EgressList) {
		if (egress->egressno() == egressno) {
			egress->active = active;
			break;
		}
	}
}

void egress_remove(egressno_t egress) {
	led_i_t offset = 0;
	for (auto it = _EgressList.begin(), end = _EgressList.end(); it != end;
			 ++it) {
		if ((*it)->egressno() == egress) {
			auto mod = *it;
			_EgressNames.erase(mod->instanceName());
			const auto count = (*it)->count();

			_Egress_leds_removed_offset = offset;
			_Egress_leds_removed_count  = count;
			hook_trigger(_Hook_ledsRemoved());

			Frame::LEDsRemoved(offset, count);
			AnimatorPool::Get().ledsRemoved(offset, count);
			Animation::LEDsRemoved(offset, count);
			_EgressList.erase(it);
			break;
		}
		offset += (*it)->count();
	}
	if (auto it = _EgressMap.find(egress); it != _EgressMap.end()) {
		_EgressMap.erase(it);
	}
}

void egress_cleanup() {
	_Egress_leds_removed_offset = 0;
	_Egress_leds_removed_count  = 0;
	for (auto it = _EgressList.begin(), end = _EgressList.end(); it != end;
			 ++it) {
		_Egress_leds_removed_count += (*it)->count();
	}
	hook_trigger(_Hook_ledsRemoved());

	Frame::LEDsRemoved(0, _Egress_leds_removed_count);
	AnimatorPool::Get().ledsRemoved(0, _Egress_leds_removed_count);
	Animation::LEDsRemoved(0, _Egress_leds_removed_count);

	_EgressList.clear();
	_EgressMap.clear();
	_EgressNames.clear();
	_EgressCount = 0;
}
}