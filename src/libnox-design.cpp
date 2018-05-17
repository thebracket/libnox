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
#include "systems/ai/architecture_system.hpp"
#include "global_assets/architecture_designations.hpp"
#include "raws/buildings.hpp"
#include "raws/defs/building_def_t.hpp"
#include "raws/plants.hpp"
#include "raws/defs/plant_t.hpp"
#include "raws/reactions.hpp"
#include "raws/defs/reaction_t.hpp"
#include "raws/items.hpp"
#include "raws/materials.hpp"
#include "raws/defs/item_def_t.hpp"
#include "raws/defs/material_def_t.hpp"
#include "nox_impl_helpers.hpp"
#include <utility>
#include <algorithm>

namespace nf {

	namespace impl {
		std::vector<buildable_building_t> available_buildings;
		std::vector<plantable_seed_t> available_seeds;
		std::vector<queued_work_t> workflow_queue;
		std::vector<available_work_t> workflow_available;
		std::vector<active_standing_order_t> worfklow_standing_orders;
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
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		architecture_designations->architecture[idx] = architecture_mode;
		systems::architecture_system::architecture_map_changed();
	}

	void architecture_clear() {
		const auto idx = mapidx(mouse_x, mouse_y, mouse_z);
		architecture_designations->architecture.erase(idx);
		systems::architecture_system::architecture_map_changed();
	}

	int get_available_block_count() {
		return 0;
	}

	int get_required_block_count() {
		return 0;
	}

	void workflow_menu(size_t &queue_size, queued_work_t *& queued_work_ptr, size_t &available_size, available_work_t *& available_work_ptr, size_t &standing_order_size, active_standing_order_t *& standing_order_ptr) {		
		// Clear everything
		impl::workflow_queue.clear();
		impl::workflow_available.clear();
		impl::worfklow_standing_orders.clear();

		// Build the lists

		// Queue
		for (const auto &wo : building_designations->build_orders) {
			queued_work_t w;
			w.qty = wo.first;
			strncpy_s(w.reaction_def, wo.second.c_str(), 254);
			const auto finder = get_reaction_def(wo.second);
			strncpy_s(w.reaction_name, finder->name.c_str(), 254);
			impl::workflow_queue.emplace_back(w);
		}

		// Available
		for (const auto &av : inventory::get_available_reactions()) {
			available_work_t w;
			strncpy_s(w.reaction_def, av.first.c_str(), 254);
			strncpy_s(w.reaction_name, av.second.c_str(), 254);
			const auto rf = get_reaction_def(av.first);
			const auto bf = get_building_def(rf->workshop);
			strncpy_s(w.building_name, bf->name.c_str(), 254);
			std::string inputs;
			std::string outputs;

			// Calculate the inputs
			for (const auto &i : rf->inputs)
			{
				const auto ifinder = get_item_def(i.tag);
				if (i.required_material > 0)
				{
					const auto mf = get_material(i.required_material);
					inputs += std::string("{") + mf->name + std::string("}");
				}
				if (i.required_material_type > 0)
				{
					switch (i.required_material_type)
					{
					case CLUSTER_ROCK: inputs += std::string("[rock] "); break;
					case ROCK: inputs += std::string("[rock] "); break;
					case SOIL: inputs += std::string("[soil] "); break;
					case SAND: inputs += std::string("[sand] "); break;
					case METAL: inputs += std::string("[metal] "); break;
					case SYNTHETIC: inputs += std::string("[synthetic] "); break;
					case ORGANIC: inputs += std::string("[organic] "); break;
					case LEATHER: inputs += std::string("[leather] "); break;
					case FOOD: inputs += std::string("[food] "); break;
					case SPICE: inputs += std::string("[spice] "); break;
					case BLIGHT: inputs += std::string("[blight] "); break;
					default: inputs += std::string("[?]");
					}
				}
				const auto name = ifinder ? ifinder->name : std::string("");
				inputs += name + std::string(" (x") + std::to_string(i.quantity) + ") ";
			}
			if (rf->power_drain > 0) inputs += std::string("Drains ") + std::to_string(rf->power_drain) + std::string(" power. ");

			// Calculate the outputs
			for (const auto &i : rf->outputs)
			{
				const auto ifinder = get_item_def(i.first);
				outputs += ifinder->name + std::string(" (x") + std::to_string(i.second) + ") ";
			}

			// Copy
			strncpy_s(w.inputs, inputs.c_str(), 1024);
			strncpy_s(w.outputs, outputs.c_str(), 1024);

			impl::workflow_available.emplace_back(w);
		}

		// Standing Orders
		for (const auto &so : building_designations->standing_build_orders) {
			active_standing_order_t s;

			s.min_qty = so.second.first;
			strncpy_s(s.reaction_name, so.first.c_str(), 254);
			const auto item_finder = get_item_def(so.first);
			strncpy_s(s.item_name, item_finder->name.c_str(), 254);
			strncpy_s(s.item_tag, item_finder->tag.c_str(), 254);

			impl::worfklow_standing_orders.emplace_back(s);
		}

		// Return them
		ArrayToUnrealPtr<queued_work_t>(queue_size, queued_work_ptr, impl::workflow_queue);
		ArrayToUnrealPtr<available_work_t>(available_size, available_work_ptr, impl::workflow_available);
		ArrayToUnrealPtr<active_standing_order_t>(standing_order_size, standing_order_ptr, impl::worfklow_standing_orders);
	}

	void workflow_remove_from_queue(int index) {
		if (impl::workflow_queue.size() - 1 < index) return;

		std::vector<std::string> to_remove;

		std::string reaction_def = std::string(impl::workflow_queue[index].reaction_def);
		for (auto &bo : building_designations->build_orders) {
			if (bo.second == reaction_def) {
				if (bo.first > 1) {
					--bo.first;
				}
				else {
					to_remove.emplace_back(bo.second);
				}
			}
		}

		for (auto &j : to_remove)
		{
			building_designations->build_orders.erase(
				std::remove_if(
					building_designations->build_orders.begin(),
					building_designations->build_orders.end(),
					[&j](const auto &b) { return b.second == j; }
				),
				building_designations->build_orders.end()
			);
		}
	}

	void workflow_enqueue(int index) {
		if (impl::workflow_available.size() - 1 < index) return;
		std::string reaction_tag = impl::workflow_available[index].reaction_def;
		auto found = false;
		for (auto &job : building_designations->build_orders) {
			if (job.second == reaction_tag) {
				found = true;
				++job.first;
			}
		}

		if (!found) {
			building_designations->build_orders.emplace_back(std::make_pair(1, reaction_tag));
		}
	}

	void workflow_add_so(int index) {
		if (impl::workflow_available.size() - 1 < index) return;

		auto wf = impl::workflow_available[index];

		//building_designations->standing_build_orders.insert(std::make_pair( std::string(wf.reaction_def), std::make_pair(1, std::string(wf.reaction_name) )));
	}

	void workflow_remove_so(int index) {
		if (impl::worfklow_standing_orders.size() - 1 < index) return;

		auto wf = impl::worfklow_standing_orders[index];
		building_designations->standing_build_orders.erase(std::string(wf.item_tag));
	}

}