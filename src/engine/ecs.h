# pragma once

#include <core/vector.h>
#include <core/memory.h>

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
		, entities(0)
		, components(0)
		, busy() {

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
				core::set256(U32_MAX, bytes(sparse.data[start]), size);
			}

			sparse[e.id()] = dense.size;
			dense.add(e.id());
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
			components.del(index);
			sparse[dense[index]] = index;
			sparse[e.id()] = U32_MAX;
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
			}

			e_id* entity;
			type* component;
		};

		iterator begin() const {
			return iterator(&entities[0], &components[0]);
		}

		iterator end() const {
			return iterator(&entities[entities.size], &components[components.size]);
		}

		core::vector<u32> sparse;
		core::vector<u32> dense;
		core::vector<e_id> entities;
		core::vector<type> components;
		core::mutex busy;
	};

	// user should explicitly specify structures
	template<typename... T>
	struct group {};

	enum class ecs_event {
		create,
		destroy,
		add,
		del,
	};

	struct ecs {
		static constexpr u32 MAX_POOLS = BLOCK_64;

		ecs();
		~ecs();

		e_id create();
		void destroy(e_id e);

		template<typename T>
		void register() {
			core::lock lock(busy);
			JOLLY_ASSERT(pools.size < MAX_POOLS, "max pool size reached");
			u32 index = pools.size;
			pool_index<T>::value = index;
			auto& pool = pools.add();
			pool = core::ptr_create<pool>();
		}

		template<typename T>
		void add(e_id e, cref<T> item) {
			auto& pool = view<T>();
			pool.add(e, item);
		}

		template <typename T>
		ref<T> add(e_id e) {
			auto& pool = view<T>();
			return pool.add();
		}

		template<typename T>
		void del(e_id e) {
			auto& pool = view<T>();
			pool.del(e);
		}

		template<typename T>
		ref<T> get(e_id e) const {
			auto& pool = view<T>();
			return pool.get(e);
		}

		template<typename T>
		auto view() const {
			u32 index = pool_index<T>::value;
			if (index == U32_MAX)
				register<T>();
			return pools[index].ref<pool<T>>();
		}

		core::vector<e_id> entities;
		core::vector<core::any> pools;
		core::mutex busy;

		u32 free;
	};
};
