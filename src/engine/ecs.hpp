module;

#include <core/core.h>

export module jolly.ecs;
import core.types;
import core.vector;
import core.simd;
import core.tuple;
import core.lock;
import core.traits;
import core.iterator;

namespace impl_ecs {
	template <typename S>
	struct iterator {
		iterator(cref<S> in, u32 idx)
		: data(in), index(idx) {}

		bool operator!=(cref<iterator> other) const {
			return index != other.index;
		}

		ref<iterator> operator++() {
			index++;
			return *this;
		}

		cref<S> data;
		u32 index;
	};

	template <typename S, typename Iterator>
	struct view_base {
		view_base(cref<S> in)
		: data(in) {}

		Iterator begin() const {
			return Iterator(data, 0);
		}

		Iterator end() const {
			return Iterator(data, data.dense.size);
		}

		cref<S> data;
	};
}

export namespace jolly {
	struct e_id {
		static constexpr u32 gen_mask = U8_MAX << 24;
		static constexpr u32 id_mask = U32_MAX & (~gen_mask);

		u32 gen() const {
			return (_id & gen_mask) >> 24;
		}

		u32 id() const {
			return _id & id_mask;
		}

		operator u32() const {
			return id();
		}

		u32 _id;
	};

	struct sparse_set {
		sparse_set()
		: sparse(0)
		, dense(0) {
			core::set256(U32_MAX, bytes(sparse[0]), sparse.reserve);
			core::set256(U32_MAX, bytes(dense[0]), sparse.reserve);
		}

		~sparse_set() = default;

		u32 index(e_id e) const {
			if (e.id() >= sparse.reserve) return U32_MAX;
			return sparse[e.id()];
		}

		void add(e_id e) {
			if (e.id() >= sparse.reserve) {
				u32 start = sparse.reserve;
				sparse.resize(e.id());
				u32 size = (sparse.reserve - start) * sizeof(u32);
				core::set256(U32_MAX, bytes(sparse[start]), size);
			}

			sparse[e.id()] = dense.size;

			u32 reserve = dense.reserve;
			e_id& entity = dense.add();
			if (reserve != dense.reserve) {
				core::set256(U32_MAX, bytes(dense[reserve]), dense.reserve - reserve);
			}

			entity = e;
		}

		void del(e_id e) {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			u32 idx = index(e);

			dense.del(idx);
			dense[dense.size]._id = U32_MAX;

			sparse[e.id()] = U32_MAX;

			if (dense[idx]._id != U32_MAX) {
				sparse[dense[idx].id()] = idx;
			}
		}

		bool has(e_id e) const {
			return index(e) != U32_MAX;
		}

		core::vector<u32> sparse;
		core::vector<e_id> dense;
	};

	template <typename T>
	struct pool {
		using type = T;
		using this_type = pool<type>;

		pool()
		: set()
		, components(0)
		, busy() {}

		~pool() = default;

		void add(e_id e, cref<type> item) {
			JOLLY_ASSERT(!has(e), "entity already contains this component");
			set.add(e);
			components.add(item);
		}

		ref<type> add(e_id e) {
			JOLLY_ASSERT(!has(e), "entity already contains this component");
			set.add(e);
			return components.add();
		}

		void del(e_id e) {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			u32 index = set.index(e);
			components.del(index);
			set.del(e);
		}

		bool has(e_id e) const {
			return set.has(e);
		}

		ref<T> get(e_id e) const {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			return components[set.index(e)];
		}

		cref<core::rwlock> get_lock() const {
			return busy;
		}

		struct iterator: public impl_ecs::iterator<this_type> {
			using parent_type = impl_ecs::iterator<this_type>;
			using pair_type = core::pair<e_id, core::wref<type>>;

			using parent_type::parent_type;

			pair_type operator*() const {
				u32 index = parent_type::index;
				auto& data = parent_type::data;
				return pair_type(data.set.dense[index], data.components[index]);
			}
		};

		auto begin() const {
			return iterator(*this, 0);
		}

		auto end() const {
			return iterator(*this, set.dense.size);
		}

		sparse_set set;
		core::vector<type> components;
		core::rwlock busy;

		static inline u64 index = U64_MAX;
	};

	struct ecs;

	template<typename T>
	cref<core::rwlock> get_view_lock(cref<ecs> state);

	// we choose not to cache pointers as it is expensive
	template <typename... Ts>
	struct group {
		using this_type = group<Ts...>;
		using tuple_type = core::tuple<core::wref<Ts>...>;
		using sequence_type = core::index_sequential<sizeof...(Ts)>;

