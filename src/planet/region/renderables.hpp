#pragma once

#include "../../noxtypes.h"
#include <vector>

namespace render {
	void build_voxel_list(int selected_building, int mouse_x, int mouse_y, int mouse_z);
	void get_model_list(std::vector<nf::dynamic_model_t> &models);
	void invalidate_composite_cache_for_entity(const int &id);
}