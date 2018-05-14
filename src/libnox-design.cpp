#include "libnox.h"
#include "libnox-render.hpp"
#include "global_assets/game_ecs.hpp"
#include "planet/region/region.hpp"
#include "global_assets/game_designations.hpp"
#include "global_assets/game_camera.hpp"
#include "global_assets/building_designations.hpp"
#include "global_assets/game_building.hpp"
#include "global_assets/farming_designations.hpp"
#include "global_assets/game_mining.hpp"
#include "systems/helpers/inventory_assistant.hpp"
#include "systems/ai/mining_system.hpp"
#include "raws/buildings.hpp"
#include "raws/defs/building_def_t.hpp"
#include "raws/plants.hpp"
#include "raws/defs/plant_t.hpp"
#include "nox_impl_helpers.hpp"
#include <utility>
#include <algorithm>

namespace nf {

	namespace impl {
		std::vector<buildable_building_t> available_buildings;
		std::vector<plantable_seed_t> available_seeds;
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

	void plantable_seeds(size_t &size, plantable_seed_t *& seed_ptr) {
		impl::available_seeds.clear();

		std::map<std::string, std::pair<int, std::string>> available_seeds;
		bengine::each_without<claimed_t, item_seed_t, item_t>([&available_seeds](bengine::entity_t &e, item_seed_t &seed, item_t &item) {
			auto plant_finder = get_plant_def(get_plant_idx(seed.grows_into));
			if (plant_finder) {
				const std::string name = plant_finder->name;
				auto finder = available_seeds.find(name);
				if (finder == available_seeds.end()) {
					available_seeds[name] = std::make_pair(1, seed.grows_into);
				}
				else {
					int tmp = available_seeds[name].first;
					available_seeds[name].first = tmp + 1;
				}
			}
		});

		for (const auto &seed : available_seeds) {
			std::string seed_name;
			seed_name = seed.first + std::string(" (") + std::to_string(seed.second.first) + std::string(")");
			plantable_seed_t p;
			strncpy_s(p.name, seed_name.c_str(), 254);
			p.number = seed.second.first;
			strncpy_s(p.grows_into, seed.second.second.c_str(), 254);
			impl::available_seeds.emplace_back(p);
		}

		ArrayToUnrealPtr<plantable_seed_t>(size, seed_ptr, impl::available_seeds);
	}

	void set_selected_seed(int list_index) {
		selected_seed = list_index;
	}

	void plant_set() {
		if (!impl::available_seeds.empty() && selected_seed < impl::available_seeds.size()) {
			const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
			const auto farm = farm_designations->farms.find(idx);
			if (farm == farm_designations->farms.end()) {
				auto found = false;
				std::size_t seed_id = 0;
				bengine::each_without<claimed_t, item_seed_t>([&found, &seed_id](bengine::entity_t &seed_entity, item_seed_t &seed) {
					//std::cout << seed.grows_into << " / " << seed_options[selected_seed] << "\n";
					std::string grows_info(impl::available_seeds[selected_seed].grows_into);
					if (!found && seed.grows_into == grows_info) {
						found = true;
						seed_entity.assign(claimed_t{ 0 });
						seed_id = seed_entity.id;
					}
				});
				if (found) {
					farm_designations->farms[idx] = farm_cycle_t{ 0, std::string(impl::available_seeds[selected_seed].grows_into), seed_id };
				}
			}
		}
	}

	void plant_clear() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		farm_designations->farms.erase(idx);
	}

	static int mine_mode = 0;

	void set_mining_mode(int mode) {
		mine_mode = mode;
	}

	int get_mining_mode() {
		return mine_mode;
	}

	void mine_set() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		mining_designations->mining_targets[idx] = mine_mode;
		systems::mining_system::mining_map_changed();
	}

	void mine_clear() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		mining_designations->mining_targets.erase(idx);
		systems::mining_system::mining_map_changed();
	}

	static int architecture_mode = 0;

	void set_architecture_mode(int mode) {
		architecture_mode = mode;
	}

	int get_architecture_mode() {
		return architecture_mode;
	}

	void architecture_set() {

	}

	void architecture_clear() {

	}
}