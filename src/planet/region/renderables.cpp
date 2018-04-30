#include "renderables.hpp"
#include "../../global_assets/game_ecs.hpp"
#include "../../global_assets/game_camera.hpp"
#include "../../global_assets/game_mode.hpp"
#include "../../global_assets/building_designations.hpp"
#include "../../raws/species.hpp"
#include "region.hpp"
#include <map>
#include <vector>

namespace render {
	struct instance_t {
		float x, y, z;
		float axis1, axis2, axis3, rot_angle;
		float tint_r, tint_g, tint_b;
		int entity_id;
	};

	std::map<int, std::vector<instance_t>> models_to_render;

	static inline void add_voxel_model(const int &model, const int &id, const float &x, const float &y, const float &z, const float &red, const float &green, const float &blue, const float angle = 0.0f, const float x_rot = 0.0f, const float y_rot = 0.0f, const float z_rot = 0.0f) {
		auto finder = models_to_render.find(model);
		if (finder != models_to_render.end()) {
			finder->second.push_back(instance_t{ x, y, z, x_rot, y_rot, z_rot, angle, red, green, blue, id });
		}
		else {
			models_to_render.insert(std::make_pair(model, std::vector<instance_t>{instance_t{ x, y, z, x_rot, y_rot, z_rot, angle, red, green, blue, id }}));
		}
	}

	static void build_voxel_buildings() {
		bengine::each<building_t, position_t>(
			[](bengine::entity_t &e, building_t &b, position_t &pos) {
			if (pos.z > camera_position->region_z - 10 && pos.z <= camera_position->region_z) {
				if (b.vox_model > 0) {
					//std::cout << "Found model #" << b.vox_model << "\n";
					auto x = static_cast<float>(pos.x);
					const auto y = static_cast<float>(pos.y);
					auto z = static_cast<float>(pos.z);

					//std::cout << b.width << " x " << b.height << "\n";

					auto red = 1.0f;
					auto green = 1.0f;
					auto blue = 1.0f;

					if (!b.complete) {
						red = 0.1f;
						green = 0.1f;
						blue = 0.1f;
					}

					add_voxel_model(b.vox_model, e.id, x, y, z, red, green, blue, static_cast<float>(pos.rotation), 0.0f, 1.0f, 0.0f);
				}
			}
		});
	}

	static void build_voxel_items() {
		bengine::each<renderable_t, position_t>([](bengine::entity_t &e, renderable_t &r, position_t &pos) {
			if (pos.z > camera_position->region_z - 10 && pos.z <= camera_position->region_z) {
				if (r.vox > 0) {
					auto x = static_cast<float>(pos.x);
					const auto y = static_cast<float>(pos.y);
					auto z = static_cast<float>(pos.z);

					add_voxel_model(r.vox, e.id, x, y, z, 1.0f, 1.0f, 1.0f);
				}
			}
		});
	}

	static bool is_lying_down(bengine::entity_t &e)
	{
		const auto sleepy = e.component<sleep_clock_t>();
		if (sleepy && sleepy->is_sleeping) return true;
		const auto health = e.component<health_t>();
		return (health && health->unconscious);
	}

	struct composite_cache_t
	{
		int voxel_model;
		float r, g, b;
	};

	static std::map<int, std::vector<composite_cache_t>> composite_cache;

	void invalidate_composite_cache_for_entity(const int &id)
	{
		composite_cache.erase(id);
	}

