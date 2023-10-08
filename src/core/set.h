#pragma once

#include "table.h"
#include "iterator.h"

#include <initializer_list>

namespace core {
	template <typename T>
	struct set {
		using type = T;

		set(u32 sz = 0)
		: _hash(), _keys(), reserve(0), size(0) {
			_allocate(sz);
		}

		set(std::initializer_list<type> l)
		: _hash(), _keys(), reserve(0), size(0) {
			*this = l;
		}

		~set() {
			reserve = 0;
			size = 0;
		}

		ref<set<type>> operator=(std::initializer_list<type> l) {
			_allocate(0);
			for (auto& item : l) {
				add(item);
			}

			return *this;
		}

		void _allocate(u32 sz) {
			sz = table_size(max<u32>(sz, 32));
			_hash = vector<u32>(sz);
			_keys = vector<type>(sz);
			reserve = sz;
		}

		void resize(u32 sz) {
			sz = table_size(sz);

			vector<u32> nhash = _hash;
			vector<type> nkeys = _keys;

			_hash = vector<u32>(sz);
			_keys = vector<type>(sz);

			u32 count = reserve;
			reserve = sz;
			for (i32 i : range(count)) {
				u32 hash = nhash[i];
				if (!hash) continue;
				_probe(nkeys[i], hash);
			}
		}

		u32 _probe(type key, u32 hash) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			u32 probe = 0;
			u32 i = hash % reserve;

			while (true) {
				u32 cur = _hash[i];
				if (!cur) {
					_hash[i] = hash;
					_keys[i] = key;
					break;
				}

				u32 dist = (cur % reserve) - i;
				if (dist < probe) {
					swap(_hash[i], hash);
					swap(_keys[i], key);
				}

				i = (i + 1) % reserve;
				probe++;
			}

			return probe;
		}

		u32 _find(cref<type> key) const {
			u32 h = hash(key);
			for (i32 i : range(TABLE_PROBE)) {
				u32 idx = (h + i) % reserve;
				u32 tmp = _hash[idx];
				if (tmp == h) {
					if (key == _keys[idx]) return idx;
				}
			}

			return U32_MAX;
		}

		bool has(cref<type> key) const {
			return _find(key) != U32_MAX;
		}

		void add(cref<type> key) {
			if (has(key)) return;
			u32 p = _probe(key, hash(key));
			size++;

			if (p >= TABLE_PROBE) {
				resize(reserve * 2);
			}
		}

		void del(cref<type> key) {
			u32 idx = _find(key);
			if (idx == U32_MAX) return;

			ref<type> _key = _keys[idx];
			_keys._cleanup<is_destructible_v<type>>(_key);
			zero8(bytes(_key), sizeof(type));
			_hash[idx] = 0;

			size--;
		}

		struct iterator {
			iterator(cref<vector<type>> _keys, u32* _hash, u32 _size, u32 _reserve)
				: keys(_keys), hash(_hash), size(_size), reserve(_reserve) {}

			ref<iterator> operator++() {
				if (reserve <= ++size) return *this;
				hash++;
				while (*hash == 0) {
					hash++;
				}

				return *this;
			}

			bool operator!=(cref<iterator> other) const {
				return size != other.size;
			}

			ref<type> operator*() const {
				u32 i = *hash % reserve;
				return keys[i];
			}

			cref<vector<type>> keys;
			u32* hash;
			u32 size;
			u32 reserve;
		};

		auto begin() {
			u32* hash = _hash.data;
			while (*hash == 0) {
				hash++;
			}

			return iterator(_keys, hash, 0, reserve);
		}

		auto end() {
			return iterator(_keys, _hash.data, size, reserve);
		}

		vector<u32> _hash;
		vector<type> _keys;
		u32 reserve;
		u32 size;
	};
}
