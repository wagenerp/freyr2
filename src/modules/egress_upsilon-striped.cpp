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


#if defined(__linux__)

#include "alpha4/common/error.hpp"
#include "alpha4/common/linescanner.hpp"
#include "alpha4/common/logger.hpp"
#include "core/egress_api.h"
#include "modules/stream_api.h"
#include "util/egress.h"
#include <netdb.h>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

struct strand_t {
	enum mode_t {
		WS2811   = 0,
		Upsilon2 = 1,
	};
	mode_t                    mode  = WS2811;
	uint16_t                  count = 0;
	size_t                    index = 0;
	mutable std::vector<char> buffer;
};

struct UARTEncoder {
	uint8_t *&ptr;
	uint8_t   currentByte = 0;
	int       counter     = 0;

	static size_t EncodedSize(size_t frames) {
		size_t res = frames * 10;
		return res / 8 + ((res % 8) ? 1 : 0);
	}

	UARTEncoder(uint8_t *&ptr) : ptr(ptr) {}

	inline void addBit(int v) {
		currentByte = (currentByte << 1) | (v & 1);
		if (++counter == 8) {
			*ptr++      = currentByte;
			currentByte = 0;
			counter     = 0;
		}
	}

	inline void flush() {
		while (counter != 0)
			addBit(1);
	}
	inline void addFrame(uint8_t data) {
		addBit(0);
		for (int i = 0; i < 8; i++)
			addBit(data >> i);
		addBit(1);
	}

	inline void addIdle(size_t duration) {
		for (size_t i = 0; i < duration; i++)
			addBit(1);
	}
};

struct Userdata {
	const egressno_t      egressno;
	struct sockaddr_in    addr;
	std::vector<strand_t> strands;
	bool                  buffered = false;

	mutable std::vector<char> frameHeader;
	mutable std::vector<char> frameBuffer;

	int                  sockfd = -1;
	std::vector<uint8_t> buf;

