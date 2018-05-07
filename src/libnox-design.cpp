#include "libnox.h"
#include "libnox-render.hpp"
#include "planet/region/region.hpp"
#include "global_assets/game_designations.hpp"
#include "global_assets/game_camera.hpp"
#include "global_assets/building_designations.hpp"
#include "global_assets/game_building.hpp"
#include "systems/helpers/inventory_assistant.hpp"
#include "raws/buildings.hpp"
#include "raws/defs/building_def_t.hpp"
#include "nox_impl_helpers.hpp"
#include <utility>
#include <algorithm>

namespace nf {

	namespace impl {
		std::vector<buildable_building_t> available_buildings;
	}

	void guardmode_set() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		if (region::flag(idx, tile_flags::CAN_STAND_HERE)) {
			bool found = false;
			for (const auto &g : designations->guard_points) {
				if (mapidx(g.second) == idx) found = true;
			}
			if (!found) designations->guard_points.push_back(std::make_pair(false, position_t{ mouse_x, mouse_y, mouse_z }));
		}
	}

	void guardmode_clear() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		if (region::flag(idx, tile_flags::CAN_STAND_HERE)) {
			designations->guard_points.erase(std::remove_if(
				designations->guard_points.begin(),
				designations->guard_points.end(),
				[&idx](std::pair<bool, position_t> p) { return idx == mapidx(p.second); }
			),
				designations->guard_points.end());
		}
	}

	void lumberjack_set() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		const auto tree_id = region::tree_id(idx);
		if (tree_id > 0) {
			int lowest_z = camera_position->region_z;
			const int stop_z = lowest_z - 10;

			position_t tree_pos{ mouse_x, mouse_y, lowest_z };
			while (lowest_z > stop_z) {
				for (int y = -10; y<10; ++y) {
					for (int x = -10; x<10; ++x) {
						const int tree_idx = mapidx(mouse_x + x, mouse_y + y, lowest_z);
						if (region::tree_id(tree_idx) == tree_id) {
							tree_pos.x = mouse_x + x;
							tree_pos.y = mouse_y + y;
							tree_pos.z = lowest_z;
							// TODO: Emit Particles
						}
					}
				}
				--lowest_z;
			}

			designations->chopping[(int)tree_id] = tree_pos;
		}
	}

	void lumberjack_clear() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		const auto tree_id = region::tree_id(idx);
		if (tree_id > 0) {
			designations->chopping.erase((int)tree_id);
		}
	}

	void available_buildings(size_t &size, buildable_building_t *& build_ptr) {
		impl::available_buildings.clear();

		auto available_buildings = inventory::get_available_buildings();
		for (const auto &building : available_buildings) {
			buildable_building_t b;
			strncpy_s(b.tag, building.tag.c_str(), 254);
			strncpy_s(b.displayName, building.name.c_str(), 254);
			impl::available_buildings.emplace_back(b);
		}

		ArrayToUnrealPtr<buildable_building_t>(size, build_ptr, impl::available_buildings);
	}

	void set_selected_building(int list_index) {
		selected_building = list_index;

		auto available_buildings = inventory::get_available_buildings();
		if (list_index < available_buildings.size() && list_index > -1) {
			buildings::has_build_mode_building = true;
			buildings::build_mode_building = available_buildings[list_index];
		}
		else {
			buildings::has_build_mode_building = false;
		}
	}

	void place_selected_building() {
		auto can_build = true;
		const auto tag = buildings::build_mode_building.tag;
		const auto building_def = get_building_def(tag);

		const auto bx = mouse_x + (building_def->width == 3 ? -1 : 0);
		const auto by = mouse_y + (building_def->height == 3 ? -1 : 0);
		const auto bz = mouse_z;

		std::vector<int> target_tiles;
		for (auto y = by; y < by + building_def->height; ++y) {
			for (auto x = bx; x < bx + building_def->width; ++x) {
				const auto idx = mapidx(x, y, bz);
				if (!region::flag(idx, tile_flags::CAN_STAND_HERE))
				{
					// TODO: Check or open space and allow that for some tags.
					if (!(tag == "floor" || tag == "wall"))
					{
						can_build = false;
					}
				}
				if (region::get_building_id(idx) > 0) can_build = false;
				target_tiles.emplace_back(idx);
			}
		}

		if (can_build) {

			// Perform the building
			systems::inventory_system::building_request(bx, by, bz, buildings::build_mode_building);
			buildings::has_build_mode_building = false;
		}
	}
}