#include "tick_system.hpp"
#include "../../global_assets/game_pause.hpp"
#include "../../global_assets/game_config.hpp"

namespace systems {
    namespace tick {
		const double MS_PER_TICK = 33.3;
        double time_count = 0.0;
        int slow_tick_count = 0;

        void run(const double &duration_ms) {
            major_tick = false;
            slow_tick = false;
            hour_elapsed = false;
            day_elapsed = false;

            if (pause_mode != PAUSED) {
                time_count += duration_ms;

                if (time_count > MS_PER_TICK) {
                    time_count = 0.0;
                    major_tick = true;

                    ++slow_tick_count;
                    if (slow_tick_count > 9) {
                        slow_tick_count = 0;
                        slow_tick = true;
                    }

                    if (pause_mode == ONE_STEP) {
			        	pause_mode = PAUSED;
			        }
                }
            }
        }
    }
}
