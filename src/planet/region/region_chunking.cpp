#include "region_chunking.hpp"
#include "region.hpp"
#include "../../raws/materials.hpp"
#include "../../raws/defs/material_def_t.hpp"
#include "../../raws/buildings.hpp"
#include "../../raws/defs/building_def_t.hpp"
#include "../../global_assets/game_ecs.hpp"
#include "../../global_assets/farming_designations.hpp"
#include <array>
#include <vector>
#include <bitset>
#include <map>
#include <algorithm>

namespace region {	

	using namespace nf;

	inline int chunk_id_by_world_pos(const int x, const int y, const int z) noexcept
	{
		const int chunk_x = x / CHUNK_SIZE;
		const int chunk_y = y / CHUNK_SIZE;
		const int chunk_z = z / CHUNK_SIZE;
		return chunk_idx(chunk_x, chunk_y, chunk_z);
	}	

	struct layer_t {
		std::vector<cube_t> cubes;
		std::vector<floor_t> floors;
		std::vector<floor_t> design_mode;
	};

	struct chunk_t {
		int index = 0, base_x = 0, base_y = 0, base_z = 0;
		std::array<layer_t, CHUNK_SIZE> layers;
		std::map<int, std::vector<std::tuple<int, int, int>>> static_voxel_models;
		std::vector<std::tuple<int, int, int, int, int>> vegetation_models; // plant, state, x, y, z
	};

	std::array<chunk_t, CHUNKS_TOTAL> chunks;
	bool chunks_initialized = false;
	std::bitset<CHUNKS_TOTAL> dirty;

	void mark_chunk_dirty(const int &idx) {
		dirty.set(idx);
	}

	void mark_chunk_dirty_by_tileidx(const int &idx) {
		const auto &[x, y, z] = idxmap(idx);
		mark_chunk_dirty(chunk_idx(x / CHUNK_SIZE, y / CHUNK_SIZE, z / CHUNK_SIZE));
	}

	void setup_chunk(const int &idx, const int &x, const int &y, const int &z) {
		chunks[idx].base_x = x * CHUNK_SIZE;
		chunks[idx].base_y = y * CHUNK_SIZE;
		chunks[idx].base_z = z * CHUNK_SIZE;
		chunks[idx].index = idx;
	}

	void initialize_chunks() {
		dirty.reset();
		for (int z = 0; z<CHUNK_DEPTH; ++z) {
			for (int y = 0; y<CHUNK_HEIGHT; ++y) {
				for (int x = 0; x<CHUNK_WIDTH; ++x) {
					const int idx = chunk_idx(x, y, z);
					setup_chunk(idx, x, y, z);
					mark_chunk_dirty(idx);
				}
			}
		}
	}

	static bool is_cube(const uint8_t type) {
		switch (type) {
		case tile_type::SOLID: return true;
		case tile_type::WALL: return true;
		case tile_type::SEMI_MOLTEN_ROCK: return true;
		default: return false;
		}
	}

	void greedy_floors(std::map<int, unsigned int> &floors, const int &chunk_idx, const int &chunk_z) {
		auto n_floors = floors.size();
		const int base_x = chunks[chunk_idx].base_x;
		const int base_y = chunks[chunk_idx].base_y;
		const int base_z = chunks[chunk_idx].base_z;

		while (!floors.empty()) {
			const auto first_floor = floors.begin();
			const auto base_region_idx = first_floor->first;
			const auto texture_id = first_floor->second;

			const auto &[tile_x, tile_y, tile_z] = idxmap(base_region_idx);
			auto width = 1;
			auto height = 1;

			floors.erase(base_region_idx);
			auto idx_grow_right = base_region_idx + 1;
			auto x_coordinate = tile_x;
			auto right_finder = floors.find(idx_grow_right);
			while (x_coordinate < REGION_WIDTH - 1 && idx_grow_right < mapidx(std::min(REGION_WIDTH - 1, base_x + CHUNK_SIZE), tile_y, tile_z) && right_finder != floors.end() && right_finder->second == texture_id) {
				floors.erase(idx_grow_right);
				++width;
				++idx_grow_right;
				++x_coordinate;
				right_finder = floors.find(idx_grow_right);
			}

			if (tile_y < REGION_HEIGHT - 1) {
				auto y_progress = tile_y + 1;

				auto possible = true;
				while (possible && y_progress < base_y + CHUNK_SIZE && y_progress < REGION_HEIGHT - 1) {
					for (auto gx = tile_x; gx < tile_x + width; ++gx) {
						const auto candidate_idx = mapidx(gx, y_progress, tile_z);
						const auto vfinder = floors.find(candidate_idx);
						if (vfinder == floors.end() || vfinder->second != texture_id) possible = false;
					}
					if (possible) {
						++height;
						for (auto gx = tile_x; gx < tile_x + width; ++gx) {
							const auto candidate_idx = mapidx(gx, y_progress, tile_z);
							floors.erase(candidate_idx);
						}
					}

					++y_progress;
				}
			}

			chunks[chunk_idx].layers[chunk_z].floors.emplace_back(floor_t{ tile_x, tile_y, tile_z, width, height, texture_id });
		}
	}

