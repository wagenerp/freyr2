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

#ifndef UTIL_EGRESS_H
#define UTIL_EGRESS_H
#include "alpha4c/common/inline.h"
#include "core/frame_api.h"

#ifdef __cplusplus
extern "C" {
#endif

ALPHA4C_INLINE(uint8_t ctou8)(color_c_t c) {
	c *= 255;
	if (!(c < 255)) return 255;
	if (!(c > 0)) return 0;
	return (uint8_t)c;
}
ALPHA4C_INLINE(uint16_t ctou16)(color_c_t c) {
	c *= 65535;
	if (!(c < 65535)) return 65535;
	if (!(c > 0)) return 0;
	return (uint8_t)c;
}
ALPHA4C_INLINE(float ctouf)(color_c_t c) { return c; }

ALPHA4C_INLINE(float satf)(float v) {
	if (!(v < 1)) return 1;
	if (!(v > 0)) return 0;
	return v;
}

ALPHA4C_INLINE(uint8_t *encode_c24)(color_c_t c, uint8_t *p) {
	uint32_t comp = 0xffffff * satf(c);
	*p++          = comp >> 16;
	*p++          = (comp >> 8) & 0xff;
	*p++          = comp & 0xff;
	return p;
}

ALPHA4C_INLINE(uint8_t *encode_c16)(color_c_t c, uint8_t *p) {
	uint16_t comp = 65535 * satf(c);
	*p++          = comp >> 8;
	*p++          = comp & 0xff;
	return p;
}
ALPHA4C_INLINE(uint8_t *encode_c8)(color_c_t c, uint8_t *p) {
	*p++ = 255 * satf(c);
	return p;
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
        this.print(f"\tALPHA4C_INLINE(uint8_t *encode_{v}{width})(const led_t *cl, uint8_t *p) {{ ")
        for c in v:
          if c=="w":
            this.print(f"p=encode_c{width}(0,p); ")
          else:
            this.print(f"p=encode_c{width}(cl->{c},p); ")
        this.print("return p; }\n")
  info_count=0
  for seq in ( {"r","g","b"}, {"r","g","b","w"}):
    for width in (8,16,24):
      for v in perm(seq):
        info_count+=1
! output*/
	ALPHA4C_INLINE(uint8_t *encode_bgr8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brg8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbr8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grb8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbg8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgb8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgr16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brg16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbr16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grb16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbg16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgb16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgr24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brg24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbr24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grb24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbg24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgb24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgrw8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); p=encode_c8(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgwr8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); p=encode_c8(0,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brgw8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); p=encode_c8(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brwg8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); p=encode_c8(0,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bwgr8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(0,p); p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bwrg8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->b,p); p=encode_c8(0,p); p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbrw8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); p=encode_c8(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbwr8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); p=encode_c8(0,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grbw8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); p=encode_c8(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grwb8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); p=encode_c8(0,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gwbr8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(0,p); p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gwrb8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->g,p); p=encode_c8(0,p); p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbgw8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); p=encode_c8(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbwg8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); p=encode_c8(0,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgbw8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); p=encode_c8(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgwb8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); p=encode_c8(0,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rwbg8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(0,p); p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rwgb8)(const led_t *cl, uint8_t *p) { p=encode_c8(cl->r,p); p=encode_c8(0,p); p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wbgr8)(const led_t *cl, uint8_t *p) { p=encode_c8(0,p); p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wbrg8)(const led_t *cl, uint8_t *p) { p=encode_c8(0,p); p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wgbr8)(const led_t *cl, uint8_t *p) { p=encode_c8(0,p); p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); p=encode_c8(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wgrb8)(const led_t *cl, uint8_t *p) { p=encode_c8(0,p); p=encode_c8(cl->g,p); p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wrbg8)(const led_t *cl, uint8_t *p) { p=encode_c8(0,p); p=encode_c8(cl->r,p); p=encode_c8(cl->b,p); p=encode_c8(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wrgb8)(const led_t *cl, uint8_t *p) { p=encode_c8(0,p); p=encode_c8(cl->r,p); p=encode_c8(cl->g,p); p=encode_c8(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgrw16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); p=encode_c16(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgwr16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); p=encode_c16(0,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brgw16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); p=encode_c16(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brwg16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); p=encode_c16(0,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bwgr16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(0,p); p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bwrg16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->b,p); p=encode_c16(0,p); p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbrw16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); p=encode_c16(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbwr16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); p=encode_c16(0,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grbw16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); p=encode_c16(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grwb16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); p=encode_c16(0,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gwbr16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(0,p); p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gwrb16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->g,p); p=encode_c16(0,p); p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbgw16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); p=encode_c16(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbwg16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); p=encode_c16(0,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgbw16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); p=encode_c16(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgwb16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); p=encode_c16(0,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rwbg16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(0,p); p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rwgb16)(const led_t *cl, uint8_t *p) { p=encode_c16(cl->r,p); p=encode_c16(0,p); p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wbgr16)(const led_t *cl, uint8_t *p) { p=encode_c16(0,p); p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wbrg16)(const led_t *cl, uint8_t *p) { p=encode_c16(0,p); p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wgbr16)(const led_t *cl, uint8_t *p) { p=encode_c16(0,p); p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); p=encode_c16(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wgrb16)(const led_t *cl, uint8_t *p) { p=encode_c16(0,p); p=encode_c16(cl->g,p); p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wrbg16)(const led_t *cl, uint8_t *p) { p=encode_c16(0,p); p=encode_c16(cl->r,p); p=encode_c16(cl->b,p); p=encode_c16(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wrgb16)(const led_t *cl, uint8_t *p) { p=encode_c16(0,p); p=encode_c16(cl->r,p); p=encode_c16(cl->g,p); p=encode_c16(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgrw24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); p=encode_c24(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bgwr24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); p=encode_c24(0,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brgw24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); p=encode_c24(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_brwg24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); p=encode_c24(0,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bwgr24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(0,p); p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_bwrg24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->b,p); p=encode_c24(0,p); p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbrw24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); p=encode_c24(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gbwr24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); p=encode_c24(0,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grbw24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); p=encode_c24(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_grwb24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); p=encode_c24(0,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gwbr24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(0,p); p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_gwrb24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->g,p); p=encode_c24(0,p); p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbgw24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); p=encode_c24(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rbwg24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); p=encode_c24(0,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgbw24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); p=encode_c24(0,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rgwb24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); p=encode_c24(0,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rwbg24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(0,p); p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_rwgb24)(const led_t *cl, uint8_t *p) { p=encode_c24(cl->r,p); p=encode_c24(0,p); p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wbgr24)(const led_t *cl, uint8_t *p) { p=encode_c24(0,p); p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wbrg24)(const led_t *cl, uint8_t *p) { p=encode_c24(0,p); p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wgbr24)(const led_t *cl, uint8_t *p) { p=encode_c24(0,p); p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); p=encode_c24(cl->r,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wgrb24)(const led_t *cl, uint8_t *p) { p=encode_c24(0,p); p=encode_c24(cl->g,p); p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wrbg24)(const led_t *cl, uint8_t *p) { p=encode_c24(0,p); p=encode_c24(cl->r,p); p=encode_c24(cl->b,p); p=encode_c24(cl->g,p); return p; }
	ALPHA4C_INLINE(uint8_t *encode_wrgb24)(const led_t *cl, uint8_t *p) { p=encode_c24(0,p); p=encode_c24(cl->r,p); p=encode_c24(cl->g,p); p=encode_c24(cl->b,p); return p; }
//^ py
// clang-format on

#ifdef __cplusplus
}
#endif

#endif