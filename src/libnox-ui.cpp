#include "libnox.h"
#include "global_assets/game_ecs.hpp"
#include "planet/region/region.hpp"
#include "libnox-render.hpp"
#include "raws/materials.hpp"
#include "raws/defs/material_def_t.hpp"
#include "raws/plants.hpp"
#include "raws/defs/plant_t.hpp"
#include "raws/items.hpp"
#include "raws/defs/item_def_t.hpp"
#include "global_assets/farming_designations.hpp"
#include "global_assets/game_designations.hpp"
#include "global_assets/game_calendar.hpp"
#include "nox_impl_helpers.hpp"
#include <vector>
#include <string>

namespace nf {
	namespace impl {
		std::vector<unit_list_settler_t> unit_list_settlers;
		std::vector<unit_list_settler_t> unit_list_native;
		std::vector<unit_list_settler_t> unit_list_wildlife;
		std::vector<settler_job_t> jobs_list;
	}

	const char * gender_male = "Male";
	const char * gender_female = "Female";

	void get_unit_list_settlers(size_t &size, unit_list_settler_t *& settler_ptr) {
		impl::unit_list_settlers.clear();
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

			impl::unit_list_settlers.push_back(newsettler);
		});
		ArrayToUnrealPtr<unit_list_settler_t>(size, settler_ptr, impl::unit_list_settlers);
	}

	void get_unit_list_natives(size_t &size, unit_list_settler_t *& settler_ptr) {
		impl::unit_list_native.clear();
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

			impl::unit_list_native.push_back(newsettler);
		});
		ArrayToUnrealPtr<unit_list_settler_t>(size, settler_ptr, impl::unit_list_native);
	}

	void get_unit_list_wildlife(size_t &size, unit_list_settler_t *& settler_ptr) {
		impl::unit_list_wildlife.clear();
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

			impl::unit_list_wildlife.push_back(newsettler);
		});
		ArrayToUnrealPtr<unit_list_settler_t>(size, settler_ptr, impl::unit_list_wildlife);
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

	void get_settler_job_list(size_t &size, settler_job_t *& job_ptr) {
		impl::jobs_list.clear();

		bengine::each<settler_ai_t, name_t, game_stats_t, species_t, health_t>([](bengine::entity_t &e, settler_ai_t &s, name_t &n, game_stats_t &stats, species_t &species, health_t &health) {
			std::string name = n.first_name + std::string(" ") + n.last_name;
			settler_job_t newsettler;

			strncpy_s(newsettler.name, name.c_str(), 254);
			strncpy_s(newsettler.profession, stats.profession_tag.c_str(), 254);
			newsettler.is_farmer = e.component<designated_farmer_t>() != nullptr;
			newsettler.is_lumberjack = e.component<designated_lumberjack_t>() != nullptr;
			newsettler.is_miner = e.component<designated_miner_t>() != nullptr;
			newsettler.is_hunter = e.component<designated_hunter_t>() != nullptr;
			newsettler.id = e.id;

			impl::jobs_list.emplace_back(newsettler);
		});

		ArrayToUnrealPtr<settler_job_t>(size, job_ptr, impl::jobs_list);
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

	void make_hunter(int id) {
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			settler->assign(designated_hunter_t{});
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->original_profession = stats->profession_tag;
				stats->profession_tag = "Hunter";
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

	void fire_hunter(int id) {
		bengine::delete_component<designated_hunter_t>(id);
		bengine::entity_t * settler = bengine::entity(id);
		if (settler) {
			auto stats = settler->component<game_stats_t>();
			if (stats) {
				stats->profession_tag = stats->original_profession;
			}
		}
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
}