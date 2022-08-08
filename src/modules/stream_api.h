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

#ifndef STREAM_API_H
#define STREAM_API_H

#include "core/egress_api.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum led_encoding_t {

	// clang-format off
		/*v py
      def perm(rem,prefix=""):
        if len(rem)<1:
          yield prefix
        for v in sorted(rem):
          yield from perm(rem-{v},prefix+v)
      idx=0
      this.print("\t\t\t")
      for seq in ( {"r","g","b"}, {"r","g","b","w"}):
        for width in (8,16,24):
          for v in perm(seq):
            this.print(f"{v.upper()}{width}={idx}, ")
            idx+=1
    ! output*/
			BGR8=0, BRG8=1, GBR8=2, GRB8=3, RBG8=4, RGB8=5, BGR16=6, BRG16=7, GBR16=8, GRB16=9, RBG16=10, RGB16=11, BGR24=12, BRG24=13, GBR24=14, GRB24=15, RBG24=16, RGB24=17, BGRW8=18, BGWR8=19, BRGW8=20, BRWG8=21, BWGR8=22, BWRG8=23, GBRW8=24, GBWR8=25, GRBW8=26, GRWB8=27, GWBR8=28, GWRB8=29, RBGW8=30, RBWG8=31, RGBW8=32, RGWB8=33, RWBG8=34, RWGB8=35, WBGR8=36, WBRG8=37, WGBR8=38, WGRB8=39, WRBG8=40, WRGB8=41, BGRW16=42, BGWR16=43, BRGW16=44, BRWG16=45, BWGR16=46, BWRG16=47, GBRW16=48, GBWR16=49, GRBW16=50, GRWB16=51, GWBR16=52, GWRB16=53, RBGW16=54, RBWG16=55, RGBW16=56, RGWB16=57, RWBG16=58, RWGB16=59, WBGR16=60, WBRG16=61, WGBR16=62, WGRB16=63, WRBG16=64, WRGB16=65, BGRW24=66, BGWR24=67, BRGW24=68, BRWG24=69, BWGR24=70, BWRG24=71, GBRW24=72, GBWR24=73, GRBW24=74, GRWB24=75, GWBR24=76, GWRB24=77, RBGW24=78, RBWG24=79, RGBW24=80, RGWB24=81, RWBG24=82, RWGB24=83, WBGR24=84, WBRG24=85, WGBR24=86, WGRB24=87, WRBG24=88, WRGB24=89, 
    //^ py
	// clang-format on

	LED_ENCODING_COUNT

} led_encoding_t;

typedef struct stream_info_t {
	size_t bpp;
	size_t ce_per_msg;
	uint8_t *(*encode)(const led_t *, uint8_t *);
} stream_info_t;

typedef struct led_stream_t {
	led_encoding_t type;
	size_t         count;
} led_stream_t;

const stream_info_t *streams_getInfo();
size_t               streams_get(egressno_t egress, led_stream_t **pstreams);

#ifdef __cplusplus
}
#endif

#endif