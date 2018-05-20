#include "libnox.h"
#include "planet/planet.hpp"
#include "global_assets/game_planet.hpp"
#include "raws/raws.hpp"
#include "bengine/filesystem.hpp"
#include "global_assets/game_planet.hpp"
#include "raws/buildings.hpp"
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
#include "raws/materials.hpp"
#include <string>


namespace nf {

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

	bool is_world_loadable() {
		return exists(save_filename());
	}

	namespace impl {
		std::vector<material_map_t> matmap;
	}

	void get_materials_map(size_t &size, material_map_t *& mat_ptr) {
		impl::matmap.clear();
		impl::matmap.resize(texture_atlas.size() + 1);
		for (const auto &t : texture_atlas) {
			material_map_t mm;
			strncpy_s(mm.UnrealPath, t.second.c_str(), 254);
			impl::matmap[t.first] = mm;
		}

		size = impl::matmap.size();
		mat_ptr = &impl::matmap[0];
	}
}