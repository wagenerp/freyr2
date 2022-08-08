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

#ifndef CORE_LEDSET_HPP
#define CORE_LEDSET_HPP

#include "core/frame_api.h"
#include <algorithm>
#include <cstring>
#include <type_traits>
#include <vector>
struct LEDSet {
public:
	struct ModGuard {
	protected:
		LEDSet &_owner;

	public:
		ModGuard(LEDSet &owner) : _owner(owner) { _owner._modCount++; }
		~ModGuard() {
			if (_owner._modCount < 2) {
				_owner._modCount = 0;
				_owner.sort();
			} else {
				_owner._modCount--;
			}
		}
	};

	using storage_type = std::vector<led_i_t>;
	using iterator     = storage_type::const_iterator;
	using size_type    = storage_type::size_type;

protected:
	storage_type _storage;
	bool         _sorted   = false;
	unsigned     _modCount = 0;

public:
	size_type size() const { return _storage.size(); }
	bool      empty() const { return _storage.empty(); }
	bool      sorted() const { return _sorted; }

	const storage_type &storage() const { return _storage; }
	const led_i_t *     data() const { return _storage.data(); }

	iterator begin() const { return _storage.cbegin(); }
	iterator end() const { return _storage.cend(); }
	iterator cbegin() const { return _storage.cbegin(); }
	iterator cend() const { return _storage.cend(); }

	ModGuard beginModification() { return ModGuard(*this); }
	void     reserve(size_t count) { _storage.reserve(count); }

	void sort(bool force = false) {
		if (_sorted) return;
		if (!force & (_modCount > 0)) return;
		std::sort(_storage.begin(), _storage.end());
		{
			auto last = std::unique(_storage.begin(), _storage.end());
			_storage.erase(last, _storage.end());
		}
		_sorted = true;
	}

	LEDSet &clear() {
		_storage.clear();
		_sorted = true;
		return *this;
	}

	LEDSet &operator+=(led_i_t led) {
		_storage.emplace_back(led);
		_sorted = false;
		sort();
		return *this;
	}

	LEDSet &operator+=(const LEDSet leds) {
		_storage.insert(_storage.end(), leds.begin(), leds.end());
		_sorted = false;
		sort();
		return *this;
	}

	LEDSet &append(const led_i_t *ledv, size_t ledn) {
		if ((ledn < 1) | (nullptr == ledv)) return *this;
		const size_t i0 = _storage.size();
		_storage.resize(_storage.size() + ledn);
		std::memcpy(_storage.data() + i0, ledv, sizeof(led_i_t) * ledn);
		_sorted = false;
		sort();
		return *this;
	}

	LEDSet &append(const led_i_t first, size_t count) {
		const size_t i0 = _storage.size();
		_storage.resize(_storage.size() + count);
		for (auto i = i0; i < _storage.size(); i++) {
			_storage[i] = i - i0 + first;
		}
		_sorted = false;
		sort();
		return *this;
	}

	LEDSet &operator%=(const LEDSet leds) {
		if (_storage.empty()) return *this;
		if (!leds.sorted()) {
			LEDSet leds1 = leds;
			leds1.sort();
			return operator%=(leds1);
		}

		sort();
		_storage.insert(_storage.end(), leds._storage.begin(), leds._storage.end());
		std::sort(_storage.begin(), _storage.end());

		std::vector<led_i_t> result;
		result.reserve(std::min(_storage.size(), leds.size()));

		bool isDuplicate = false;
		for (auto it = _storage.begin(); (it + 1) != _storage.end(); ++it) {
			if (*it == *(it + 1)) {
				if (!isDuplicate) {
					isDuplicate = true;
					result.emplace_back(*it);
				}
			} else {
				isDuplicate = false;
			}
		}
		_storage = std::move(result);
		_sorted  = true;
		return *this;
	}

	LEDSet &operator-=(const LEDSet &b) {
		if (_storage.empty() | b.empty()) return *this;

		std::vector<led_i_t> result;
		result.reserve(_storage.size());

		_storage.reserve(_storage.size() + 2 * b.size());
		_storage.insert(_storage.end(), b._storage.begin(), b.storage().end());
		_storage.insert(_storage.end(), b._storage.begin(), b.storage().end());
		std::sort(_storage.begin(), _storage.end());

		bool isDuplicate = false;
		for (auto it = _storage.begin(); (it + 1) != _storage.end(); ++it) {
			if (*it == *(it + 1)) {
				isDuplicate = true;
			} else {
				if (!isDuplicate) { result.emplace_back(*it); }
				isDuplicate = false;
			}
		}

		if (!isDuplicate) { result.emplace_back(_storage.back()); }
		_storage = std::move(result);
		_sorted = true;

		return *this;
	}

	void adjustRemovedLEDs(led_i_t offset, led_i_t count) {
		if (_storage.empty()) return;
		sort(true);
		if (_storage.back() < offset) return;

		for (auto it = _storage.begin(), end = _storage.end(); it != end;) {
			if (*it < offset) {
				++it;
			} else if (*it < (offset + count)) {
				it  = _storage.erase(it);
				end = _storage.end();
			} else {
				*it -= count;
			}
		}
	}
};

#endif