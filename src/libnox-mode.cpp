#include "libnox.h"
#include "global_assets/game_pause.hpp"
#include "global_assets/game_mode.hpp"

namespace nf {
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

	void get_game_mode(int &major, int &minor) {
		major = game_master_mode;
		minor = game_design_mode;
	}

	void set_game_mode(int major, int minor) {
		game_master_mode = (game_master_mode_t)major;
		game_design_mode = (game_design_mode_t)minor;
	}
}