#include "libnox.h"
#include <string>
#include "global_assets/rng.hpp"
#include "raws/string_table.hpp"

namespace nf {
	static std::string subtitle;

	static std::string get_descriptive_noun() {
		using namespace string_tables;

		const bengine::random_number_generator rng;
		return string_table(MENU_SUBTITLES)->random_entry(rng);
	}

	const char * get_game_subtitle() {
		bengine::random_number_generator rng;
		switch (rng.roll_dice(1, 8)) {
		case 1: subtitle = "Histories "; break;
		case 2: subtitle = "Chronicles "; break;
		case 3: subtitle = "Sagas "; break;
		case 4: subtitle = "Annals "; break;
		case 5: subtitle = "Narratives "; break;
		case 6: subtitle = "Recitals "; break;
		case 7: subtitle = "Tales "; break;
		case 8: subtitle = "Stories "; break;
		}

		const auto first_noun = get_descriptive_noun();
		auto second_noun = first_noun;
		while (second_noun == first_noun) {
			second_noun = get_descriptive_noun();
		}

		subtitle += "of " + first_noun + " and " + second_noun;

		return subtitle.c_str();
	}
}