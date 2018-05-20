#pragma once

#include <string>
#include <vector>
#include "../../bengine/color_t.hpp"

/*
 * Defines a material type. Used by just about everything.
 */
struct material_def_t {
    std::string tag = "";
    std::string name = "";
    material_def_spawn_type_t spawn_type = ROCK;
    std::string parent_material_tag = "";
    uint16_t glyph;
    bengine::color_t fg;
    bengine::color_t bg;
    uint8_t hit_points = 0;
    std::string mines_to_tag = "";
    std::string mines_to_tag_second = "";
    std::string layer = "";
    std::vector<std::string> ore_materials;
    int damage_bonus = 0;
    float ac_bonus = 0.0F;
	std::string floor_rough;
	std::string floor_smooth;
	std::string wall_rough;
	std::string wall_smooth;
	int floor_rough_id;
	int floor_smooth_id;
	int wall_rough_id;
	int wall_smooth_id;
};
