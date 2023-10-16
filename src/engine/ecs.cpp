#include "ecs.h"

namespace jolly {
	ecs::ecs()
	: entities(0)
	, bitset(0)
	, pools(0)
	, groups(0)
	, callbacks((u32)ecs_event::max_event_size)
	, busy()
	, free(U32_MAX) {

	}

	ecs::~ecs() {
		// consider calling destroy callbacks for all entities
	}

	e_id ecs::create() {
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

	void ecs::destroy(e_id e) {
		callback(e, ecs_event::destroy);
		core::lock lock(busy);
		entities[e.id()]._id = (e.gen() << 24) | (free & e_id::id_mask);
		bitset[e.id()] = 0;
		free = e.id();
	}

	void ecs::callback(e_id e, ecs_event event) {
		core::lock lock(busy);
		for (auto cb : callbacks[(u32)event]) {
			cb(*this, e, event);
		}
	}

	void ecs::add_cb(ecs_event event, pfn_ecs_cb cb) {
		core::lock lock(busy);
		auto& vec = callbacks[(u32)event];
		if (!vec.data) {
			vec = core::vector<pfn_ecs_cb>(0);
		}

		vec.add(cb);
	}
}
