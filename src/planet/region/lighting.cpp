#include "lighting.hpp"
#include "../../global_assets/game_ecs.hpp"
#include "../../global_assets/game_designations.hpp"

namespace render {
	void get_light_list(std::vector<nf::dynamic_lightsource_t> &lights) {
		using namespace bengine;

		float power_percent = (float)designations->current_power / (float)designations->total_capacity;
		bengine::color_t alert_color;
		if (power_percent < 0.3) {
			alert_color = color_t(1.0f, 0.0f, 0.0f);
		}
		else if (power_percent < 0.5) {
			alert_color = color_t(1.0f, 0.5f, 0.0f);
		}
		else if (power_percent < 0.75) {
			alert_color = color_t(1.0f, 1.0f, 0.0f);
		}
		else {
			alert_color = { 1.0f, 1.0f, 1.0f };
		}

		each<lightsource_t, position_t>([&lights, &alert_color](entity_t &e, lightsource_t &l, position_t &pos) {
			if (l.alert_status) {
				lights.emplace_back(nf::dynamic_lightsource_t{ (float)pos.x, (float)pos.y, (float)pos.z, alert_color.r, alert_color.g, alert_color.b, (float)l.radius, e.id });
			}
			else {
				lights.emplace_back(nf::dynamic_lightsource_t{ (float)pos.x, (float)pos.y, (float)pos.z, l.color.r, l.color.g, l.color.b, (float)l.radius, e.id });
			}
		});
	}
}