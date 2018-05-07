#include "libnox.h"
#include "planet/region/region.hpp"
#include "planet/region/region_chunking.hpp"
#include "nox_impl_helpers.hpp"
#include <vector>

namespace nf {
	namespace impl {
		static std::vector<static_model_t> model_list;
		static std::vector<int> dirty_list;
	}

	void chunks_init() {
		region::initialize_chunks();
	}

	void chunks_update() {
		region::update_chunks();
	}


	void chunks_update_list_dirty(size_t &size, int *& dirty_ptr) {
		impl::dirty_list.clear();
		region::update_chunks_listing_changes(impl::dirty_list);
		ArrayToUnrealPtr<int>(size, dirty_ptr, impl::dirty_list);
	}

	void chunk_floors(const int &chunk_idx, const int &chunk_z, size_t &size, floor_t *& floor_ptr) {
		region::get_chunk_floors(chunk_idx, chunk_z, size, floor_ptr);
	}

	void chunk_cubes(const int &chunk_idx, const int &chunk_z, size_t &size, cube_t *& cube_ptr) {
		region::get_chunk_cubes(chunk_idx, chunk_z, size, cube_ptr);
	}


	void chunk_models(const int &chunk_idx, size_t &size, static_model_t *& model_ptr) {
		impl::model_list.clear();
		region::get_chunk_models(chunk_idx, impl::model_list);
		size = impl::model_list.size();
		model_ptr = size > 0 ? &impl::model_list[0] : nullptr;
	}

	void chunk_world_coordinates(const int &idx, int &x, int &y, int &z) {
		region::get_chunk_coordinates(idx, x, y, z);
	}
}