		group(ref<ecs> in)
		: set()
		, state(in)
		, busy() {}

		~group() = default;

		void add(e_id e) {
			JOLLY_ASSERT(!has(e), "entity already contains this component");
			set.add(e);
		}

		void del(e_id e) {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			set.del(e);
		}

		bool has(e_id e) const {
			return set.index(e) != U32_MAX;
		}

		tuple_type get(e_id e) const {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			return tuple_type(state.get<Ts>(e)...);
		}

		cref<core::rwlock> get_lock() const {
			return busy;
		}

		struct iterator: public impl_ecs::iterator<this_type> {
			using parent_type = impl_ecs::iterator<this_type>;
			using pair_type = core::pair<e_id, tuple_type>;

			using parent_type::parent_type;

			pair_type operator*() const {
				u32 index = parent_type::index;
				auto& data = parent_type::data;
				e_id id = data.set.dense[index];
				return pair_type(id, data.get(id));
			}
		};

		auto begin() const {
			return iterator(*this, 0);
		}

		auto end() const {
			return iterator(*this, set.dense.size);
		}

		sparse_set set;
		ref<ecs> state;
		core::rwlock busy;

		static inline u64 index = U64_MAX;
		static u64 bitset() {
			return ((1 << pool<Ts>::index) | ...);
		}
	};

	enum class ecs_event {
		create = 0,
		destroy,
		add,
		del,

		max_event_size
	};

	typedef void (*pfn_ecs_cb)(ref<ecs> state, e_id e, ecs_event event);

	// DO NOT ACCESS DIRECTLY, obtain a rview/wview
	struct ecs {
		static constexpr u32 MAX_POOLS = core::BLOCK_64;

		ecs()
		: entities(0)
		, bitset(0)
		, pools(0)
		, groups(0)
		, callbacks((u32)ecs_event::max_event_size)
		, busy()
		, free(U32_MAX) {

		}

		~ecs() {
			// consider calling destroy callbacks for all entities
		}

		e_id create() {
			e_id entity{U32_MAX};
			if (free == U32_MAX) {
				u32 id = entities.size;

				e_id& e = entities.add();
				bitset.add();

				e._id = id; // generation = 0
				entity = e;
			} else {
				u32 id = free;
				e_id& e = entities[free];
				free = e.id() == (U32_MAX & e_id::id_mask) ? U32_MAX : e.id();
				e._id = ((e.gen() + 1) % U8_MAX) << 24 | id;
				entity = e;
			}

			callback(entity, ecs_event::create);
			return entity;
		}

		void destroy(e_id e) {
			callback(e, ecs_event::destroy);
			entities[e.id()]._id = (e.gen() << 24) | (free & e_id::id_mask);
			bitset[e.id()] = 0;
			free = e.id();
		}

		void callback(e_id e, ecs_event event) {
			for (auto cb : callbacks[(u32)event]) {
				cb(*this, e, event);
			}
		}

		void add_cb(ecs_event event, pfn_ecs_cb cb) {
			auto& vec = callbacks[(u32)event];
			if (!vec.data) {
				vec = core::vector<pfn_ecs_cb>(0);
			}

			vec.add(forward_data(cb));
		}

		template<typename T>
		void register_pool() {
			JOLLY_ASSERT(pools.size < MAX_POOLS, "max pool size reached");
			pool<T>::index = pools.size;
			auto& p = pools.add();
			p = core::mem_create<pool<T>>();

			auto destroy_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				auto& pool = state.view<T>();
				pool.del(e);
			};

			add_cb(ecs_event::destroy, destroy_cb);
		}

		template<typename... Ts>
		void register_group() {
			jolly::group<Ts...>::index = groups.size;
			auto& group_info = groups.add();
			group_info.one = core::mem_create<jolly::group<Ts...>>(*this);
			group_info.two = jolly::group<Ts...>::bitset();
			auto& g = group_info.one.get<jolly::group<Ts...>>();

			for (u32 i : core::range(bitset.size)) {
				if ((bitset[i] & group_info.two) == group_info.two) {
					g.add(entities[i]);
				}
			}

			auto entity_add_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				u64 bits = state.groups[(u32)jolly::group<Ts...>::index].two;
				if ((state.bitset[e.id()] & bits) == bits) {
					auto g = core::wview_create(state.group<Ts...>());
					g->add(e);
				}
			};

