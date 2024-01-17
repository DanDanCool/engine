module;

#include "core.h"

export module core.table;
import core.simd;
import core.types;
import core.tuple;
import core.vector;
import core.tuple;
import core.operations;
import core.iterator;
import core.traits;

export namespace core {
	constexpr u32 TABLE_PROBE = 24;

	u32 table_size(u32 sz);

	template<typename K, typename V>
	struct table {
		using key_type = K;
		using val_type = V;

		// key, hash, sparse
		using keymv_type = multi_vector<key_type, u32, u32>;
		// val, dense
		using valmv_type = multi_vector<val_type, u32>;

		static constexpr u32 KEY_INDEX = 0;
		static constexpr u32 HASH_INDEX = 1;
		static constexpr u32 SPARSE_INDEX = 2;
		static constexpr u32 VAL_INDEX = 0;
		static constexpr u32 DENSE_INDEX = 1;

		table(u32 sz = 0)
		: _keys(), _vals(0), reserve(table_size(max<u32>(sz, 100))), size(0) {
			_keys = forward_data(keymv_type(reserve));
		}

		~table() {
			reserve = 0;
			size = 0;
		}

		void resize(u32 sz) {
			sz = table_size(sz);

			keymv_type old = forward_data(_keys);
			_keys = keymv_type(sz);

			u32 count = reserve;
			reserve = sz;

			u32 p = 0;
			for (i32 i : range(count)) {
				u32 hash = old.get<HASH_INDEX>(i);
				if (!hash) continue;
				p = max(_probe(old.get<KEY_INDEX>(i), hash, old.get<SPARSE_INDEX>(i)), p);
			}

			if (p >= TABLE_PROBE) {
				resize(reserve * 2);
			}
		}

		u32 _probe(cref<key_type> k, u32 hash, u32 dense) {
			if (reserve <= size) {
				resize(reserve * 2);
			}

			key_type key = forward_data(copy(k));

			u32 probe = 0;
			u32 i = hash % reserve;

			while (true) {
				u32 cur = _keys.get<HASH_INDEX>(i);
				if (!cur) {
					_keys.get<HASH_INDEX>(i) = hash;
					_keys.get<KEY_INDEX>(i) = forward_data(key);
					_keys.get<SPARSE_INDEX>(i) = dense;
					_vals.get<DENSE_INDEX>(dense) = i;
					break;
				}

				u32 dist = (cur % reserve) - i;
				if (dist < probe) {
					_vals.get<DENSE_INDEX>(dense) = i;
					swap(_keys.get<HASH_INDEX>(i), hash);
					swap(_keys.get<KEY_INDEX>(i), key);
					swap(_keys.get<SPARSE_INDEX>(i), dense);
				}

				i = (i + 1) % reserve;
				probe++;
			}

			return probe;
		}

		u32 _hash(cref<key_type> key) const {
			u32 res = core::hash(key);
			return res | (res == 0);
		}

		u32 _find(cref<key_type> key) const {
			u32 h = _hash(key);
			for (i32 i : range(TABLE_PROBE)) {
				u32 idx = (h + i) % reserve;
				u32 tmp = _keys.get<HASH_INDEX>(idx);
				if (tmp == h) {
					if (key == _keys.get<KEY_INDEX>(idx)) return idx;
				}
			}

			return U32_MAX;
		}

		bool has(cref<key_type> key) const {
			return _find(key) != U32_MAX;
		}

		void set(cref<key_type> key, val_type&& val) {
			u32 idx = _find(key);
			if (idx != U32_MAX) {
				_vals.get<VAL_INDEX>(_keys.get<SPARSE_INDEX>(idx)) = forward_data(val);
				return;
			}

			u32 dense = _vals.size;
			_vals.add(forward_data(val), 0);
			u32 p = _probe(key, _hash(key), dense);
			size++;

			if (p >= TABLE_PROBE) {
				resize(reserve * 2);
			}
		}

		void set(cref<key_type> key, cref<val_type> val) {
			set(key, forward_data(copy(val)));
		}

		cref<val_type> get(cref<key_type> key) const {
			u32 idx = _find(key);
			JOLLY_ASSERT(idx != U32_MAX, "key does not exist in table");
			return _vals.get<VAL_INDEX>(_keys.get<SPARSE_INDEX>(idx));
		}

