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
#include "alpha4/types/vector.hpp"
#include "core/egress_api.h"
#include "core/frame_api.h"
#include "core/module_api.h"
#include "modules/stream_api.h"
#include "util/egress.h"
#include "util/module.hpp"
#include <cstdlib>
#include <stdio.h>
#include <string_view>
#include <unordered_map>
#include <vector>

struct EgressData {
	led_i_t                   count;
	std::vector<led_stream_t> streams;
};

static std::unordered_map<egressno_t, EgressData> _Streams_pre;
static std::unordered_map<egressno_t, EgressData> _Streams_egress;
static bool                                       _Dirty;

extern "C" {

modno_t SingletonInstance = INVALID_MODULE;

static void         _cmd_streams_define(modno_t, const char *argstr, void *);
static uidl_node_t *_desc_stream_define(void *);
extern const stream_info_t STREAM_INFO[90];

static void _hook_ledsRemoved(hook_t, modno_t, void *) {
	led_i_t itOffset = 0;
	led_i_t count    = egress_leds_removed_count();
	for (auto it = _Streams_pre.begin(); it != _Streams_pre.end();) {
		const auto itCount = it->second.count;
		itOffset += itCount;
		if (egress_leds_removed_offset() >= itOffset) {
			++it;
		} else {
			it     = _Streams_pre.erase(it);
			_Dirty = true;
			if (count <= itCount) break;
			count -= itCount;
		}
	}
}

void init(modno_t modno, const char *, void **) {
	module_register_command(
		modno, "streams_define", _cmd_streams_define, _desc_stream_define);
	module_hook(modno, hook_resolve("ledsRemoved"), _hook_ledsRemoved);
}
void deinit(modno_t, void *) {
	_Streams_pre.clear();
	_Streams_egress.clear();
	_Dirty = false;
}
void flush(modno_t, void *) {
	if (_Dirty) {
		_Dirty          = false;
		_Streams_egress = _Streams_pre;
	}
}

const stream_info_t *streams_getInfo() { return STREAM_INFO; }

size_t streams_get(egressno_t egress, led_stream_t **pstreams) {
	if (auto it = _Streams_egress.find(egress); it != _Streams_egress.end()) {
		*pstreams = it->second.streams.data();
		return it->second.streams.size();
	}

	return 0;
}

static void _cmd_streams_define(modno_t, const char *argstr, void *) {
	alp::LineScanner ln(argstr);

	std::string   egressName;
	egress_info_t inf;
	if (!ln.get(egressName)) {
		RESPOND(E)
			<< "incomplete streams_define command - missing egress module name"
			<< alp::over;
		return;
	}

	if (egress_find(egressName.c_str(), &inf); inf.egressno == INVALID_EGRESS) {
		RESPOND(E) << "egress instance '" << egressName << "' does not exist"
							 << alp::over;
		return;
	}

	auto &sd = _Streams_pre[inf.egressno];
	sd.count = inf.count;

	std::string_view type_s;
	size_t           count;
	led_encoding_t   encoding;

	for (led_i_t remain = inf.count; remain > 0;) {
		if (!ln.get(type_s)) break;
		if (!ln.get(count)) {
			RESPOND(E) << "missing count for led streams_define" << alp::over;
			return;
		}
		if (0) {
		}
		// clang-format off
      /*v py
        def perm(rem,prefix=""):
          if len(rem)<1:
            yield prefix
          for v in sorted(rem):
            yield from perm(rem-{v},prefix+v)
        for seq in ( {"r","g","b"}, {"r","g","b","w"}):
          for width in (8,16,24):
            for v in perm(seq):
              this.print(f"        else if (type_s == \"{v}{width}\") encoding=led_encoding_t::{v.upper()}{width};\n")
        ! output*/
        else if (type_s == "bgr8") encoding=led_encoding_t::BGR8;
        else if (type_s == "brg8") encoding=led_encoding_t::BRG8;
        else if (type_s == "gbr8") encoding=led_encoding_t::GBR8;
        else if (type_s == "grb8") encoding=led_encoding_t::GRB8;
        else if (type_s == "rbg8") encoding=led_encoding_t::RBG8;
        else if (type_s == "rgb8") encoding=led_encoding_t::RGB8;
        else if (type_s == "bgr16") encoding=led_encoding_t::BGR16;
        else if (type_s == "brg16") encoding=led_encoding_t::BRG16;
        else if (type_s == "gbr16") encoding=led_encoding_t::GBR16;
        else if (type_s == "grb16") encoding=led_encoding_t::GRB16;
        else if (type_s == "rbg16") encoding=led_encoding_t::RBG16;
        else if (type_s == "rgb16") encoding=led_encoding_t::RGB16;
        else if (type_s == "bgr24") encoding=led_encoding_t::BGR24;
        else if (type_s == "brg24") encoding=led_encoding_t::BRG24;
        else if (type_s == "gbr24") encoding=led_encoding_t::GBR24;
        else if (type_s == "grb24") encoding=led_encoding_t::GRB24;
        else if (type_s == "rbg24") encoding=led_encoding_t::RBG24;
        else if (type_s == "rgb24") encoding=led_encoding_t::RGB24;
        else if (type_s == "bgrw8") encoding=led_encoding_t::BGRW8;
        else if (type_s == "bgwr8") encoding=led_encoding_t::BGWR8;
        else if (type_s == "brgw8") encoding=led_encoding_t::BRGW8;
        else if (type_s == "brwg8") encoding=led_encoding_t::BRWG8;
        else if (type_s == "bwgr8") encoding=led_encoding_t::BWGR8;
        else if (type_s == "bwrg8") encoding=led_encoding_t::BWRG8;
        else if (type_s == "gbrw8") encoding=led_encoding_t::GBRW8;
        else if (type_s == "gbwr8") encoding=led_encoding_t::GBWR8;
        else if (type_s == "grbw8") encoding=led_encoding_t::GRBW8;
        else if (type_s == "grwb8") encoding=led_encoding_t::GRWB8;
        else if (type_s == "gwbr8") encoding=led_encoding_t::GWBR8;
        else if (type_s == "gwrb8") encoding=led_encoding_t::GWRB8;
        else if (type_s == "rbgw8") encoding=led_encoding_t::RBGW8;
        else if (type_s == "rbwg8") encoding=led_encoding_t::RBWG8;
        else if (type_s == "rgbw8") encoding=led_encoding_t::RGBW8;
        else if (type_s == "rgwb8") encoding=led_encoding_t::RGWB8;
        else if (type_s == "rwbg8") encoding=led_encoding_t::RWBG8;
        else if (type_s == "rwgb8") encoding=led_encoding_t::RWGB8;
        else if (type_s == "wbgr8") encoding=led_encoding_t::WBGR8;
        else if (type_s == "wbrg8") encoding=led_encoding_t::WBRG8;
        else if (type_s == "wgbr8") encoding=led_encoding_t::WGBR8;
        else if (type_s == "wgrb8") encoding=led_encoding_t::WGRB8;
        else if (type_s == "wrbg8") encoding=led_encoding_t::WRBG8;
        else if (type_s == "wrgb8") encoding=led_encoding_t::WRGB8;
        else if (type_s == "bgrw16") encoding=led_encoding_t::BGRW16;
        else if (type_s == "bgwr16") encoding=led_encoding_t::BGWR16;
        else if (type_s == "brgw16") encoding=led_encoding_t::BRGW16;
        else if (type_s == "brwg16") encoding=led_encoding_t::BRWG16;
        else if (type_s == "bwgr16") encoding=led_encoding_t::BWGR16;
        else if (type_s == "bwrg16") encoding=led_encoding_t::BWRG16;
        else if (type_s == "gbrw16") encoding=led_encoding_t::GBRW16;
        else if (type_s == "gbwr16") encoding=led_encoding_t::GBWR16;
        else if (type_s == "grbw16") encoding=led_encoding_t::GRBW16;
        else if (type_s == "grwb16") encoding=led_encoding_t::GRWB16;
        else if (type_s == "gwbr16") encoding=led_encoding_t::GWBR16;
        else if (type_s == "gwrb16") encoding=led_encoding_t::GWRB16;
        else if (type_s == "rbgw16") encoding=led_encoding_t::RBGW16;
        else if (type_s == "rbwg16") encoding=led_encoding_t::RBWG16;
        else if (type_s == "rgbw16") encoding=led_encoding_t::RGBW16;
        else if (type_s == "rgwb16") encoding=led_encoding_t::RGWB16;
        else if (type_s == "rwbg16") encoding=led_encoding_t::RWBG16;
        else if (type_s == "rwgb16") encoding=led_encoding_t::RWGB16;
        else if (type_s == "wbgr16") encoding=led_encoding_t::WBGR16;
        else if (type_s == "wbrg16") encoding=led_encoding_t::WBRG16;
        else if (type_s == "wgbr16") encoding=led_encoding_t::WGBR16;
        else if (type_s == "wgrb16") encoding=led_encoding_t::WGRB16;
        else if (type_s == "wrbg16") encoding=led_encoding_t::WRBG16;
        else if (type_s == "wrgb16") encoding=led_encoding_t::WRGB16;
        else if (type_s == "bgrw24") encoding=led_encoding_t::BGRW24;
        else if (type_s == "bgwr24") encoding=led_encoding_t::BGWR24;
        else if (type_s == "brgw24") encoding=led_encoding_t::BRGW24;
        else if (type_s == "brwg24") encoding=led_encoding_t::BRWG24;
        else if (type_s == "bwgr24") encoding=led_encoding_t::BWGR24;
        else if (type_s == "bwrg24") encoding=led_encoding_t::BWRG24;
        else if (type_s == "gbrw24") encoding=led_encoding_t::GBRW24;
        else if (type_s == "gbwr24") encoding=led_encoding_t::GBWR24;
        else if (type_s == "grbw24") encoding=led_encoding_t::GRBW24;
        else if (type_s == "grwb24") encoding=led_encoding_t::GRWB24;
        else if (type_s == "gwbr24") encoding=led_encoding_t::GWBR24;
        else if (type_s == "gwrb24") encoding=led_encoding_t::GWRB24;
        else if (type_s == "rbgw24") encoding=led_encoding_t::RBGW24;
        else if (type_s == "rbwg24") encoding=led_encoding_t::RBWG24;
        else if (type_s == "rgbw24") encoding=led_encoding_t::RGBW24;
        else if (type_s == "rgwb24") encoding=led_encoding_t::RGWB24;
        else if (type_s == "rwbg24") encoding=led_encoding_t::RWBG24;
        else if (type_s == "rwgb24") encoding=led_encoding_t::RWGB24;
        else if (type_s == "wbgr24") encoding=led_encoding_t::WBGR24;
        else if (type_s == "wbrg24") encoding=led_encoding_t::WBRG24;
        else if (type_s == "wgbr24") encoding=led_encoding_t::WGBR24;
        else if (type_s == "wgrb24") encoding=led_encoding_t::WGRB24;
        else if (type_s == "wrbg24") encoding=led_encoding_t::WRBG24;
        else if (type_s == "wrgb24") encoding=led_encoding_t::WRGB24;
      //^ py
		// clang-format on
		else {
			RESPOND(E) << "invalid type for led stream: '" << type_s << "'";
			return;
		}
		if (count < 1) continue;

		if (count > remain) {
			RESPOND(W) << "too many LEDs in stream for egress module '" << egressName
								 << "' - max: " << inf.count << alp::over;
			remain = 0;
		} else {
			remain -= count;
		}
		sd.streams.emplace_back(encoding, count);
		_Dirty = true;
	}
}

static uidl_node_t *_desc_stream_define(void *) {
	uidl_node_t *idents = uidl_keyword(nullptr, 0);

	auto mods = egress_list_get();
	for (size_t i = 0; i < mods->count; i++) {
		uidl_keyword_set(idents, mods->modules[i], 0);
	}
	stringlist_free(mods);

	return uidl_sequence(
		0,
		2,
		idents,
		uidl_repeat(
			0,
			uidl_sequence(
				0,
				2,

				uidl_keyword(
					0,

					// clang-format off
          /*v py
            def perm(rem,prefix=""):
              if len(rem)<1:
                yield prefix
              for v in sorted(rem):
                yield from perm(rem-{v},prefix+v)
            kws = list()
            for seq in ( {"r","g","b"}, {"r","g","b","w"}):
              for width in (8,16,24):
                for v in perm(seq): kws.append(f"{v}{width}")
            this.print(f"            {len(kws)}\n")
            for kw in kws: this.print(f"            ,uidl_pair(\"{kw}\",0)\n")
            ! output*/
            90
            ,uidl_pair("bgr8",0)
            ,uidl_pair("brg8",0)
            ,uidl_pair("gbr8",0)
            ,uidl_pair("grb8",0)
            ,uidl_pair("rbg8",0)
            ,uidl_pair("rgb8",0)
            ,uidl_pair("bgr16",0)
            ,uidl_pair("brg16",0)
            ,uidl_pair("gbr16",0)
            ,uidl_pair("grb16",0)
            ,uidl_pair("rbg16",0)
            ,uidl_pair("rgb16",0)
            ,uidl_pair("bgr24",0)
            ,uidl_pair("brg24",0)
            ,uidl_pair("gbr24",0)
            ,uidl_pair("grb24",0)
            ,uidl_pair("rbg24",0)
            ,uidl_pair("rgb24",0)
            ,uidl_pair("bgrw8",0)
            ,uidl_pair("bgwr8",0)
            ,uidl_pair("brgw8",0)
            ,uidl_pair("brwg8",0)
            ,uidl_pair("bwgr8",0)
            ,uidl_pair("bwrg8",0)
            ,uidl_pair("gbrw8",0)
            ,uidl_pair("gbwr8",0)
            ,uidl_pair("grbw8",0)
            ,uidl_pair("grwb8",0)
            ,uidl_pair("gwbr8",0)
            ,uidl_pair("gwrb8",0)
            ,uidl_pair("rbgw8",0)
            ,uidl_pair("rbwg8",0)
            ,uidl_pair("rgbw8",0)
            ,uidl_pair("rgwb8",0)
            ,uidl_pair("rwbg8",0)
            ,uidl_pair("rwgb8",0)
            ,uidl_pair("wbgr8",0)
            ,uidl_pair("wbrg8",0)
            ,uidl_pair("wgbr8",0)
            ,uidl_pair("wgrb8",0)
            ,uidl_pair("wrbg8",0)
            ,uidl_pair("wrgb8",0)
            ,uidl_pair("bgrw16",0)
            ,uidl_pair("bgwr16",0)
            ,uidl_pair("brgw16",0)
            ,uidl_pair("brwg16",0)
            ,uidl_pair("bwgr16",0)
            ,uidl_pair("bwrg16",0)
            ,uidl_pair("gbrw16",0)
            ,uidl_pair("gbwr16",0)
            ,uidl_pair("grbw16",0)
            ,uidl_pair("grwb16",0)
            ,uidl_pair("gwbr16",0)
            ,uidl_pair("gwrb16",0)
            ,uidl_pair("rbgw16",0)
            ,uidl_pair("rbwg16",0)
            ,uidl_pair("rgbw16",0)
            ,uidl_pair("rgwb16",0)
            ,uidl_pair("rwbg16",0)
            ,uidl_pair("rwgb16",0)
            ,uidl_pair("wbgr16",0)
            ,uidl_pair("wbrg16",0)
            ,uidl_pair("wgbr16",0)
            ,uidl_pair("wgrb16",0)
            ,uidl_pair("wrbg16",0)
            ,uidl_pair("wrgb16",0)
            ,uidl_pair("bgrw24",0)
            ,uidl_pair("bgwr24",0)
            ,uidl_pair("brgw24",0)
            ,uidl_pair("brwg24",0)
            ,uidl_pair("bwgr24",0)
            ,uidl_pair("bwrg24",0)
            ,uidl_pair("gbrw24",0)
            ,uidl_pair("gbwr24",0)
            ,uidl_pair("grbw24",0)
            ,uidl_pair("grwb24",0)
            ,uidl_pair("gwbr24",0)
            ,uidl_pair("gwrb24",0)
            ,uidl_pair("rbgw24",0)
            ,uidl_pair("rbwg24",0)
            ,uidl_pair("rgbw24",0)
            ,uidl_pair("rgwb24",0)
            ,uidl_pair("rwbg24",0)
            ,uidl_pair("rwgb24",0)
            ,uidl_pair("wbgr24",0)
            ,uidl_pair("wbrg24",0)
            ,uidl_pair("wgbr24",0)
            ,uidl_pair("wgrb24",0)
            ,uidl_pair("wrbg24",0)
            ,uidl_pair("wrgb24",0)
          //^ py
					// clang-format on

					),
				uidl_integer(0, UIDL_LIMIT_LOWER, 0, 0)

					),
			0)

	);
}

// clang-format off
/*v py
  def perm(rem,prefix=""):
    if len(rem)<1:
      yield prefix
    for v in sorted(rem):
      yield from perm(rem-{v},prefix+v)

  info_count=0
  info_literal=""
  for seq in ( {"r","g","b"}, {"r","g","b","w"}):
    for width in (8,16,24):
      for v in perm(seq):
        info_count+=1
        info_literal += (f"{{{width//8*3},{768//(width//8*3)},encode_{v}{width}}}, ")
  this.print(f"\tconst stream_info_t STREAM_INFO[{info_count}] = {{ {info_literal} }};\n ")
! output*/
	const stream_info_t STREAM_INFO[90] = { {3,256,encode_bgr8}, {3,256,encode_brg8}, {3,256,encode_gbr8}, {3,256,encode_grb8}, {3,256,encode_rbg8}, {3,256,encode_rgb8}, {6,128,encode_bgr16}, {6,128,encode_brg16}, {6,128,encode_gbr16}, {6,128,encode_grb16}, {6,128,encode_rbg16}, {6,128,encode_rgb16}, {9,85,encode_bgr24}, {9,85,encode_brg24}, {9,85,encode_gbr24}, {9,85,encode_grb24}, {9,85,encode_rbg24}, {9,85,encode_rgb24}, {3,256,encode_bgrw8}, {3,256,encode_bgwr8}, {3,256,encode_brgw8}, {3,256,encode_brwg8}, {3,256,encode_bwgr8}, {3,256,encode_bwrg8}, {3,256,encode_gbrw8}, {3,256,encode_gbwr8}, {3,256,encode_grbw8}, {3,256,encode_grwb8}, {3,256,encode_gwbr8}, {3,256,encode_gwrb8}, {3,256,encode_rbgw8}, {3,256,encode_rbwg8}, {3,256,encode_rgbw8}, {3,256,encode_rgwb8}, {3,256,encode_rwbg8}, {3,256,encode_rwgb8}, {3,256,encode_wbgr8}, {3,256,encode_wbrg8}, {3,256,encode_wgbr8}, {3,256,encode_wgrb8}, {3,256,encode_wrbg8}, {3,256,encode_wrgb8}, {6,128,encode_bgrw16}, {6,128,encode_bgwr16}, {6,128,encode_brgw16}, {6,128,encode_brwg16}, {6,128,encode_bwgr16}, {6,128,encode_bwrg16}, {6,128,encode_gbrw16}, {6,128,encode_gbwr16}, {6,128,encode_grbw16}, {6,128,encode_grwb16}, {6,128,encode_gwbr16}, {6,128,encode_gwrb16}, {6,128,encode_rbgw16}, {6,128,encode_rbwg16}, {6,128,encode_rgbw16}, {6,128,encode_rgwb16}, {6,128,encode_rwbg16}, {6,128,encode_rwgb16}, {6,128,encode_wbgr16}, {6,128,encode_wbrg16}, {6,128,encode_wgbr16}, {6,128,encode_wgrb16}, {6,128,encode_wrbg16}, {6,128,encode_wrgb16}, {9,85,encode_bgrw24}, {9,85,encode_bgwr24}, {9,85,encode_brgw24}, {9,85,encode_brwg24}, {9,85,encode_bwgr24}, {9,85,encode_bwrg24}, {9,85,encode_gbrw24}, {9,85,encode_gbwr24}, {9,85,encode_grbw24}, {9,85,encode_grwb24}, {9,85,encode_gwbr24}, {9,85,encode_gwrb24}, {9,85,encode_rbgw24}, {9,85,encode_rbwg24}, {9,85,encode_rgbw24}, {9,85,encode_rgwb24}, {9,85,encode_rwbg24}, {9,85,encode_rwgb24}, {9,85,encode_wbgr24}, {9,85,encode_wbrg24}, {9,85,encode_wgbr24}, {9,85,encode_wgrb24}, {9,85,encode_wrbg24}, {9,85,encode_wrgb24},  };
 
//^ py
// clang-format on
}