			auto entity_del_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				auto& g = state.group<Ts...>();
				u64 bits = state.groups[(u32)jolly::group<Ts...>::index].two;
				if (g.has(e) && (state.bitset[e.id()] & bits) != bits) {
					auto g = core::wview_create(state.group<Ts...>());
					g->del(e);
				}
			};

			add_cb(ecs_event::add, entity_add_cb);
			add_cb(ecs_event::del, entity_del_cb);
		}

		// we do not provide direct access to pool's other add overload as we can't guarantee atomicity
		template<typename T>
		void add(e_id e, T&& item) {
			{
				auto pool = core::wview_create(view<T>());
				pool->add(e, forward_data(item));
			}

			u64 bits = (u64)1 << pool<T>::index;
			bitset[e.id()] |= bits;
			callback(e, ecs_event::add);
		}

		template<typename T>
		void add(e_id e, cref<T> item) {
			add(e, forward_data(core::copy(item)));
		}

		template<typename T>
		void del(e_id e) {
			{
				auto pool = core::wview_create(view<T>());
				pool->del(e);
			}

			u64 bits = (u64)1 << pool<T>::index;
			bitset[e.id()] &= ~bits;
			callback(e, ecs_event::del);
		}

		// get functions do require additional synchronization
		// these are safe (do not require user synchronization) to use while inside a locking pool iterator
		template<typename T>
		ref<T> get(e_id e) const {
			auto pool = core::rview_create(view<T>());
			return pool->get(e);
		}

		cref<core::rwlock> get_lock() const {
			return busy;
		}

		template<typename T>
		bool has(e_id e) const {
			auto pool = core::rview_create(view<T>());
			return pool->has(e);
		}

		template<typename T>
		ref<pool<T>> view() {
			if (pool<T>::index == U64_MAX)
				register_pool<T>();
			return pools[(u32)pool<T>::index].get<pool<T>>();
		}

		template<typename T>
		cref<pool<T>> view() const {
			return pools[(u32)pool<T>::index].get<pool<T>>();
		}

		template<typename... Ts>
		ref<jolly::group<Ts...>> group() {
			if (jolly::group<Ts...>::index == U64_MAX)
				register_group<Ts...>();
			return groups[(u32)jolly::group<Ts...>::index].one.get<jolly::group<Ts...>>();
		}

		template<typename... Ts>
		cref<jolly::group<Ts...>> group() const {
			return groups[(u32)jolly::group<Ts...>::index].one.get<jolly::group<Ts...>>();
		}

		core::vector<e_id> entities;
		core::vector<u64> bitset;
		core::vector<core::any> pools;
		core::vector<core::pair<core::any, u64>> groups;
		core::vector<core::vector<pfn_ecs_cb>> callbacks;
		core::rwlock busy;

		u32 free;
	};

	template <typename T>
	cref<core::rwlock> get_view_lock(cref<ecs> state) {
		auto& view = state.view<T>();
		return view.get_lock();
	}
}

namespace jolly {
	template <typename Impl, typename... Ts>
	struct group_view_impl: public Impl {
		using this_type = group<Ts...>;
		static void acquire(cref<core::rwlock> l, ref<this_type> in) {
			auto helper = [](cref<core::rwlock> l, ref<this_type> in) {
				Impl::acquire(l, in);
				return 0;
			};

			Impl::acquire(l, in);
			(helper(get_view_lock<Ts>(in.state), in), ...);
		}

		static void release(cref<core::rwlock> l, ref<this_type> in) {
			auto helper = [](cref<core::rwlock> l, ref<this_type> in) {
				Impl::release(l, in);
				return 0;
			};

			Impl::release(l, in);
			(helper(get_view_lock<Ts>(in.state), in), ...);
		}
	};
}

template <typename... Ts>
using group_t = jolly::group<Ts...>;

template <typename... Ts>
using rview_impl_t = jolly::group_view_impl<core::rview_impl<group_t<Ts...>>, Ts...>;

template <typename... Ts>
using wview_impl_t = jolly::group_view_impl<core::wview_impl<group_t<Ts...>>, Ts...>;

export namespace core {
	template<typename... Ts>
	struct rview<group_t<Ts...>>: public view_base<group_t<Ts...>, rview_impl_t<Ts...>> {};

	template<typename... Ts>
	struct wview<group_t<Ts...>>: public view_base<group_t<Ts...>, wview_impl_t<Ts...>> {};
}
