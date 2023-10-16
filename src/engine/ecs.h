# pragma once

#include <core/core.h>
#include <core/vector.h>
#include <core/memory.h>
#include <core/tuple.h>
#include <core/lock.h>

namespace jolly {
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
			return _id & id_mask;
		}

		u32 _id;
	};

	template <typename T>
	struct pool_index {
		static inline u32 value = U32_MAX;
	};

	template <typename T>
	struct pool {
		using type = T;

		pool()
		: sparse(0)
		, dense(0)
		, components(0)
		, busy() {
			core::set256(U32_MAX, bytes(sparse[0]), sparse.reserve);
			core::set256(U32_MAX, bytes(dense[0]), sparse.reserve);
		}

		~pool() = default;

		u32 _index(e_id e) {
			if (e.id() >= sparse.reserve) return U32_MAX;
			return sparse[e.id()];
		}

		void _add_entity(e_id e) {
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

		void add(e_id e, cref<type> item) {
			core::lock lock(busy);
			JOLLY_ASSERT(!has(e), "entity already contains this component");
			_add_entity(e);
			components.add(item);
		}

		ref<type> add(e_id e) {
			core::lock lock(busy);
			JOLLY_ASSERT(!has(e), "entity already contains this component");
			_add_entity(e);
			return components.add();
		}

		void del(e_id e) {
			core::lock lock(busy);
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			u32 index = _index(e);

			dense.del(index);
			dense[dense.size]._id = U32_MAX;

			components.del(index);
			sparse[e.id()] = U32_MAX;

			if (dense[index]._id != U32_MAX) {
				sparse[dense[index].id()] = index;
			}
		}

		bool has(e_id e) {
			return _index(e) != U32_MAX;
		}

		ref<T> get(e_id e) {
			JOLLY_ASSERT(has(e), "entity does not contain this component");
			return components[_index(e)];
		}

		struct iterator {
			using pair = core::pair<ref<e_id>, ref<type>>;

			iterator(e_id* _entity, type* _component)
			: entity(_entity), component(_component) {}

			bool operator!=(cref<iterator> other) const {
				return entity != other.entity;
			}

			pair operator*() const {
				return pair{ *entity, *component };
			}

			ref<iterator> operator++() {
				entity++;
				component++;
				return *this;
			}

			e_id* entity;
			type* component;
		};

		iterator begin() const {
			return iterator(&dense[0], &components[0]);
		}

		iterator end() const {
			return iterator(&dense[dense.size], &components[components.size]);
		}

		core::vector<u32> sparse;
		core::vector<e_id> dense;
		core::vector<type> components;
		core::mutex busy;
	};

	template <typename T, typename... T1>
	struct group_index {
		static inline u32 value = U32_MAX;

		static u32 fn() {
			JOLLY_ASSERT(pool_index<T>::value != U32_MAX, "pool<T> has not been instantiated, consider manually calling register_pool<T>");
			return (1 << pool_index<T>::value) | group_index<T1...>::fn();
		}
	};

	template <typename T>
	struct group_index<T> {
		static inline u32 value = U32_MAX;

		static u32 fn() {
			JOLLY_ASSERT(pool_index<T>::value != U32_MAX, "pool<T> has not been instantiated, consider manually calling register_pool<T>");
			return 1 << pool_index<T>::value;
		}
	};

	struct ecs;

	// user should explicitly specify structures
	// MUST hold pointers to ALL components
	template<typename... T>
	struct components {
		// user does not need to call lock state
		components(ref<ecs> state, e_id e) {
			static_assert(false, "must manually specialize all groups");
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

	struct ecs {
		static constexpr u32 MAX_POOLS = core::BLOCK_64;

		ecs();
		~ecs();

		e_id create();
		void destroy(e_id e);
		void callback(e_id e, ecs_event event);
		void add_cb(ecs_event event, pfn_ecs_cb cb);

		template<typename T>
		void register_pool() {
			core::lock lock(busy);
			JOLLY_ASSERT(pools.size < MAX_POOLS, "max pool size reached");
			pool_index<T>::value = pools.size;
			auto& p = pools.add();
			p = core::ptr_create<pool<T>>();

			auto destroy_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				auto& pool = state.view<T>();
				pool.del(e);
			};

			add_cb(ecs_event::destroy, destroy_cb);
		}

		template<typename... T>
		void register_group() {
			core::lock lock(busy);
			group_index<T...>::value = groups.size;
			auto& group_info = groups.add();
			group_info.one = core::ptr_create<pool<components<T...>>>();
			group_info.two = group_index<T...>::fn();
			auto& g = group_info.one.get<pool<components<T...>>>();

			for (u32 i : core::range(bitset.size)) {
				if ((bitset[i] & group_info.two) == group_info.two) {
					auto& data = g.add(entities[i]);
					data = components<T...>(*this, entities[i]);
				}
			}

			auto entity_add_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				u64 bits = state.groups[group_index<T...>::value].two;
				if ((state.bitset[e.id()] & bits) == bits) {
					auto& g = state.group<T...>();
					auto& data = g.add(e);
					data = components<T...>(state, e);
				}
			};

			auto entity_del_cb = [](ref<ecs> state, e_id e, ecs_event event) {
				auto& g = state.group<T...>();
				u64 bits = state.groups[group_index<T...>::value].two;
				if (g.has(e) && (state.bitset[e.id()] & bits) != bits) {
					g.del(e);
				}
			};

			add_cb(ecs_event::add, entity_add_cb);
			add_cb(ecs_event::del, entity_del_cb);
		}

		template<typename T>
		void add(e_id e, cref<T> item) {
			auto& pool = view<T>();
			pool.add(e, item);

			{
				core::lock lock(busy);
				bitset[e.id()] |= 1 << (pool_index<T>::value);
			}

			callback(e, ecs_event::add);
		}

		template <typename T>
		ref<T> add(e_id e) {
			auto& pool = view<T>();
			ref<T> tmp = pool.add();

			{
				core::lock lock(busy);
				bitset[e.id()] |= 1 << (pool_index<T>::value);
			}

			callback(e, ecs_event::add);
			return tmp;
		}

		template<typename T>
		void del(e_id e) {
			auto& pool = view<T>();
			pool.del(e);

			{
				core::lock lock(busy);
				bitset[e.id()] &= ~(1 << pool_index<T>::value);
			}

			callback(e, ecs_event::del);
		}

		template<typename T>
		ref<T> get(e_id e) {
			auto& pool = view<T>();
			return pool.get(e);
		}

		template<typename T>
		ref<pool<T>> view() {
			if (pool_index<T>::value == U32_MAX)
				register_pool<T>();
			return pools[pool_index<T>::value].get<pool<T>>();
		}

		template<typename... T>
		ref<pool<components<T...>>> group() {
			if (group_index<T...>::value == U32_MAX)
				register_group<T...>();

			core::lock lock(busy);
			return groups[group_index<T...>::value].one.get<pool<components<T...>>>();
		}

		core::vector<e_id> entities;
		core::vector<u64> bitset;
		core::vector<core::any> pools;
		core::vector<core::pair<core::any, u32>> groups;
		core::vector<core::vector<pfn_ecs_cb>> callbacks;
		core::mutex busy;

		u32 free;
	};
};