	static void render_settler(bengine::entity_t &e, renderable_composite_t &r, position_t &pos) {
		const auto species = e.component<species_t>();
		if (!species) return;

		if (pos.z > camera_position->region_z - 10 && pos.z <= camera_position->region_z) {
			const auto inner_x = static_cast<float>(pos.x);
			const auto inner_y = static_cast<float>(pos.y);
			const auto inner_z = static_cast<float>(pos.z);

			const auto is_upright = !is_lying_down(e);

			const auto rotation = is_upright ? static_cast<float>(pos.rotation) : 180.0f;
			const auto rot1 = is_upright ? 0.0f : 1.0f;
			const auto rot2 = is_upright ? 1.0f : 0.0f;
			const auto rot3 = 0.0f;

			const auto cache_finder = composite_cache.find(e.id);
			if (cache_finder != composite_cache.end())
			{
				for (const auto &c : cache_finder->second)
				{
					add_voxel_model(c.voxel_model, e.id, inner_x, inner_y, inner_z, c.r, c.g, c.b, rotation, rot1, rot2, rot3);
				}
				return;
			}
			std::vector<composite_cache_t> cc;

			// Clip check passed - add the model
			add_voxel_model(49, e.id, inner_x, inner_y, inner_z, species->skin_color.second.r, species->skin_color.second.g, species->skin_color.second.b, rotation, rot1, rot2, rot3);
			cc.emplace_back(composite_cache_t{ 49, species->skin_color.second.r, species->skin_color.second.g, species->skin_color.second.b });

			// Add hair
			int hair_vox;
			switch (species->hair_style) {
			case SHORT_HAIR: hair_vox = 50; break;
			case LONG_HAIR: hair_vox = 51; break;
			case PIGTAILS: hair_vox = 52; break;
			case MOHAWK: hair_vox = 53; break;
			case BALDING: hair_vox = 54; break;
			case TRIANGLE: hair_vox = 55; break;
			default: hair_vox = 0;
			}
			if (hair_vox > 0) {
				add_voxel_model(hair_vox, e.id, inner_x, inner_y, inner_z, species->hair_color.second.r, species->hair_color.second.g, species->hair_color.second.b, rotation, rot1, rot2, rot3);
				cc.emplace_back(composite_cache_t{ hair_vox, species->hair_color.second.r, species->hair_color.second.g, species->hair_color.second.b });
			}

			// Add items
			using namespace bengine;
			each<item_t, item_carried_t>([&e, &inner_x, &inner_y, &inner_z, &rotation, &rot1, &rot2, &rot3, &cc](entity_t &E, item_t &item, item_carried_t &carried) {
				if (carried.carried_by == e.id && item.clothing_layer > 0) {
					add_voxel_model(item.clothing_layer, e.id, inner_x, inner_y, inner_z, item.clothing_color.r, item.clothing_color.g, item.clothing_color.b, rotation, rot1, rot2, rot3);
					cc.emplace_back(composite_cache_t{ item.clothing_layer, item.clothing_color.r, item.clothing_color.g, item.clothing_color.b });
				}
			});

			composite_cache.insert(std::make_pair(e.id, cc));
		}
	}

	static void render_composite_sentient(bengine::entity_t &e, renderable_composite_t &r, position_t &pos) {
		// TODO: This should follow a different code path
		//std::cout << "Render sentient\n";
		//const auto idx = mapidx(pos);
		//if (region::flag(idx, tile_flags::VISIBLE)) {
			render_settler(e, r, pos);
		//}
	}

	static void build_composites() {
		bengine::each<renderable_composite_t, position_t>([](bengine::entity_t &e, renderable_composite_t &r, position_t &pos) {
			//std::cout << r.render_mode << "\n";
			if (camera->following == e.id && camera->fps) return; // Do not render yourself in FPS mode
			if (pos.z > camera_position->region_z - 10 && pos.z <= camera_position->region_z) {
				switch (r.render_mode) {
				case RENDER_SETTLER: render_settler(e, r, pos); break;
				case RENDER_SENTIENT: render_composite_sentient(e, r, pos); break;
				}
			}
		});
	}

	static void build_creature_models() {
		bengine::each<position_t, renderable_t, grazer_ai>([](bengine::entity_t &e, position_t &pos, renderable_t &r, grazer_ai &g) {
			if (r.vox != 0) {
				//std::cout << "Found critter " << r.vox << "\n";
				const auto is_upright = true;
				const auto rotation = is_upright ? static_cast<float>(pos.rotation) : 180.0f;
				const auto rot1 = is_upright ? 0.0f : 1.0f;
				const auto rot2 = is_upright ? 1.0f : 0.0f;
				const auto rot3 = 0.0f;
				add_voxel_model(r.vox, e.id, static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z), 1.0f, 1.0f, 1.0f, rotation, rot1, rot2, rot3);
			}
		});

		// Render sentients who don't have a composite component
		bengine::each_without<renderable_composite_t, position_t, sentient_ai, species_t>([](bengine::entity_t &e, position_t &pos, sentient_ai &g, species_t &species) {
			auto def = get_species_def(species.tag);
			if (def == nullptr) return;

			if (def->voxel_model != 0) {
				//std::cout << "Found critter " << r.vox << "\n";
				const auto is_upright = true;
				const auto rotation = is_upright ? static_cast<float>(pos.rotation) : 180.0f;
				const auto rot1 = is_upright ? 0.0f : 1.0f;
				const auto rot2 = is_upright ? 1.0f : 0.0f;
				const auto rot3 = 0.0f;

				add_voxel_model(def->voxel_model, e.id, static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z), 1.0f, 1.0f, 1.0f, rotation, rot1, rot2, rot3);
			}
		});
	}

	void build_voxel_list() {
		models_to_render.clear();
		build_voxel_buildings();
		build_voxel_items();
		build_composites();
		build_creature_models();
	}

	void get_model_list(std::vector<nf::dynamic_model_t> &models) {
		for (const auto &m : models_to_render) {
			for (const auto &n : m.second) {
				models.emplace_back(nf::dynamic_model_t{ 
					m.first,
					n.entity_id,
					n.x, n.y, n.z, n.axis1, n.axis2, n.axis3, n.rot_angle - 90.0f,
					n.tint_r, n.tint_g, n.tint_b
					});
			}
		}
	}
}