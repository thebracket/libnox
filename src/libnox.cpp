#include "libnox.h"
#include "planet/planet.hpp"
#include "global_assets/game_planet.hpp"
#include "raws/raws.hpp"
#include "bengine/filesystem.hpp"
#include "components/all_components.hpp"
#include "global_assets/game_ecs.hpp"
#include "global_assets/architecture_designations.hpp"
#include "global_assets/building_designations.hpp"
#include "global_assets/farming_designations.hpp"
#include "global_assets/game_building.hpp"
#include "global_assets/game_calendar.hpp"
#include "global_assets/game_camera.hpp"
#include "global_assets/game_config.hpp"
#include "global_assets/game_mode.hpp"
#include "global_assets/game_designations.hpp"
#include "global_assets/game_mining.hpp"
#include "global_assets/game_pause.hpp"
#include "planet/region/region.hpp"
#include "planet/region/region_chunking.hpp"
#include "planet/region/renderables.hpp"
#include "planet/region/lighting.hpp"
#include "systems/run_systems.hpp"
#include "raws/string_table.hpp"
#include "raws/materials.hpp"
#include "raws/defs/material_def_t.hpp"
#include "raws/plants.hpp"
#include "raws/defs/plant_t.hpp"
#include "raws/items.hpp"
#include "raws/defs/item_def_t.hpp"
#include "raws/buildings.hpp"
#include "raws/defs/building_def_t.hpp"
#include "systems/helpers/inventory_assistant.hpp"
#include <array>

namespace nf {

	static int mouse_x = 0;
	static int mouse_y = 0;
	static int mouse_z = 0;
	int selected_building = 0;

	void set_game_def_path(const char * base_path) {
		const std::string path(base_path);
		const std::string raws_path = path + "world_defs/";
		const std::string buildings_path = path + "rex/";
		set_raws_path(raws_path.c_str());
		set_building_path(buildings_path.c_str());
	}

	const char * get_version() {
		return "Nox Futura 0.5.0U";
	}

	void setup_planet() {
		planet = load_planet();
	}

	void serialize_planet() {
		save_planet(planet);
	}

	static bool raws_loaded = false;

	void setup_raws() {
		if (!raws_loaded) load_raws();
		raws_loaded = true;
	}

	void load_game() {
		using namespace bengine;

		// Planet
		load_planet();

		// ECS state
		{
			std::unique_ptr<std::ifstream> lbfile = std::make_unique<std::ifstream>(save_filename(), std::ios::in | std::ios::binary);
			ecs_load(lbfile);
		}

		// Pointers to entities
		int region_x, region_y;
		each<world_position_t, calendar_t, designations_t, logger_t, camera_options_t, mining_designations_t, farming_designations_t, building_designations_t, architecture_designations_t>(
			[&region_x, &region_y](entity_t &entity, world_position_t &pos, calendar_t &cal, designations_t &design,
				logger_t &log, camera_options_t &camera_prefs, mining_designations_t &mining,
				farming_designations_t &farming, building_designations_t &building,
				architecture_designations_t &arch)
		{
			camera_entity = entity.id;
			region_x = pos.world_x;
			region_y = pos.world_y;
			camera_position = &pos;
			calendar = &cal;
			designations = &design;
			//logger = &log;
			camera = &camera_prefs;
			mining_designations = &mining;
			farm_designations = &farming;
			building_designations = &building;
			architecture_designations = &arch;
		});

		// Region
		region::load_current_region(region_x, region_y);
		region::tile_recalc_all();
		region::update_outdoor_calculation();

	}

	void chunks_init() {
		region::initialize_chunks();
	}

	void chunks_update() {
		region::update_chunks();
	}

	std::vector<int> impl_dirty_list;

	void chunks_update_list_dirty(size_t &size, int *& dirty_ptr) {
		impl_dirty_list.clear();
		region::update_chunks_listing_changes(impl_dirty_list);
		size = impl_dirty_list.size();
		dirty_ptr = size > 0 ? &impl_dirty_list[0] : nullptr;
	}