	void greedy_design(std::map<int, unsigned int> &floors, const int &chunk_idx, const int &chunk_z) {
		auto n_floors = floors.size();
		const int base_x = chunks[chunk_idx].base_x;
		const int base_y = chunks[chunk_idx].base_y;
		const int base_z = chunks[chunk_idx].base_z;

		while (!floors.empty()) {
			const auto first_floor = floors.begin();
			const auto base_region_idx = first_floor->first;
			const auto texture_id = first_floor->second;

			const auto &[tile_x, tile_y, tile_z] = idxmap(base_region_idx);
			auto width = 1;
			auto height = 1;

			floors.erase(base_region_idx);
			auto idx_grow_right = base_region_idx + 1;
			auto x_coordinate = tile_x;
			auto right_finder = floors.find(idx_grow_right);
			while (x_coordinate < REGION_WIDTH - 1 && idx_grow_right < mapidx(std::min(REGION_WIDTH - 1, base_x + CHUNK_SIZE), tile_y, tile_z) && right_finder != floors.end() && right_finder->second == texture_id) {
				floors.erase(idx_grow_right);
				++width;
				++idx_grow_right;
				++x_coordinate;
				right_finder = floors.find(idx_grow_right);
			}

			if (tile_y < REGION_HEIGHT - 1) {
				auto y_progress = tile_y + 1;

				auto possible = true;
				while (possible && y_progress < base_y + CHUNK_SIZE && y_progress < REGION_HEIGHT - 1) {
					for (auto gx = tile_x; gx < tile_x + width; ++gx) {
						const auto candidate_idx = mapidx(gx, y_progress, tile_z);
						const auto vfinder = floors.find(candidate_idx);
						if (vfinder == floors.end() || vfinder->second != texture_id) possible = false;
					}
					if (possible) {
						++height;
						for (auto gx = tile_x; gx < tile_x + width; ++gx) {
							const auto candidate_idx = mapidx(gx, y_progress, tile_z);
							floors.erase(candidate_idx);
						}
					}

					++y_progress;
				}
			}

			chunks[chunk_idx].layers[chunk_z].design_mode.emplace_back(floor_t{ tile_x, tile_y, tile_z, width, height, texture_id });
		}
	}

	void greedy_cubes(std::map<int, unsigned int> &cubes, const int &chunk_idx, const int &chunk_z) {
		const int base_x = chunks[chunk_idx].base_x;
		const int base_y = chunks[chunk_idx].base_y;
		const int base_z = chunks[chunk_idx].base_z;

		auto n_cubes = cubes.size();
		while (!cubes.empty()) {
			const auto first_floor = cubes.begin();
			const auto base_region_idx = first_floor->first;
			const auto texture_id = first_floor->second;

			const auto &[tile_x, tile_y, tile_z] = idxmap(base_region_idx);
			auto width = 1;
			auto height = 1;

			cubes.erase(base_region_idx);
			auto idx_grow_right = base_region_idx + 1;
			auto x_coordinate = tile_x;
			auto right_finder = cubes.find(idx_grow_right);
			while (x_coordinate < REGION_WIDTH - 1 && idx_grow_right < mapidx(std::min(REGION_WIDTH - 1, base_x + CHUNK_SIZE), tile_y, tile_z) && right_finder != cubes.end() && right_finder->second == texture_id) {
				cubes.erase(idx_grow_right);
				++width;
				++idx_grow_right;
				++x_coordinate;
				right_finder = cubes.find(idx_grow_right);
			}

			//std::cout << "Merging " << width << " tiles horizontally\n";

			if (tile_y < REGION_HEIGHT - 1) {
				auto y_progress = tile_y + 1;

				auto possible = true;
				while (possible && y_progress < base_y + CHUNK_SIZE && y_progress < REGION_HEIGHT - 1) {
					for (auto gx = tile_x; gx < tile_x + width; ++gx) {
						const auto candidate_idx = mapidx(gx, y_progress, tile_z);
						const auto vfinder = cubes.find(candidate_idx);
						if (!(vfinder != cubes.end() && vfinder->second == texture_id)) possible = false;
					}
					if (possible) {
						++height;
						for (int gx = tile_x; gx < tile_x + width; ++gx) {
							const auto candidate_idx = mapidx(gx, y_progress, tile_z);
							cubes.erase(candidate_idx);
						}
					}

					++y_progress;
				}
			}

			chunks[chunk_idx].layers[chunk_z].cubes.emplace_back(cube_t{ tile_x, tile_y, tile_z, width, height, 1, texture_id });
		}
	}

