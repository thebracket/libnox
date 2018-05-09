#include "libnox.h"
#include "libnox-render.hpp"
#include "global_assets/game_camera.hpp"
#include "noxconsts.h"
#include "planet/region/renderables.hpp"
#include "planet/region/lighting.hpp"
#include "planet/region/region.hpp"
#include "global_assets/game_ecs.hpp"
#include "global_assets/game_mode.hpp"
#include "global_assets/game_designations.hpp"
#include "global_assets/farming_designations.hpp"
#include "nox_impl_helpers.hpp"
#include <vector>

namespace nf {

	namespace impl {
		static std::vector<dynamic_model_t> dyn_models;
		static std::vector<dynamic_lightsource_t> dyn_lights;
		static std::vector<water_t> water;
		static std::vector<cube_t> cursors;
	}

	int mouse_x = 0;
	int mouse_y = 0;
	int mouse_z = 0;
	int selected_building = 0;

	void get_camera_position(float &x, float &y, float &z, float &zoom, bool &perspective, int &mode) {
		x = camera_position->region_x;
		y = camera_position->region_y;
		z = camera_position->region_z;
		zoom = camera->zoom_level;
		perspective = camera->perspective;
		mode = camera->camera_mode;
	}

	void camera_zoom_in() {
		--camera->zoom_level;
		if (camera->zoom_level < 1) camera->zoom_level = 1;
	}

	void camera_zoom_out() {
		++camera->zoom_level;
		if (camera->zoom_level > 150) camera->zoom_level = 150;
	}

	void camera_move(const int &x, const int &y, const int &z) {
		camera_position->region_x += x;
		camera_position->region_y += y;
		camera_position->region_z += z;

		if (camera_position->region_x < 1) camera_position->region_x = 1;
		if (camera_position->region_x > REGION_WIDTH - 1) camera_position->region_x = REGION_WIDTH - 1;
		if (camera_position->region_y < 1) camera_position->region_y = 1;
		if (camera_position->region_y > REGION_HEIGHT - 1) camera_position->region_y = REGION_HEIGHT - 1;
		if (camera_position->region_z < 1) camera_position->region_z = 1;
		if (camera_position->region_z > REGION_DEPTH - 1) camera_position->region_z = REGION_DEPTH - 1;
	}

	void toggle_camera_mode() {
		switch (camera->camera_mode) {
		case game_camera_mode_t::DIAGONAL_LOOK_NW: {
			camera->camera_mode = game_camera_mode_t::DIAGONAL_LOOK_NE;
		}break;
		case game_camera_mode_t::DIAGONAL_LOOK_NE: {
			camera->camera_mode = game_camera_mode_t::DIAGONAL_LOOK_SW;
		}break;
		case game_camera_mode_t::DIAGONAL_LOOK_SW: {
			camera->camera_mode = game_camera_mode_t::DIAGONAL_LOOK_SE;
		}break;
		case game_camera_mode_t::DIAGONAL_LOOK_SE: {
			camera->camera_mode = game_camera_mode_t::FRONT;
		}break;
		case game_camera_mode_t::FRONT: {
			camera->camera_mode = game_camera_mode_t::TOP_DOWN;
		} break;
		case game_camera_mode_t::TOP_DOWN: {
			camera->camera_mode = game_camera_mode_t::DIAGONAL_LOOK_NW;
		} break;
		default: {} break; // We're ignoring the FPS modes
		}
	}

	void toggle_camera_perspective() {
		camera->perspective = !camera->perspective;
	}

	void voxel_render_list(size_t &size, dynamic_model_t *& model_ptr) {
		impl::dyn_models.clear();
		render::build_voxel_list(selected_building, mouse_x, mouse_y, mouse_z);
		render::get_model_list(impl::dyn_models);
		ArrayToUnrealPtr<dynamic_model_t>(size, model_ptr, impl::dyn_models);
	}

	void lightsource_list(size_t &size, dynamic_lightsource_t *& light_ptr) {
		impl::dyn_lights.clear();
		render::get_light_list(impl::dyn_lights);
		ArrayToUnrealPtr<dynamic_lightsource_t>(size, light_ptr, impl::dyn_lights);
	}

	void water_cubes(size_t &size, water_t *& water_ptr) {
		impl::water.clear();
		std::vector<uint32_t> * w = region::get_water_level();
		for (int z = 0; z < REGION_DEPTH; ++z) {
			for (int y = 0; y < REGION_HEIGHT; ++y) {
				for (int x = 0; x < REGION_WIDTH; ++x) {
					const int idx = mapidx(x, y, z);
					const uint32_t wl = w->operator[](idx);
					if (wl > 0) {
						impl::water.emplace_back(water_t{ (float)x, (float)y, (float)z, ((float)wl / 10.0f) });
					}
				}
			}
		}
		ArrayToUnrealPtr<water_t>(size, water_ptr, impl::water);
	}

	void zoom_settler(int id) {
		auto settler = bengine::entity(id);
		if (settler) {
			auto pos = settler->component<position_t>();
			if (pos) {
				camera_position->region_x = pos->x;
				camera_position->region_y = pos->y;
				camera_position->region_z = pos->z;
			}
		}
	}

	void follow_settler(int id) {
		auto settler = bengine::entity(id);
		if (settler) {
			auto pos = settler->component<position_t>();
			if (pos) {
				camera_position->region_x = pos->x;
				camera_position->region_y = pos->y;
				camera_position->region_z = pos->z;
				camera->following = id;
			}
		}
	}

	void cursor_list(size_t &size, cube_t *& cube_ptr) {
		impl::cursors.clear();

		// Normal cursor
		impl::cursors.emplace_back(cube_t{ mouse_x, mouse_y, mouse_z, 1, 1, 1, 1 });

		if (game_master_mode == DESIGN) {
			if (game_design_mode == GUARDPOINTS) {
				for (const auto &gp : designations->guard_points) {
					impl::cursors.emplace_back(cube_t{ gp.second.x, gp.second.y, gp.second.z, 1, 1, 1, 3 });
				}
			}
			else if (game_design_mode == CHOPPING) {
				for (size_t i = 0; i < REGION_TILES_COUNT; ++i) {
					auto tree_id = region::tree_id(i);
					if (tree_id > 0) {
						auto[x, y, z] = idxmap(i);
						if (designations->chopping.find(tree_id) != designations->chopping.end()) {
							impl::cursors.emplace_back(cube_t{ x, y, z, 1, 1, 1, 2 });
							//std::cout << "Highlighting tree at " << x << ", " << y << ", " << z << "\n";
						}
						//else {
						//	add_cube_geometry(data, n_elements_cursor_elements, x, y, z, 1, 1, 2);
						//}
					}
				}
			}
			else if (game_design_mode == HARVEST) {
				for (const auto &idx : farm_designations->harvest) {					
					impl::cursors.emplace_back(cube_t{ idx.second.x, idx.second.y, idx.second.z, 1, 1, 1, 4 });
				}
			}
		}

		ArrayToUnrealPtr<cube_t>(size, cube_ptr, impl::cursors);
	}
}