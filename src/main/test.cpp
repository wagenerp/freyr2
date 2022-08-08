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

#include "alpha4/common/string.hpp"
#include <iostream>
#include <string_view>

struct Foo {
	int a;
};

namespace alp {
template<>
struct StringTranscoder<Foo> {
	bool operator()(const std::string_view &s, Foo &v) {
		return StringTranscoder<int>()(s,v.a);
	}
};
}

int main() {
	

	Foo foo = alp::decodeString<Foo>("12 ");

	std::cout << foo.a << std::endl;

	return foo.a;
}