	void chunk_floors(const int &chunk_idx, const int &chunk_z, size_t &size, floor_t *& floor_ptr) {
		region::get_chunk_floors(chunk_idx, chunk_z, size, floor_ptr);
	}

	void chunk_cubes(const int &chunk_idx, const int &chunk_z, size_t &size, cube_t *& cube_ptr) {
		region::get_chunk_cubes(chunk_idx, chunk_z, size, cube_ptr);
	}

	std::vector<static_model_t> impl_model_list;

	void chunk_models(const int &chunk_idx, size_t &size, static_model_t *& model_ptr) {
		impl_model_list.clear();
		region::get_chunk_models(chunk_idx, impl_model_list);
		size = impl_model_list.size();
		model_ptr = size > 0 ? &impl_model_list[0] : nullptr;
	}

	void chunk_world_coordinates(const int &idx, int &x, int &y, int &z) {
		region::get_chunk_coordinates(idx, x, y, z);
	}

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

	static std::vector<dynamic_model_t> impl_dyn_models;

	void voxel_render_list(size_t &size, dynamic_model_t *& model_ptr) {
		impl_dyn_models.clear();
		render::build_voxel_list(selected_building, mouse_x, mouse_y, mouse_z);
		render::get_model_list(impl_dyn_models);
		size = impl_dyn_models.size();
		model_ptr = size > 0 ? &impl_dyn_models[0] : nullptr;
	}

	static std::vector<dynamic_lightsource_t> impl_dyn_lights;

	void lightsource_list(size_t &size, dynamic_lightsource_t *& light_ptr) {
		impl_dyn_lights.clear();
		render::get_light_list(impl_dyn_lights);
		size = impl_dyn_lights.size();
		light_ptr = size > 0 ? &impl_dyn_lights[0] : nullptr;
	}

	hud_info_t get_hud_info() {
		hud_info_t result;

		result.year = calendar->year;
		result.month = calendar->month;
		result.day = calendar->day;
		result.hour = calendar->hour;
		result.minute = calendar->minute;
		result.second = calendar->second;
		result.max_power = designations->total_capacity;
		result.current_power = designations->current_power;
		result.cash = designations->current_cash;

		return result;
	}

	static std::vector<water_t> impl_water;

	void water_cubes(size_t &size, water_t *& water_ptr) {
		impl_water.clear();
		std::vector<uint32_t> * w = region::get_water_level();
		for (int z = 0; z < REGION_DEPTH; ++z) {
			for (int y = 0; y < REGION_HEIGHT; ++y) {
				for (int x = 0; x < REGION_WIDTH; ++x) {
					const int idx = mapidx(x, y, z);
					const uint32_t wl = w->operator[](idx);
					if (wl > 0) {
						impl_water.emplace_back(water_t{ (float)x, (float)y, (float)z, ((float)wl / 10.0f) });
					}
				}
			}
		}
		size = impl_water.size();
		water_ptr = size > 0 ? &impl_water[0] : nullptr;
	}

	bool water_dirty = true;

	int get_pause_mode() {
		return pause_mode;
	}

	void set_pause_mode(int mode) {
		switch (mode) {
		case 0: pause_mode = RUNNING; break;
		case 1: pause_mode = PAUSED; break;
		case 2: pause_mode = ONE_STEP; break;
		default: pause_mode = RUNNING;
		}
	}

	void on_tick(const double duration_ms) {
		systems::run_systems(duration_ms / 1000.0);
	}

	bool is_world_loadable() {
		return exists(save_filename());
	}

	static std::string subtitle;

	static std::string get_descriptive_noun() {
		using namespace string_tables;

		const bengine::random_number_generator rng;
		return string_table(MENU_SUBTITLES)->random_entry(rng);
	}

	const char * get_game_subtitle() {
		bengine::random_number_generator rng;
		switch (rng.roll_dice(1, 8)) {
		case 1: subtitle = "Histories "; break;
		case 2: subtitle = "Chronicles "; break;
		case 3: subtitle = "Sagas "; break;
		case 4: subtitle = "Annals "; break;
		case 5: subtitle = "Narratives "; break;
		case 6: subtitle = "Recitals "; break;
		case 7: subtitle = "Tales "; break;
		case 8: subtitle = "Stories "; break;
		}

		const auto first_noun = get_descriptive_noun();
		auto second_noun = first_noun;
		while (second_noun == first_noun) {
			second_noun = get_descriptive_noun();
		}

		subtitle += "of " + first_noun + " and " + second_noun;

		return subtitle.c_str();
	}