		ref<val_type> get(cref<key_type> key) {
			u32 idx = _find(key);
			JOLLY_ASSERT(idx != U32_MAX, "key does not exist in table");
			return _vals.get<VAL_INDEX>(_keys.get<SPARSE_INDEX>(idx));
		}

		cref<key_type> get_key(u32 idx) const {
			return _keys.get<KEY_INDEX>(idx);
		}

		ref<key_type> get_key(u32 idx) {
			return _keys.get<KEY_INDEX>(idx);
		}

		cref<val_type> get_val(u32 idx) const {
			return _vals.get<VAL_INDEX>(idx);
		}

		ref<val_type> get_val(u32 idx) {
			return _vals.get<VAL_INDEX>(idx);
		}

		void del(cref<key_type> key) {
			u32 sparse_idx = _find(key);
			JOLLY_ASSERT(sparse_idx != U32_MAX, "key does not exist in table");

			u32 dense_idx = _keys.get<SPARSE_INDEX>(sparse_idx);
			_vals.del(dense_idx);
			_keys.get<SPARSE_INDEX>(_vals.get<DENSE_INDEX>(dense_idx)) = dense_idx;

			ref<key_type> _key = _keys.get<KEY_INDEX>(sparse_idx);
			core::destroy(&_key);
			zero8(bytes(_key), sizeof(key_type));

			_keys.get<HASH_INDEX>(sparse_idx) = 0;
			_keys.get<SPARSE_INDEX>(sparse_idx) = 0;

			size--;
		}

		cref<val_type> operator[](cref<key_type> key) const {
			return get(key);
		}

		ref<val_type> operator[](cref<key_type> key) {
			u32 idx = _find(key);
			if (idx == U32_MAX) {
				set(key, forward_data(val_type()));
				return get(key);
			}

			return _vals.get<VAL_INDEX>(_keys.get<SPARSE_INDEX>(idx));
		}

		struct iterator_base {
			iterator_base(cref<keymv_type> _keys, cref<valmv_type> _vals, u32 idx)
			: keys(_keys), vals(_vals), index(idx) {}

			ref<iterator_base> operator++() {
				index++;
				return *this;
			}

			bool operator!=(cref<iterator_base> other) const {
				return index != other.index;
			}

			cref<keymv_type> keys;
			cref<valmv_type> vals;
			u32 index;
		};

		template <typename Iterator>
		struct view_base {
			view_base(cref<keymv_type> _keys, cref<valmv_type> _vals)
			: keys(_keys), vals(_vals) {}

			auto begin() {
				return Iterator(keys, vals, 0);
			}

			auto end() {
				return Iterator(keys, vals, vals.size);
			}

			cref<keymv_type> keys;
			cref<valmv_type> vals;
		};

		struct iterator_keys: public iterator_base {
			using iterator_base::iterator_base;
			ref<key_type> operator*() const {
				u32 index = iterator_base::index;
				auto& keys = iterator_base::keys;
				auto& vals = iterator_base::vals;
				return keys.get<KEY_INDEX>(vals.get<DENSE_INDEX>(index));
			}
		};

		struct iterator_vals: public iterator_base {
			using iterator_base::iterator_base;
			ref<val_type> operator*() const {
				u32 index = iterator_base::index;
				auto& keys = iterator_base::keys;
				auto& vals = iterator_base::vals;
				return vals.get<VAL_INDEX>(index);
			}
		};

		struct iterator_items: iterator_base {
			using pair_type = pair<wref<key_type>, wref<val_type>>;
			using iterator_base::iterator_base;

			pair_type operator*() const {
				u32 index = iterator_base::index;
				auto& keys = iterator_base::keys;
				auto& vals = iterator_base::vals;
				return pair_type(keys.get<KEY_INDEX>(vals.get<DENSE_INDEX>(index)), vals.get<VAL_INDEX>(index));
			}
		};

		auto keys() {
			return view_base<iterator_keys>(_keys, _vals);
		}

		auto vals() {
			return view_base<iterator_vals>(_keys, _vals);
		}

