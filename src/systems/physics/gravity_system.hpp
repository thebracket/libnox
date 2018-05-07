#pragma once

#include <array>
#include "../../noxconsts.h"

namespace systems {
	namespace gravity {
		void run(const double &duration_ms);
		extern std::array<bool, nf::REGION_TILES_COUNT> supported;
		void tile_was_removed();
	}
}
