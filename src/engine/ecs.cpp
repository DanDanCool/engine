module;

#include <core/core.h>

export module jolly.ecs;
import core.vector;
import core.simd;
import core.tuple;
import core.lock;

namespace impl_ecs {
	template <typename S, typename View>
	struct iterator {
		iterator(cref<S> in, u32 idx, bool b)
		: data(in), index(idx), should_lock(b) {
			if (should_lock)
				View::acquire(data.busy);
		}

		~iterator() {
			if (should_lock)
				View::release(data.busy);
		}

		bool operator!=(cref<iterator> other) const {
			return index != other.index;
		}

		ref<iterator> operator++() {
			index++;
			return *this;
		}

		cref<S> data;
		u32 index;
		bool should_lock;
	};

	template <typename S, typename Iterator>
	struct view_base {
		view_base(cref<S> in)
		: data(in) {}

		Iterator begin() const {
			return Iterator(data, 0, true);
		}

		Iterator end() const {
			return Iterator(data, data.dense.size, false);
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
		: dense(0) {
			core::set256(U32_MAX, bytes(sparse[0]), sparse.reserve);
			core::set256(U32_MAX, bytes(dense[0]), sparse.reserve);
		}

		~group() = default;

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
			u32 index = index(e);

			dense.del(index);
			dense[dense.size]._id = U32_MAX;

			sparse[e.id()] = U32_MAX;

			if (dense[index]._id != U32_MAX) {
				sparse[dense[index].id()] = index;
			}
		}

		bool has(e_id e) const {
			core::lock lock(busy.read())
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
		, busy() {
			core::set256(U32_MAX, bytes(sparse[0]), sparse.reserve);
			core::set256(U32_MAX, bytes(dense[0]), sparse.reserve);
		}

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

		ref<core::rwlock> get_lock() const {
			return busy;
		}

		template <typename View>
		struct iterator: public impl_ecs::iterator<this_type, View> {
			using pair_type = core::pair<e_id, ref<type>>;
			pair_type operator*() const {
				return pair_type(data.set.dense[index], data.components[index]);
			}
		};

		auto rview() const {
			return impl_ecs::view_base<this_type, iterator<core::rview_impl>>(*this);
		}

		auto wview() const {
			return impl_ecs::view_base<this_type, iterator<core::wview_impl>>(*this);
		}

		sparse_set set;
		core::vector<type> components;
		core::rwlock busy;

		static u32 index = U32_MAX;
	};

	struct ecs;

	// we choose not to cache pointers as it is expensive
	template <typename... Ts>
	struct group {
		using this_type = group<Ts...>;
		using tuple_type = core::tuple<Ts&...>;
		using sequence_type = index_sequential<sizeof...(Ts)>;

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
			set.del(e)
		}

		bool has(e_id e) const {
			return set.index(e) != U32_MAX;
		}

		tuple_type get(e_id e) const {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			return tuple_type(state.get<Ts>(e)...);
		}

		ref<core::rwlock> get_lock() const {
			return busy;
		}

		template<typename View>
		struct iterator: public impl_ecs::iterator<this_type, View> {
			using pair_type = core::pair<e_id, tuple_type>;
			iterator(cref<this_type> in, u32 idx, bool b)
			: impl_ecs::iterator(in, idx, b) {
				auto helper = [](ref<core::rwlock> l) {
					View::acquire(l);
					return 0;
				};

				if (should_lock) {
					core::swallow(helper(data.state.view<Ts>().get_lock())...);
				}
			}

			~iterator() {
				auto helper = [](ref<core::rwlock> l) {
					View::release(l);
					return 0;
				};

				if (should_lock) {
					core::swallow(helper(data.state.view<Ts>().get_lock())...);
				}

				impl_ecs::~iterator();
			}

			pair_type operator*() const {
				e_id id = data.set.dense[index];
				return pair_type(id, tuple_type(data.get(id));
			}
		};

		auto read() const {
			return impl_ecs::view_base<this_type, iterator<impl_ecs::read_view>>();
		}

		auto write() const {
			return impl_ecs::view_base<this_type, iterator<impl_ecs::write_view>>();
		}