	std::vector<unit_list_settler_t> impl_unit_list_settlers;
	std::vector<unit_list_settler_t> impl_unit_list_native;
	std::vector<unit_list_settler_t> impl_unit_list_wildlife;
	const char * gender_male = "Male";
	const char * gender_female = "Female";

	void get_unit_list_settlers(size_t &size, unit_list_settler_t *& settler_ptr) {
		impl_unit_list_settlers.clear();
		bengine::each<settler_ai_t, name_t, game_stats_t, species_t, health_t>([](bengine::entity_t &e, settler_ai_t &s, name_t &n, game_stats_t &stats, species_t &species, health_t &health) {
			std::string name = n.first_name + std::string(" ") + n.last_name;
			unit_list_settler_t newsettler;

			strncpy_s(newsettler.name, name.c_str(), 254);
			strncpy_s(newsettler.gender, species.gender == MALE ? gender_male : gender_female, 8);
			strncpy_s(newsettler.profession, stats.profession_tag.c_str(), 254);
			strncpy_s(newsettler.task, s.job_status.c_str(), 254);
			std::string hp = std::string("HP:") + std::to_string(health.current_hitpoints) + std::string("/") + std::to_string(health.max_hitpoints);
			if (health.blind) hp += std::string(" Blind.");
			if (health.no_grasp) hp += std::string(" Cannot grasp.");
			if (health.slow) hp += std::string(" Slow.");
			if (health.unconscious) hp += std::string(" Unsconscious.");
			strncpy_s(newsettler.hp, hp.c_str(), 254);
			newsettler.health_percent = (float)health.current_hitpoints / (float)health.max_hitpoints;
			newsettler.id = e.id;

			impl_unit_list_settlers.push_back(newsettler);
		});
		size = impl_unit_list_settlers.size();
		settler_ptr = size > 0 ? &impl_unit_list_settlers[0] : nullptr;
	}

	void get_unit_list_natives(size_t &size, unit_list_settler_t *& settler_ptr) {
		impl_unit_list_native.clear();
		bengine::each<sentient_ai, name_t, game_stats_t, species_t, health_t>([](bengine::entity_t &e, sentient_ai &s, name_t &n, game_stats_t &stats, species_t &species, health_t &health) {
			std::string name = n.first_name + std::string(" ") + n.last_name;
			unit_list_settler_t newsettler;

			strncpy_s(newsettler.name, name.c_str(), 254);
			strncpy_s(newsettler.gender, species.gender == MALE ? gender_male : gender_female, 8);
			strncpy_s(newsettler.profession, stats.profession_tag.c_str(), 254);
			std::string hp = std::string("HP:") + std::to_string(health.current_hitpoints) + std::string("/") + std::to_string(health.max_hitpoints);
			if (health.blind) hp += std::string(" Blind.");
			if (health.no_grasp) hp += std::string(" Cannot grasp.");
			if (health.slow) hp += std::string(" Slow.");
			if (health.unconscious) hp += std::string(" Unsconscious.");
			strncpy_s(newsettler.hp, hp.c_str(), 254);
			newsettler.health_percent = (float)health.current_hitpoints / (float)health.max_hitpoints;
			newsettler.id = e.id;

			impl_unit_list_native.push_back(newsettler);
		});
		size = impl_unit_list_native.size();
		settler_ptr = size > 0 ? &impl_unit_list_native[0] : nullptr;
	}