	Userdata(egressno_t egressno, const char *argstr) : egressno(egressno) {
		alp::LineScanner ln(argstr);
		{ // get host and port
			std::string  host;
			unsigned int port;
			if (!ln.getAll(host, port)) {
				alp::thrower<alp::Exception>()
					<< "incomplete upsilon egress command: missing host / port"
					<< alp::over;
			}

			struct hostent *he;
			if (!(((he = gethostbyname(host.c_str())) != nullptr)
						&& (he->h_addrtype == AF_INET)
						&& (he->h_addr_list[0] != nullptr))) {
				alp::thrower<alp::Exception>()
					<< "unable to resolve host '" << host << "'" << alp::over;
			}

			addr.sin_family      = AF_INET;
			addr.sin_port        = htons(port);
			addr.sin_addr.s_addr = *(uint32_t *)he->h_addr_list[0];
		}

		{ // get list of strands w/ config and length
			std::string_view str;
			strand_t         newStrand;
			size_t           i_strand = 0;
			while (ln.get(str)) {
				if (str == "buffered") {
					buffered = true;
				} else if (str == "upsilon2") {
					newStrand.mode = strand_t::Upsilon2;
				} else if (str == "ws2811") {
					newStrand.mode = strand_t::WS2811;
				} else {
					if (uint16_t length;
							alp::decodeString<uint16_t>(str, length) && (length != 0)) {
						newStrand.index = i_strand;
						newStrand.count = length;
						strands.push_back(newStrand);
						newStrand = strand_t();
					}
					i_strand++;
				}
			}
		}

		{ // initialize header
			frameHeader.resize(8 + 80 * 2, 0);
			frameHeader[0] = 0x42; // blt;
			frameHeader[1] = 0x0;  // flags;
			frameHeader[2] = 0x01; // address;
			frameHeader[3] = 0x0;  // address;
			frameHeader[4] = 0x0;  // address;
			frameHeader[5] = 0x0;  // address;
		}

		sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

	~Userdata() {
		if (sockfd != -1) {
			shutdown(sockfd, SHUT_RDWR);
			close(sockfd);
		}
	}

	void update(led_i_t firstLED, led_i_t) {
		// todo: double check that count is correct
		if (strands.size() < 1) return;
		constexpr const size_t cb_command_header = 8;

		{ // fill strand buffers
			led_stream_t *streams    = nullptr;
			size_t        ce_streams = streams_get(egressno, &streams);

			if ((ce_streams < 1) || (nullptr == streams)) return;

			const auto *leds = frame_raw_egress();

			auto     it_strand     = strands.begin();
			auto     end_strand    = strands.end();
			auto     it_stream     = streams;
			auto     end_stream    = streams + ce_streams;
			uint16_t strand_offset = 0;
			size_t   stream_offset = 0;
			size_t   buffer_offset = 0;
			size_t   led_offset    = firstLED;
			uint8_t  u2_index      = 0;
			auto     streamInfo    = streams_getInfo();

			struct UpsilonHelper {
				UARTEncoder &uart;
				void         encode(float v) {
          uint32_t pwm = 0xffff * satf(v);

          // todo: compute intensity or derive from additional led data
          uint8_t intensity = 0xff; // 0xff * __satf(v);

          // pwm
          uart.addFrame((pwm >> 8) & 0xff);
          uart.addFrame((pwm >> 0) & 0xff);

          uart.addFrame((intensity)&0xff); // intensity
				}
			};

			while ((it_strand != end_strand) && (it_stream != end_stream)) {
				uint16_t count = std::min(
					it_strand->count - strand_offset,
					(int)(it_stream->count - stream_offset));
				size_t cb = 0;
				switch (it_strand->mode) {
					case strand_t::WS2811:
						cb = streamInfo[it_stream->type].bpp * count;
						break;
					case strand_t::Upsilon2:
						// 12.5 bytes for 10 frames of UART data plus 4 bits of idle time
						// between pixels, 20 frames of synchronization plus 64 bits of sync
						// delay after pixels 1 frame of sync plus six bits of idle time
						cb = 13 * count + 33 + 2;
						break;
				}
				{
					size_t cb_buffer = cb + buffer_offset;
					if (cb_buffer != it_strand->buffer.size()) {
						it_strand->buffer.resize(cb_buffer);
					}
				}
				uint8_t *pbuf = (uint8_t *)it_strand->buffer.data() + buffer_offset;
				auto &   inf  = streamInfo[it_stream->type];
				switch (it_strand->mode) {
					case strand_t::WS2811:
						for (size_t i = led_offset, e = led_offset + count; i < e; i++) {
							pbuf = inf.encode(leds + i, pbuf);
						}
						break;
					case strand_t::Upsilon2: {
						UARTEncoder   uart(pbuf);
						UpsilonHelper u{uart};
						for (int i = 0; i < 20; i++)
							uart.addFrame(0x80); // 25.0 B of raw frame data
						for (size_t i = led_offset, e = led_offset + count; i < e; i++) {
							// 12.5 + 0.5 B of frame data + padding
							auto &led = leds[i];
							uart.addFrame(u2_index);
							u2_index++;
							u.encode(led.r);
							u.encode(led.g);
							u.encode(led.b);
							uart.flush();
						}
						{ // 2B of sync frame + idle
							uart.addFrame(0x88);
							uart.flush();
						}
						uart.addIdle(64);

					} break;
				}

				if (count + strand_offset >= it_strand->count) {
					++it_strand;
					strand_offset = 0;
					buffer_offset = 0;
					u2_index      = 0;
				} else {
					strand_offset += count;
					buffer_offset += cb;
				}
				if (count + stream_offset >= it_stream->count) {
					++it_stream;
					stream_offset = 0;
				} else {
					stream_offset += count;
				}

				led_offset += count;
			}
		}

		{ // write frame header and striped buffer
			size_t cb_total = cb_command_header; // leave space for the command header
			for (const auto &strand : strands) {
				uint16_t cb = strand.buffer.size();
				cb_total += cb;

				uint16_t header = cb;
				if (strand.mode == strand_t::Upsilon2) { header |= 0x8000; }

				frameHeader[6 + strand.index * 2] = (header >> 8) & 0xff;
				frameHeader[7 + strand.index * 2] = (header)&0xff;
			}
			if (frameBuffer.size() != cb_total) { frameBuffer.resize(cb_total); }

			size_t i_round = 0;
			for (char *pbuf = frameBuffer.data() + cb_command_header,
								*ebuf = frameBuffer.data() + cb_total;
					 pbuf < ebuf;) {
				for (auto &strand : strands) {
					if (strand.buffer.size() <= i_round) continue;
					*pbuf++ = strand.buffer[i_round];
				}
				i_round++;
			}
		}

		// start the frame
		sendto(
			sockfd,
			frameHeader.data(),
			frameHeader.size(),
			MSG_WAITALL,
			(struct sockaddr *)&addr,
			sizeof(struct sockaddr_in));

		// {
		// 	using namespace std::chrono_literals;
		// 	std::this_thread::sleep_for(100us);
		// }

		if (!buffered) { // stream out frame buffer
			const size_t stride = 512;
			const size_t cb_buf = frameBuffer.size();
			char *       buf    = frameBuffer.data();
			// stream in one additional byte of padding
			for (size_t ib = cb_command_header - 1; ib < cb_buf; ib += stride) {
				size_t cb_msg = stride;
				if (ib + cb_msg > cb_buf) cb_msg = cb_buf - ib;

				// add the command header
				buf[ib - 1] = 0x52;

				sendto(
					sockfd,
					buf + ib - 1,
					cb_msg + 1,
					MSG_DONTWAIT,
					(struct sockaddr *)&addr,
					sizeof(struct sockaddr_in));
			}
		} else { // blt out frame buffer
			const size_t stride = 512;
			const size_t cb_buf = frameBuffer.size();
			char *       buf    = frameBuffer.data();
			// stream in one additional byte of padding
			size_t n = 0;
			for (size_t ib = cb_command_header; ib < cb_buf; ib += stride) {
				size_t       cb_msg  = stride;
				const size_t busaddr = 0x2000'0000 + ib - cb_command_header;
				if (ib + cb_msg > cb_buf) cb_msg = cb_buf - ib;

				// add the command header
				buf[ib - 6] = 0x42;                                  // command
				buf[ib - 5] = (ib + stride >= cb_buf) ? 0x01 : 0x00; // flags
				buf[ib - 4] = (busaddr >> 24) & 0xff;                // busaddr
				buf[ib - 3] = (busaddr >> 16) & 0xff;                // busaddr
				buf[ib - 2] = (busaddr >> 8) & 0xff;                 // busaddr
				buf[ib - 1] = (busaddr)&0xff;                        // busaddr

				sendto(
					sockfd,
					buf + ib - 6,
					cb_msg + 6,
					MSG_DONTWAIT,
					(struct sockaddr *)&addr,
					sizeof(struct sockaddr_in));
				n++;
			}
		}
	}
};

extern "C" {

void init(egressno_t egressno, const char *argstr, Userdata **puserdata) {
	*puserdata = new Userdata(egressno, argstr);
}

void deinit(Userdata *userdata) { delete userdata; }

void flush(led_i_t firstLED, led_i_t count, Userdata *userdata) {
	userdata->update(firstLED, count);
}
uidl_node_t *describe() {
	return uidl_sequence(
		0,
		3,
		uidl_string(0, 0),
		uidl_integer(0, UIDL_LIMIT_RANGE, 0, 0xffff),

		uidl_repeat(
			0,
			uidl_keyword(
				0,
				3,
				uidl_pair("buffered", 0),
				uidl_pair("upsilon2", 0),
				uidl_pair("ws2811", 0)

					),
			0)

	);
}
}

#endif