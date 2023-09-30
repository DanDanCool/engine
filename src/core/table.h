#pragma once

#include "vector.h"

namespace core {
	const u32 TABLE_PROBE = 24;

	u32 table_size(u32 sz);

	template<typename K, typename V>
	struct table {
		using key_type = K;
		using val_type = V;

		struct hash_ {
			u32 hash;
			u32 idx;
		};

		table(u32 sz = 0)
		: _hash(), _keys(), _vals(0), reserve(0), size(0) {
			sz = table_size(max<u32>(sz, 100));
			_hash = vector<hash_>(sz);
			_keys = vector<key_type>(sz);
			reserve = sz;
		}

		~table() {
			reserve = 0;
			size = 0;
		}

		void resize(u32 sz) {
			sz = table_size(sz);

			vector<hash_> nhash = _hash;
			vector<key_type> nkeys = _keys;

			_hash = vector<hash_>(sz);
			_keys = vector<key_type>(sz);

			u32 count = reserve;
			reserve = sz;
			for (i32 i : range(count)) {
				cref<hash_> h = nhash[i];
				if (!h.hash) continue;
				probe(nkeys[i], h);
			}
		}

		u32 probe(key_type key, hash_ h) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			u32 probe = 0;
			u32 i = h.hash % reserve;

			while (true) {
				ref<hash_> cur = _hash[i];
				if (!cur.hash) {
					cur = h;
					_keys[i] = key;
					break;
				}

				u32 dist = (cur.hash % reserve) - i;
				if (dist < probe) {
					swap(cur, h);
					swap(_keys[i], key);
				}

				i = (i + 1) % reserve;
				probe++;
			}

			key = {}; // do this to prevent freeing of memory

			size++;
			return probe;
		}

		u32 find_(cref<key_type> key) const {
			u32 h = hash(key);
			for (i32 i : range(TABLE_PROBE)) {
				u32 idx = (h + i) % reserve;
				cref<hash_> tmp = _hash[idx];
				if (tmp.hash == h) {
					if (key == _keys[idx]) return idx;
				}
			}

			return U32_MAX;
		}

		void set(cref<key_type> key, cref<val_type> val) {
			u32 idx = find_(key);
			if (idx != U32_MAX) {
				_vals[_hash[idx].idx] = val;
				return;
			}

			u32 p = probe(key, hash_{ hash(key), _vals.size });
			_vals.add(val);

			if (p >= TABLE_PROBE) {
				resize(reserve * 2);
			}
		}

		ref<val_type> get(cref<key_type> key) const {
			u32 idx = find_(key);
			assert(idx != U32_MAX);
			return vals[hash[idx].idx];
		}

		void del(cref<key_type> key) {
			u32 idx = find_(key);
			assert(idx != U32_MAX);

			zero8(bytes(_vals[_hash[idx].idx]), sizeof(val_type));
			zero8(bytes(_hash[idx]), sizeof(hash_));
			zero8(bytes(_keys[idx]), sizeof(key_type));
		}

		ref<val_type> operator[](cref<key_type> key) const {
			return get(key);
		}

		ref<val_type> operator[](cref<key_type> key) {
			u32 idx = find_(key);
			if (idx == U32_MAX) {
				u32 p = probe(key, hash_{ hash(key), _vals.size });
				if (p >= TABLE_PROBE) {
					resize(reserve * 2);
				}

				return _vals.add();
			}

			return _vals[_hash[idx].idx];
		}

		struct vals_view {
			vals_view(cref<vector<val_type>> in) : data(in) {}

			auto begin() {
				return data.begin();
			}

			auto end() {
				return data.end();
			}

			cref<vector<val_type>> data;
		};

		vals_view vals() {
			return vals_view(_vals);
		}

		vector<hash_> _hash;
		vector<key_type> _keys;
		vector<val_type> _vals;
		u32 reserve;
		u32 size;
	};
}
