#include "run_systems.hpp"
#include "scheduler/tick_system.hpp"
#include "scheduler/calendar_system.hpp"
#include "scheduler/hunger_system.hpp"
#include "../global_assets/game_pause.hpp"
#include "ai/wildlife_population.hpp"
#include "physics/explosive_system.hpp"
#include "physics/door_system.hpp"
#include "physics/gravity_system.hpp"
#include "ai/distance_map_system.hpp"
#include "overworld/world_system.hpp"
#include "scheduler/initiative_system.hpp"
#include "ai/sentient_ai_system.hpp"
#include "scheduler/corpse_system.hpp"
#include "ai/mining_system.hpp"
#include "ai/architecture_system.hpp"
#include "ai/stockpile_system.hpp"
#include "power/power_system.hpp"
#include "ai/workflow_system.hpp"
#include "ai/ai_status_effects.hpp"
#include "ai/ai_stuck.hpp"
#include "ai/settler/ai_visibility_scan.hpp"
#include "ai/settler/ai_new_arrival.hpp"
#include "ai/settler/ai_scheduler.hpp"
#include "ai/settler/ai_leisure_time.hpp"
#include "ai/settler/ai_sleep_time.hpp"
#include "ai/settler/ai_work_time.hpp"
#include "ai/settler/ai_work_lumberjack.hpp"
#include "ai/settler/ai_work_mining.hpp"
#include "ai/settler/ai_work_guard.hpp"
#include "ai/settler/ai_work_harvest.hpp"
#include "ai/settler/ai_work_farm_fertilize.hpp"
#include "ai/settler/ai_work_farm_clear.hpp"
#include "ai/settler/ai_work_farm_fixsoil.hpp"
#include "ai/settler/ai_work_farm_plant.hpp"
#include "ai/settler/ai_work_farm_water.hpp"
#include "ai/settler/ai_work_farm_weed.hpp"
#include "ai/settler/ai_work_building.hpp"
#include "ai/settler/ai_work_order.hpp"
#include "ai/settler/ai_work_architect.hpp"
#include "ai/settler/ai_work_butcher.hpp"
#include "ai/settler/ai_work_hunt.hpp"
#include "ai/settler/ai_work_stockpile.hpp"
#include "ai/settler/ai_deconstruct.hpp"
#include "ai/settler/ai_leisure_eat.hpp"
#include "ai/settler/ai_leisure_drink.hpp"
#include "ai/settler/ai_idle_time.hpp"
#include "physics/movement_system.hpp"
#include "physics/trigger_system.hpp"
#include "damage/settler_ranged_attack_system.hpp"
#include "damage/settler_melee_attacks_system.hpp"
#include "damage/sentient_attacks_system.hpp"
#include "damage/creature_attacks_system.hpp"
#include "damage/turret_ranged_attack_system.hpp"
#include "damage/damage_system.hpp"
#include "damage/kill_system.hpp"
#include "damage/healing_system.hpp"
#include "physics/topology_system.hpp"
#include "physics/vegetation_system.hpp"
#include "physics/visibility_system.hpp"
#include "physics/item_wear_system.hpp"
#include "ai/inventory_system.hpp"
#include "overworld/settler_spawner_system.hpp"

namespace systems {
	void run_systems(const double ms) {
		tick::run(ms);
		if (major_tick) {
			// Age log
			calendarsys::run(ms);
			hunger_system::run(ms);
			if (hour_elapsed) settler_spawner::run(ms);
			wildlife_population::run(ms);
			// fluids
			explosives::run(ms);
			doors::run(ms);
			gravity::run(ms);
			distance_map::run(ms);
			world::run(ms);
			initiative::run(ms);
			if (day_elapsed) sentient_ai_system::run(ms);
			corpse_system::run(ms);
			mining_system::run(ms);
			architecture_system::run(ms);
			stockpile_system::run(ms);
			power::run(ms);
			workflow_system::run(ms);
			ai_status_effects::run(ms);
			ai_stuck::run(ms);
			ai_visibility_scan::run(ms);
			ai_new_arrival::run(ms);
			ai_scheduler::run(ms);
			ai_leisure_time::run(ms);
			ai_sleep_time::run(ms);
			ai_work_time::run(ms);
			ai_work_lumberjack::run(ms);
			ai_mining::run(ms);
			ai_guard::run(ms);
			ai_harvest::run(ms);
			ai_farm_plant::run(ms);
			ai_farm_fertilize::run(ms);
			ai_farm_clear::run(ms);
			ai_farm_fixsoil::run(ms);
			ai_farm_water::run(ms);
			ai_farm_weed::run(ms);
			ai_building::run(ms);
			ai_workorder::run(ms);
			ai_architect::run(ms);
			ai_hunt::run(ms);
			ai_butcher::run(ms);
			ai_work_stockpiles::run(ms);
			ai_deconstruction::run(ms);
			ai_leisure_eat::run(ms);
			ai_leisure_drink::run(ms);
			ai_idle_time::run(ms);
			movement::run(ms);
			triggers::run(ms);
			settler_ranged_attack::run(ms);
			settler_melee_attack::run(ms);
			sentient_attacks::run(ms);
			creature_attacks::run(ms);
			turret_attacks::run(ms);
			damage_system::run(ms);
			kill_system::run(ms);
			if (hour_elapsed) healing_system::run(ms);
			topology::run(ms);
			visibility::run(ms);
			vegetation::run(ms);
			item_wear::run(ms);
			inventory_system::run(ms);
		}
	}
}