		auto begin() {
			return iterator_items(_keys, _vals, 0);
		}

		auto end() {
			return iterator_items(_keys, _vals, size);
		}

		keymv_type _keys;
		valmv_type _vals;
		u32 reserve, size;
	};

	constexpr u32 PRIME_ARRAY[] = {
		187091u,     1289u,       28802401u,   149u,        15173u,      2320627u,    357502601u,  53u,
		409u,        4349u,       53201u,      658753u,     8175383u,    101473717u,  1259520799u, 19u,
		89u,         241u,        709u,        2357u,       8123u,       28411u,      99733u,      351061u,
		1236397u,    4355707u,    15345007u,   54061849u,   190465427u,  671030513u,  2364114217u, 7u,
		37u,         71u,         113u,        193u,        313u,        541u,        953u,        1741u,
		3209u,       5953u,       11113u,      20753u,      38873u,      72817u,      136607u,     256279u,
		480881u,     902483u,     1693859u,    3179303u,    5967347u,    11200489u,   21023161u,   39460231u,
		74066549u,   139022417u,  260944219u,  489790921u,  919334987u,  1725587117u, 3238918481u, 3u,
		13u,         29u,         43u,         61u,         79u,         103u,        137u,        167u,
		211u,        277u,        359u,        467u,        619u,        823u,        1109u,       1493u,
		2029u,       2753u,       3739u,       5087u,       6949u,       9497u,       12983u,      17749u,
		24281u,      33223u,      45481u,      62233u,      85229u,      116731u,     159871u,     218971u,
		299951u,     410857u,     562841u,     771049u,     1056323u,    1447153u,    1982627u,    2716249u,
		3721303u,    5098259u,    6984629u,    9569143u,    13109983u,   17961079u,   24607243u,   33712729u,
		46187573u,   63278561u,   86693767u,   118773397u,  162723577u,  222936881u,  305431229u,  418451333u,
		573292817u,  785430967u,  1076067617u, 1474249943u, 2019773507u, 2767159799u, 3791104843u, 2u,
		5u,          11u,         17u,         23u,         31u,         41u,         47u,         59u,
		67u,         73u,         83u,         97u,         109u,        127u,        139u,        157u,
		179u,        199u,        227u,        257u,        293u,        337u,        383u,        439u,
		503u,        577u,        661u,        761u,        887u,        1031u,       1193u,       1381u,
		1613u,       1879u,       2179u,       2549u,       2971u,       3469u,       4027u,       4703u,
		5503u,       6427u,       7517u,       8783u,       10273u,      12011u,      14033u,      16411u,
		19183u,      22447u,      26267u,      30727u,      35933u,      42043u,      49201u,      57557u,
		67307u,      78779u,      92203u,      107897u,     126271u,     147793u,     172933u,     202409u,
		236897u,     277261u,     324503u,     379787u,     444487u,     520241u,     608903u,     712697u,
		834181u,     976369u,     1142821u,    1337629u,    1565659u,    1832561u,    2144977u,    2510653u,
		2938679u,    3439651u,    4026031u,    4712381u,    5515729u,    6456007u,    7556579u,    8844859u,
		10352717u,   12117689u,   14183539u,   16601593u,   19431899u,   22744717u,   26622317u,   31160981u,
		36473443u,   42691603u,   49969847u,   58488943u,   68460391u,   80131819u,   93793069u,   109783337u,
		128499677u,  150406843u,  176048909u,  206062531u,  241193053u,  282312799u,  330442829u,  386778277u,
		452718089u,  529899637u,  620239453u,  725980837u,  849749479u,  994618837u,  1164186217u, 1362662261u,
		1594975441u, 1866894511u, 2185171673u, 2557710269u, 2993761039u, 3504151727u, 4101556399u
	};

	u32 table_size(u32 sz) {
		u32 best = U32_MAX;
		u32 cur  = 0;

		for (int i = 0; i < 7; i++)	{
			u32 prime = PRIME_ARRAY[cur];
			if (sz < prime) {
				cur = 2 * cur + 1;
			} else {
				cur = 2 * cur + 2;
			}

			if (sz <= prime && prime < best) {
				best = prime;
			}
		}

		return best;
	}
}