	unsigned int get_floor_tex(const int &idx) {
		// If its a stockpile, render it as such
		if (region::stockpile_id(idx) > 0) return 3; // TODO: Determine texture

													 // We no longer hard-code grass.
		if (region::veg_type(idx) > 0 && !region::flag(idx, tile_flags::CONSTRUCTION)) {
			switch (region::veg_lifecycle(idx)) {
			case 0: return 18; // Germination
			case 1: return 21; // Sprouting
			case 2: return 0; // Growing (grass is material 0)
			case 3: return 24; // Flowering
			}
			return 0; // Grass is determined to be index 0
		}
		const auto material_idx = region::material(idx);
		const auto material = get_material(material_idx);
		if (!material) return 3;

		unsigned int use_id = 3;
		if (region::flag(idx, tile_flags::CONSTRUCTION)) {
			use_id = (unsigned int)material->constructed_floor_texture_id;
		}
		else {
			use_id = (unsigned int)material->floor_texture_id;
		}
		if (use_id == 3) {
			//glog(log_target::LOADER, log_severity::warning, "Material [{0}] is lacking a texture", material->name);
		}

		//std::cout << material->name << "=" << use_id << "\n";
		return use_id;
	}

	unsigned int get_cube_tex(const int &idx) {
		const auto tt = region::tile_type(idx);
		if (tt == tile_type::TREE_TRUNK) return 6;
		if (tt == tile_type::TREE_LEAF) return 9;

		const auto material_idx = region::material(idx);
		const auto material = get_material(material_idx);
		if (!material) return 3;

		unsigned int use_id = 3;
		if (!region::flag(idx, tile_flags::CONSTRUCTION)) {
			use_id = (unsigned int)material->constructed_texture_id;
		}
		else {
			use_id = (unsigned int)material->base_texture_id;
		}
		if (use_id == 3) {
			//glog(log_target::LOADER, log_severity::warning, "Material [{0}] is lacking a texture.", material->name);
		}
		return use_id;
	}

	unsigned int get_design_tex(const int &idx) {
		const auto tt = region::tile_type(idx);

		// Default graphics for open space and not-yet-revealed
		if (tt == tile_type::OPEN_SPACE) return 3;
		if (!region::flag(idx, tile_flags::REVEALED)) return 3;
		if (tt == tile_type::FLOOR) return get_floor_tex(idx);
		if (tt == tile_type::TREE_TRUNK) return 6;
		return get_cube_tex(idx);
	}

