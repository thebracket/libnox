#include <iostream>
#include "../src/libnox.h"
#include "../src/noxtypes.h"
#include "../src/noxconsts.h"
#include "../src/raws/plants.hpp"

int main() {
	std::cout << "Loading the planet...\n";
	nf::setup_planet();

	std::cout << "Saving the planet...\n";
	nf::serialize_planet();

	std::cout << "Loading Raws\n";
	nf::setup_raws();

	std::cout << "Loading the game\n";
	nf::load_game();

	std::cout << "Initializing chunks\n";
	nf::chunks_init();

	std::cout << "Updating chunks\n";
	nf::chunks_update();

	std::cout << "Dumping chunk floors\n";
	size_t total_floors = 0;
	size_t n_floors = 0;
	nf::floor_t * floor_ptr = nullptr;
	for (int i = 0; i < nf::CHUNKS_TOTAL; ++i) {
		for (int j = 0; j < nf::CHUNK_SIZE; ++j) {
			nf::chunk_floors(i, j, n_floors, floor_ptr);
			total_floors += n_floors;
		}
	}
	std::cout << "There are " << total_floors << " floors available\n";

	std::cout << "Dumping chunk cubes\n";
	size_t total_cubes = 0;
	size_t n_cubes = 0;
	nf::cube_t * cube_ptr = nullptr;
	for (int i = 0; i < nf::CHUNKS_TOTAL; ++i) {
		for (int j = 0; j < nf::CHUNK_SIZE; ++j) {
			nf::chunk_cubes(i, j, n_cubes, cube_ptr);
			total_cubes += n_cubes;
		}
	}
	std::cout << "There are " << total_cubes << " cubes available\n";

	size_t n_water = 0;
	nf::water_t * water_ptr = nullptr;
	nf::water_cubes(n_water, water_ptr);
	std::cout << "There are " << n_water << " wet tiles.\n";

	std::cout << "Running a paused tick\n";
	nf::set_pause_mode(1);
	nf::on_tick(20.0);

	std::cout << "Running a single step tick\n";
	nf::set_pause_mode(2);
	nf::on_tick(20.0);

	std::cout << "Running a free tick\n";
	nf::set_pause_mode(0);
	nf::on_tick(20.0);

	std::stringstream ss;
	dump_plant_data(ss);
	std::cout << ss.str() << "\n";

	std::cout << "Press ENTER to close\n";
	std::cin.ignore();
}