		sparse_set set;
		ref<ecs> state;
		core::rwlock busy;

		static u32 index = U32_MAX;
		static u32 bitset() {
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
				core::lock lock(busy);
				u32 id = entities.size;

				e_id& e = entities.add();
				bitset.add();

				e._id = id; // generation = 0
				entity = e;
			} else {
				core::lock lock(busy);
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

		void ecs::add_cb(ecs_event event, pfn_ecs_cb cb) {
			auto& vec = callbacks[(u32)event];
			if (!vec.data) {
				vec = core::vector<pfn_ecs_cb>(0);
			}

			vec.add(cb);
		}

		template<typename T>
		void register_pool() {
			JOLLY_ASSERT(pools.size < MAX_POOLS, "max pool size reached");
			pool<T>::index = pools.size;
			auto& p = pools.add();
			p = core::ptr_create<pool<T>>();

			auto destroy_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				auto& pool = state.view<T>();
				pool.del(e);
			};

			add_cb(ecs_event::destroy, destroy_cb);
		}

		template<typename... Ts>
		void register_group() {
			group<Ts...>::index = groups.size;
			auto& group_info = groups.add();
			group_info.one = core::ptr_create<group<Ts...>>();
			group_info.two = group<Ts...>::bitset();
			auto& g = group_info.one.get<group<T...>>();

			for (u32 i : core::range(bitset.size)) {
				if ((bitset[i] & group_info.two) == group_info.two) {
					g.add(entities[i]);
				}
			}

			auto entity_add_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				u64 bits = state.groups[group<Ts...>::index].two;
				if ((state.bitset[e.id()] & bits) == bits) {
					g.add(e);
				}
			};

			auto entity_del_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				auto& g = state.group<T...>();
				u64 bits = state.groups[group<Ts...>::index].two;
				if (g.has(e) && (state.bitset[e.id()] & bits) != bits) {
					g.del(e);
				}
			};

			add_cb(ecs_event::add, entity_add_cb);
			add_cb(ecs_event::del, entity_del_cb);
		}

		// we do not provide direct access to pool's other add overload as we can't guarantee atomicity
		template<typename T>
		void add(e_id e, T&& item) {
			{
				auto pool = core::wview(view<T>());
				pool->add(e, forward_data(item));
			}

			bitset[e.id()] |= 1 << (pool<T>::index);
			callback(e, ecs_event::add);
			return tmp;
		}

		template<typename T>
		void del(e_id e) {
			{
				auto pool = core::wview(view<T>());
				pool->del(e);
			}
			bitset[e.id()] &= ~(1 << pool<T>::index);
			callback(e, ecs_event::del);
		}

		// get functions do require additional synchronization
		// these are safe (do not require user synchronization) to use while inside a locking pool iterator
		template<typename T>
		ref<T> get(e_id e) const {
			auto pool = core::rview(view<T>());
			return pool->get(e);
		}

		ref<core::rwlock> get_lock() const {
			return busy;
		}

		template<typename T>
		bool has(e_id e) const {
			auto& pool = core::rview(view<T>());
			return pool->has(e);
		}

		template<typename T>
		ref<pool<T>> view() {
			if (pool<T>::index == U32_MAX)
				register_pool<T>();
			return pools[pool<T>::index].get<pool<T>>();
		}

		template<typename T>
		ref<pool<T>> view() const {
			return pools[pool<T>::index].get<pool<T>>();
		}

		template<typename... Ts>
		ref<group<Ts...>> group() {
			if (group<Ts...>::index == U32_MAX)
				register_group<Ts...>();
			return groups[group<Ts...>::index].one.get<group<Ts...>>();
		}

		template<typename... Ts>
		ref<group<Ts...>> group() const {
			return groups[group<Ts...>::index].one.get<group<Ts...>>();
		}

		core::vector<e_id> entities;
		core::vector<u64> bitset;
		core::vector<core::any> pools;
		core::vector<core::pair<core::any, u64>> groups;
		core::vector<core::vector<pfn_ecs_cb>> callbacks;
		core::rwlock busy;

		u32 free;
	};
}
