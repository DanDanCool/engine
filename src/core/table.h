#pragma once

#include "tuple.h"
#include "vector.h"

namespace core {
	constexpr u32 TABLE_PROBE = 24;

	u32 table_size(u32 sz);

	template<typename K, typename V>
	struct table {
		using key_type = K;
		using val_type = V;

		table(u32 sz = 0)
		: _hash(), _keys(), _vals(0), reserve(0), size(0), _sparse(), _dense(0)  {
			sz = table_size(max<u32>(sz, 100));
			_sparse = vector<u32>(sz);
			_hash = vector<u32>(sz);
			_keys = vector<key_type>(sz);
			reserve = sz;
		}

		~table() {
			reserve = 0;
			size = 0;
		}

		void resize(u32 sz) {
			sz = table_size(sz);

			vector<u32> nhash = _hash;
			vector<key_type> nkeys = _keys;
			vector<u32> nsparse = _sparse;

			_hash = vector<u32>(sz);
			_keys = vector<key_type>(sz);
			_sparse = vector<u32>(sz);

			u32 count = reserve;
			reserve = sz;
			for (i32 i : range(count)) {
				u32 hash = nhash[i];
				if (!hash) continue;
				_probe(nkeys[i], hash, nsparse[i]);
			}
		}

		u32 _probe(key_type key, u32 hash, u32 dense) {
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
					_sparse[i] = dense;
					_dense[dense] = i;
					break;
				}

				u32 dist = (cur % reserve) - i;
				if (dist < probe) {
					_dense[dense] = i;
					swap(_hash[i], hash);
					swap(_keys[i], key);
					swap(_sparse[i], dense);
				}

				i = (i + 1) % reserve;
				probe++;
			}

			return probe;
		}

		u32 _find(cref<key_type> key) const {
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

		bool has(cref<key_type> key) const {
			return _find(key) != U32_MAX;
		}

		void set(cref<key_type> key, cref<val_type> val) {
			u32 idx = _find(key);
			if (idx != U32_MAX) {
				_vals[_sparse[idx]] = val;
				return;
			}

			_dense.add() = 0;
			u32 p = _probe(key, hash(key), _vals.size);
			_vals.add(val);
			size++;

			if (p >= TABLE_PROBE) {
				resize(reserve * 2);
			}
		}

		ref<val_type> get(cref<key_type> key) const {
			u32 idx = _find(key);
			JOLLY_ASSERT(idx != U32_MAX, "key does not exist in table");
			return _vals[_sparse[idx]];
		}

		void del(cref<key_type> key) {
			u32 idx = _find(key);
			JOLLY_ASSERT(idx != U32_MAX, "key does not exist in table");

			_dense.del(_sparse[idx]);
			_vals.del(_sparse[idx]);
			_sparse[_dense[_sparse[idx]]] = _sparse[idx];

			ref<key_type> _key = _keys[idx];
			core::destroy(&_key);
			zero8(bytes(_key), sizeof(key_type));

			_hash[idx] = 0;
			_sparse[idx] = 0;

			size--;
		}

		ref<val_type> operator[](cref<key_type> key) const {
			return get(key);
		}

		ref<val_type> operator[](cref<key_type> key) {
			u32 idx = _find(key);
			if (idx == U32_MAX) {
				_dense.add() = 0;
				u32 p = _probe(key, hash(key), _vals.size);
				if (p >= TABLE_PROBE) {
					resize(reserve * 2);
				}

				size++;
				return _vals.add();
			}

			return _vals[_sparse[idx]];
		}

		struct key_view {
			key_view(cref<vector<key_type>> _keys, cref<vector<u32>> _dense)
			: keys(_keys), dense(_dense) {}

			struct iterator {
				iterator(cref<vector<key_type>> _keys, u32* _idx) : keys(_keys), idx(_idx) {}

				ref<iterator> operator++() {
					idx++;
					return *this;
				}

				bool operator!=(cref<iterator> other) const {
					return idx != other.idx;
				}

				ref<key_type> operator*() const {
					return keys[*idx];
				}

				cref<vector<key_type>> keys;
				u32* idx;
			};

			auto begin() {
				return iterator(keys, &dense[0]);
			}

			auto end() {
				return iterator(keys, &dense[dense.size]);
			}

			cref<vector<key_type>> keys;
			cref<vector<u32>> dense;
		};

		struct val_view {
			val_view(cref<vector<val_type>> in) : data(in) {}

			auto begin() {
				return data.begin();
			}

			auto end() {
				return data.end();
			}

			cref<vector<val_type>> data;
		};

		struct iterator {
			using pair_type = pair<ref<key_type>, ref<val_type>>;

			iterator(cref<vector<key_type>> _keys, cref<vector<val_type>> _vals, cref<vector<u32>> _dense, u32 _idx)
			: keys(_keys), vals(_vals), dense(_dense), idx(_idx) {}

			ref<iterator> operator++() {
				idx++;
				return *this;
			}

			bool operator!=(cref<iterator> other) const {
				return idx != other.idx;
			}

			pair_type operator*() const {
				return pair_type(keys[dense[idx]], vals[idx]);
			}

			cref<vector<key_type>> keys;
			cref<vector<val_type>> vals;
			cref<vector<u32>> dense;
			u32 idx;
		};

		key_view keys() {
			return key_view(_keys, _dense);
		}

		val_view vals() {
			return val_view(_vals);
		}

		auto begin() {
			return iterator(_keys, _vals, _dense, 0);
		}

		auto end() {
			return iterator(_keys, _vals, _dense, size);
		}

		vector<u32> _hash;
		vector<key_type> _keys;
		vector<val_type> _vals;
		vector<u32> _sparse;
		vector<u32> _dense;
		u32 reserve;
		u32 size;
	};
}