	void update_chunk(const int &chunk_idx) {
		for (auto &layer : chunks[chunk_idx].layers) {
			layer.cubes.clear();
			layer.floors.clear();
			layer.design_mode.clear();
		}
		chunks[chunk_idx].static_voxel_models.clear();
		chunks[chunk_idx].vegetation_models.clear();

		const int base_x = chunks[chunk_idx].base_x;
		const int base_y = chunks[chunk_idx].base_y;
		const int base_z = chunks[chunk_idx].base_z;


		for (int chunk_z = 0; chunk_z < CHUNK_SIZE; ++chunk_z) {
			const int region_z = chunk_z + base_z;
			std::map<int, unsigned int> floors;
			std::map<int, unsigned int> cubes;
			std::map<int, unsigned int> design_mode;

			for (int chunk_y = 0; chunk_y < CHUNK_SIZE; ++chunk_y) {
				const int region_y = chunk_y + base_y;
				for (int chunk_x = 0; chunk_x < CHUNK_SIZE; ++chunk_x) {
					const int region_x = chunk_x + base_x;
					const int ridx = mapidx(region_x, region_y, region_z);

					const auto tiletype = region::tile_type(ridx);
					design_mode.insert(std::make_pair(ridx, get_design_tex(ridx)));
					if (tiletype != tile_type::OPEN_SPACE) {
						if (region::flag(ridx, tile_flags::REVEALED)) {
							if (tiletype == tile_type::WINDOW) {
								cubes.insert(std::make_pair(ridx, 15));
							}
							else if (tiletype == tile_type::FLOOR) {
								floors.insert(std::make_pair(ridx, get_floor_tex(ridx)));
								if (farm_designations->farms.find(ridx) != farm_designations->farms.end()) {
									chunks[chunk_idx].static_voxel_models[116].push_back(std::make_tuple(region_x, region_y, region_z));
								}

								if (region::veg_type(ridx) > 0 && !region::flag(ridx, tile_flags::CONSTRUCTION)) {
									chunks[chunk_idx].vegetation_models.emplace_back(std::make_tuple<int, int, int, int, int>( (int)region::veg_type(ridx), (int)region::veg_lifecycle(ridx), (int)region_x, (int)region_y, (int)region_z ));
								}
							}
							else if (tiletype == tile_type::TREE_TRUNK) {
								chunks[chunk_idx].vegetation_models.emplace_back(std::make_tuple<int, int, int, int, int>(-1, 0, (int)region_x, (int)region_y, (int)region_z));
							}
							else if (is_cube(tiletype))
							{
								cubes.insert(std::make_pair(ridx, get_cube_tex(ridx)));
							}
							else if (tiletype == tile_type::RAMP) {
								// TODO: Handle differently
								cubes.insert(std::make_pair(ridx, get_cube_tex(ridx)));
							}
							else if (tiletype == tile_type::STAIRS_DOWN) {
								chunks[chunk_idx].static_voxel_models[24].push_back(std::make_tuple(region_x, region_y, region_z));
							}
							else if (tiletype == tile_type::STAIRS_UP) {
								chunks[chunk_idx].static_voxel_models[23].push_back(std::make_tuple(region_x, region_y, region_z));
							}
							else if (tiletype == tile_type::STAIRS_UPDOWN) {
								chunks[chunk_idx].static_voxel_models[25].push_back(std::make_tuple(region_x, region_y, region_z));
							}
							else if (tiletype == tile_type::CLOSED_DOOR) {
								auto vox_id = 128;
								const auto bid = region::get_building_id(ridx);
								if (bid > 0)
								{
									const auto building_entity = bengine::entity(bid);
									if (building_entity)
									{
										const auto building_comp = building_entity->component<building_t>();
										if (building_comp)
										{
											const auto def = get_building_def(building_comp->tag);
											if (def)
											{
												for (const auto &p : def->provides)
												{
													if (p.alternate_vox > 0) vox_id = p.alternate_vox;
												}
											}
										}
									}
								}
								chunks[chunk_idx].static_voxel_models[vox_id].push_back(std::make_tuple(region_x, region_y, region_z));
							}
						} // revealed
						else {
							cubes.insert(std::make_pair(ridx, 3));
						}
					}
				}
			}			

			greedy_floors(floors, chunk_idx, chunk_z);
			greedy_cubes(cubes, chunk_idx, chunk_z);
			greedy_design(design_mode, chunk_idx, chunk_z);
		}
	}

	void update_chunks() {
		for (auto i = 0; i < CHUNKS_TOTAL; ++i) {
			if (dirty.test(i)) update_chunk(i);
		}
		dirty.reset();
	}

	void update_chunks_listing_changes(std::vector<int> &dirty_list) {
		for (auto i = 0; i < CHUNKS_TOTAL; ++i) {
			if (dirty.test(i)) {
				update_chunk(i);
				dirty_list.emplace_back(i);
			}
		}
		dirty.reset();
	}

	void get_chunk_floors(const int &chunk_idx, const int &chunk_z, size_t &size, floor_t *& floor_ptr) {
		size = chunks[chunk_idx].layers[chunk_z].floors.size();
		floor_ptr = size > 0 ? &chunks[chunk_idx].layers[chunk_z].floors[0] : nullptr;
	}

	void get_chunk_cubes(const int &chunk_idx, const int &chunk_z, size_t &size, cube_t *& cube_ptr) {
		size = chunks[chunk_idx].layers[chunk_z].cubes.size();
		if (size > 0) {
			cube_ptr = &(chunks[chunk_idx].layers[chunk_z].cubes[0]);
		}
		else {
			cube_ptr = nullptr;
		}
	}

	void get_chunk_coordinates(const int &idx, int &x, int &y, int &z) {
		x = chunks[idx].base_x;
		y = chunks[idx].base_y;
		z = chunks[idx].base_z;
	}

	void get_chunk_models(const int &chunk_idx, std::vector<nf::static_model_t> &models) {
		for (const auto &m : chunks[chunk_idx].static_voxel_models) {
			for (const auto &n : m.second) {
				models.emplace_back(nf::static_model_t{ m.first, std::get<0>(n), std::get<1>(n), std::get<2>(n) });
			}
		}
	}

	void get_chunk_veg(const int &chunk_idx, std::vector<nf::veg_t> &veg) {
		for (const auto &v : chunks[chunk_idx].vegetation_models) {
			veg.emplace_back(nf::veg_t{ std::get<0>(v), std::get<1>(v), std::get<2>(v), std::get<3>(v), std::get<4>(v) });
		}
	}

	void get_chunk_design_mode(const int &chunk_idx, const int &chunk_z, size_t &size, nf::floor_t *& floor_ptr) {
		size = chunks[chunk_idx].layers[chunk_z].design_mode.size();
		floor_ptr = size > 0 ? &chunks[chunk_idx].layers[chunk_z].design_mode[0] : nullptr;
	}
}
