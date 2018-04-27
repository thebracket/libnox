#pragma once

#include "region.hpp"
#include "../../noxtypes.h"

namespace region {	
	void initialize_chunks();
	void update_chunks();
	void update_chunks_listing_changes(std::vector<int> &dirty);
	void get_chunk_floors(const int &chunk_idx, const int &chunk_z, size_t &size, nf::floor_t *& floor_ptr);
	void get_chunk_cubes(const int &chunk_idx, const int &chunk_z, size_t &size, nf::cube_t *& cube_ptr);
	void get_chunk_models(const int &chunk_idx, std::vector<nf::static_model_t> &models);
	void get_chunk_coordinates(const int &idx, int &x, int &y, int &z);
	void mark_chunk_dirty_by_tileidx(const int &idx);
}