	void get_unit_list_wildlife(size_t &size, unit_list_settler_t *& settler_ptr) {
		impl_unit_list_wildlife.clear();
		bengine::each<grazer_ai, name_t, game_stats_t, species_t, health_t>([](bengine::entity_t &e, grazer_ai &s, name_t &n, game_stats_t &stats, species_t &species, health_t &health) {
			std::string name = n.first_name + std::string(" ") + n.last_name;
			unit_list_settler_t newsettler;

			strncpy_s(newsettler.name, name.c_str(), 254);
			strncpy_s(newsettler.gender, species.gender == MALE ? gender_male : gender_female, 8);
			strncpy_s(newsettler.profession, stats.profession_tag.c_str(), 254);
			std::string hp = std::string("HP:") + std::to_string(health.current_hitpoints) + std::string("/") + std::to_string(health.max_hitpoints);
			if (health.blind) hp += std::string(" Blind.");
			if (health.no_grasp) hp += std::string(" Cannot grasp.");
			if (health.slow) hp += std::string(" Slow.");
			if (health.unconscious) hp += std::string(" Unsconscious.");
			strncpy_s(newsettler.hp, hp.c_str(), 254);
			newsettler.health_percent = (float)health.current_hitpoints / (float)health.max_hitpoints;
			newsettler.id = e.id;

			impl_unit_list_wildlife.push_back(newsettler);
		});
		size = impl_unit_list_wildlife.size();
		settler_ptr = size > 0 ? &impl_unit_list_wildlife[0] : nullptr;
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

	void set_world_pos_from_mouse(int x, int y, int z) {
		mouse_x = x;
		mouse_y = y;
		mouse_z = z;

		if (mouse_x < 1) mouse_x = 1;
		if (mouse_x > nf::REGION_WIDTH - 1) mouse_x = nf::REGION_WIDTH - 1;
		if (mouse_y < 1) mouse_y = 1;
		if (mouse_y > nf::REGION_HEIGHT - 1) mouse_y = nf::REGION_HEIGHT - 1;
		if (mouse_z < 1) mouse_z = 1;
		if (mouse_z > nf::REGION_DEPTH - 1) mouse_z = nf::REGION_DEPTH - 1;
	}

	tooltip_info_t get_tooltip_info() {
		tooltip_info_t info;

		std::array<std::string, 5> lines;

		const int tile_idx = mapidx(mouse_x, mouse_y, mouse_z);

		// Basic info
		{
			std::stringstream ss;
			switch (region::tile_type(tile_idx)) {

			case tile_type::SEMI_MOLTEN_ROCK: ss << "Magma"; break;
			case tile_type::SOLID: ss << "Solid Rock (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::OPEN_SPACE: ss << "Open Space"; break;
			case tile_type::WALL: ss << "Wall (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::RAMP: ss << "Ramp (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::STAIRS_UP: ss << "Up Stairs (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::STAIRS_DOWN: ss << "Down Stairs (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::STAIRS_UPDOWN: ss << "Spiral Stairs (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::FLOOR: ss << "Floor (" << material_name(region::material(tile_idx)) << ")"; break;
			case tile_type::TREE_TRUNK: ss << "Tree Trunk"; break;
			case tile_type::TREE_LEAF: ss << "Tree Foliage"; break;
			case tile_type::WINDOW: ss << "Window"; break;
			case tile_type::CLOSED_DOOR: ss << "Closed Door (" << material_name(region::material(tile_idx)) << ")"; break;
			}
			if (region::tile_type(tile_idx) != tile_type::OPEN_SPACE) {
				ss << " (" << region::tile_hit_points(tile_idx) << ")";
			}
			lines[0] = ss.str();
		}
		int line_idx = 1;

		{
			// Farming
			const auto farm_finder = farm_designations->farms.find(tile_idx);
			if (farm_finder != farm_designations->farms.end()) {
				std::stringstream ss;
				ss << " Farm: " << farm_finder->second.seed_type << " - ";
				switch (farm_finder->second.state) {
				case farm_steps::CLEAR: ss << "Plough"; break;
				case farm_steps::FIX_SOIL: ss << "Fix Soil"; break;
				case farm_steps::PLANT_SEEDS: ss << "Plant Seeds"; break;
				case farm_steps::GROWING: ss << "Growing"; break;
				}
				ss << ". Weeded/Watered " << farm_finder->second.days_since_weeded << "/" << farm_finder->second.days_since_watered << " days ago.";
				lines[line_idx] = ss.str();
				++line_idx;
			}
		}

		// Plants
		if (region::tile_type(tile_idx) == tile_type::FLOOR && !region::flag(tile_idx, tile_flags::CONSTRUCTION)) {
			if (region::veg_type(tile_idx) > 0) {
				std::stringstream ss;
				auto plant = get_plant_def(region::veg_type(tile_idx));
				if (plant != nullptr) {
					ss << plant->name << " (";
					switch (region::veg_lifecycle(tile_idx)) {
					case 0: ss << "Germinating"; break;
					case 1: ss << "Sprouting"; break;
					case 2: ss << "Growing"; break;
					case 3: ss << "Flowering"; break;
					default: ss << "Unknown - error!";
					}
					if (!plant->provides.empty()) {
						const std::string harvest_to = plant->provides[region::veg_lifecycle(tile_idx)];
						if (harvest_to != "none") {
							const auto harvest_result = get_item_def(harvest_to);
							if (harvest_result) {
								ss << " - provides: " << harvest_result->name;
							}
							else {
								ss << " - provides: " << harvest_to << "; WARNING - RAW TAG";
							}
						}
					}
					ss << ")";
					lines[line_idx] = ss.str();
					++line_idx;
				}
			}
		}

		// Named entities
		{
			std::stringstream ss;
			bool added = false;
			bengine::each<name_t, position_t>([&ss, &added](bengine::entity_t &entity, name_t &name, position_t &pos) {
				auto is_building = entity.component<building_t>();
				if (!is_building && pos.x == mouse_x && pos.y == mouse_y && pos.z == mouse_z) {
					ss << name.first_name << " " << name.last_name << "  ";
					added = true;
				}
			});
			if (added && line_idx < 5) lines[line_idx] = ss.str();
			++line_idx;
		}

		// Items
		{
			std::stringstream ss;
			bool added = false;
			std::map<std::string, int> items;
			bengine::each<item_t, position_t>([&items](bengine::entity_t &entity, item_t &item, position_t &pos) {
				if (pos.x == mouse_x && pos.y == mouse_y && pos.z == mouse_z) {
					std::string claimed;
					if (entity.component<claimed_t>() != nullptr) claimed = " (c)";
					std::string quality;
					std::string wear;

					auto qual = entity.component<item_quality_t>();
					auto wr = entity.component<item_wear_t>();
					if (qual) quality = std::string(" (") + qual->get_quality_text() + std::string(" quality)");
					if (wr) wear = std::string(" (") + wr->get_wear_text() + std::string(")");
					const auto name = item.item_name + claimed + quality + wear;

					auto finder = items.find(name);
					if (finder == items.end()) {

						items[name] = 1;
					}
					else {
						++finder->second;
					}
				}
			});
			for (const auto &it : items) {
				const auto n = std::to_string(it.second);
				ss << n << "x " << it.first << "  ";
				added = true;
			}
			if (added && line_idx < 5) lines[line_idx] = ss.str();
			++line_idx;
		}

		// Buildings
		// Stockpiles

		strncpy_s(info.line1, lines[0].c_str(), 254);
		strncpy_s(info.line2, lines[1].c_str(), 254);
		strncpy_s(info.line3, lines[2].c_str(), 254);
		strncpy_s(info.line4, lines[3].c_str(), 254);
		strncpy_s(info.line5, lines[4].c_str(), 254);

		return info;
	}

	void get_game_mode(int &major, int &minor) {
		major = game_master_mode;
		minor = game_design_mode;
	}

	void set_game_mode(int major, int minor) {
		game_master_mode = (game_master_mode_t)major;
		game_design_mode = (game_design_mode_t)minor;
	}

	std::vector<settler_job_t> impl_jobs_list;

	void get_settler_job_list(size_t &size, settler_job_t *& job_ptr) {
		impl_jobs_list.clear();

		bengine::each<settler_ai_t, name_t, game_stats_t, species_t, health_t>([](bengine::entity_t &e, settler_ai_t &s, name_t &n, game_stats_t &stats, species_t &species, health_t &health) {
			std::string name = n.first_name + std::string(" ") + n.last_name;
			settler_job_t newsettler;

			strncpy_s(newsettler.name, name.c_str(), 254);
			strncpy_s(newsettler.profession, stats.profession_tag.c_str(), 254);
			newsettler.is_farmer = e.component<designated_farmer_t>() != nullptr;
			newsettler.is_lumberjack = e.component<designated_lumberjack_t>() != nullptr;
			newsettler.is_miner = e.component<designated_miner_t>() != nullptr;
			newsettler.id = e.id;

			impl_jobs_list.emplace_back(newsettler);
		});

		size = impl_jobs_list.size();
		job_ptr = size > 0 ? &impl_jobs_list[0] : nullptr;
	}

	void make_miner(int id) {
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			settler->assign(designated_miner_t{});
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->original_profession = stats->profession_tag;
				stats->profession_tag = "Miner";
			}
		}
	}

	void make_farmer(int id) {
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			settler->assign(designated_farmer_t{});
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->original_profession = stats->profession_tag;
				stats->profession_tag = "Farmer";
			}
		}
	}

	void make_lumberjack(int id) {
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			settler->assign(designated_lumberjack_t{});
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->original_profession = stats->profession_tag;
				stats->profession_tag = "Lumberjack";
			}
		}
	}

	void fire_miner(int id) {
		bengine::delete_component<designated_miner_t>(id);
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->profession_tag = stats->original_profession;
			}
		}
	}

	void fire_lumberjack(int id) {
		bengine::delete_component<designated_lumberjack_t>(id);
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->profession_tag = stats->original_profession;
			}
		}
	}

	void fire_farmer(int id) {
		bengine::delete_component<designated_farmer_t>(id);
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->profession_tag = stats->original_profession;
			}
		}
	}

	static std::vector<cube_t> impl_cursors;

	void cursor_list(size_t &size, cube_t *& cube_ptr) {
		impl_cursors.clear();

		// Normal cursor
		impl_cursors.emplace_back(cube_t{ mouse_x, mouse_y, mouse_z, 1, 1, 1, 1 });

		if (game_master_mode == DESIGN) {
			if (game_design_mode == GUARDPOINTS) {
				for (const auto &gp : designations->guard_points) {
					impl_cursors.emplace_back(cube_t{ gp.second.x, gp.second.y, gp.second.z, 1, 1, 1, 3 });
				}
			}
			else if (game_design_mode == CHOPPING) {
				for (size_t i = 0; i < REGION_TILES_COUNT; ++i) {
					auto tree_id = region::tree_id(i);
					if (tree_id > 0) {
						auto[x, y, z] = idxmap(i);
						if (designations->chopping.find(tree_id) != designations->chopping.end()) {
							impl_cursors.emplace_back(cube_t{ x, y, z, 1, 1, 1, 2 });
							//std::cout << "Highlighting tree at " << x << ", " << y << ", " << z << "\n";
						}
						//else {
						//	add_cube_geometry(data, n_elements_cursor_elements, x, y, z, 1, 1, 2);
						//}
					}
				}
			}
		}

		size = impl_cursors.size();
		cube_ptr = size > 0 ? &impl_cursors[0] : nullptr;
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

	std::vector<buildable_building_t> impl_available_buildings;

	void available_buildings(size_t &size, buildable_building_t *& build_ptr) {
		impl_available_buildings.clear();

		auto available_buildings = inventory::get_available_buildings();
		for (const auto &building : available_buildings) {
			buildable_building_t b;
			strncpy_s(b.tag, building.tag.c_str(), 254);
			strncpy_s(b.displayName, building.name.c_str(), 254);
			impl_available_buildings.emplace_back(b);
		}

		size = impl_available_buildings.size();
		build_ptr = size > 0 ? &impl_available_buildings[0] : nullptr;
	}

	void set_selected_building(int list_index) {
		selected_building = list_index;

		auto available_buildings = inventory::get_available_buildings();
		buildings::has_build_mode_building = true;
		buildings::build_mode_building = available_buildings[list_index];
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