#include "libnox.h"
#include "planet/planet.hpp"
#include "global_assets/game_planet.hpp"
#include "raws/raws.hpp"
#include "bengine/filesystem.hpp"
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
#include "libnox-render.hpp"
#include <array>

namespace nf {			

	bool water_dirty = true;

	void on_tick(const double duration_ms) {
		systems::run_systems(duration_ms * 1000.0);
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
}