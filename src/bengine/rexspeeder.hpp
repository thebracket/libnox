#pragma once
/* RLTK (RogueLike Tool Kit) 1.00
 * Copyright (c) 2016-Present, Bracket Productions.
 * Licensed under the MIT license - see LICENSE file.
 *
 * REXPaint 1.02 file reader. Credit to https://github.com/pyridine/REXSpeeder for the original.
 */

/*For version 1.02 of REXPaint*/
#include <stdint.h>
#include <string>
#include <vector>
#include <array>
#include "color_t.hpp"

namespace xp {

	struct vchar {
		vchar() {}
		vchar(uint16_t g, bengine::color_t fg, bengine::color_t bg) : glyph(g), foreground(fg), background(bg) {}
		vchar(uint16_t gl, uint8_t fr, uint8_t fg, uint8_t fb, uint8_t br, uint8_t bg, uint8_t bb) :
			glyph(gl), foreground{fr, fg, fb}, background{br, bg, bb} {}

		uint16_t glyph;
		bengine::color_t foreground;
		bengine::color_t background;

		template<class Archive>
		void serialize(Archive & archive)
		{
			archive( glyph, foreground, background ); // serialize things by passing them to the archive
		}
	};

	//REXpaint identifies transparent tiles by setting their background color to 255,0,255.
	//You may want to check this for each tile before drawing or converting a RexFile.
	//(By default, no tile in the first layer is transaprent).
	inline bool is_transparent(const vchar * tile)
	{
		//This might be faster than comparing with transparentTile(), despite it being a constexpr
		return (tile->background.r == 255 && tile->background.g == 0 && tile->background.b == 255);
	}	

	//Returns a transparent tile.
	inline vchar transparent_tile() {
		return vchar{0, 0, 0, 0, 255, 0, 255};
	}

	struct rex_layer {
		rex_layer() {}
		rex_layer(int width, int height) 
		{
			tiles.resize(width * height);
		} 

		~rex_layer()
		{
			tiles.clear();
		}

		std::vector<vchar> tiles;
	};

	class rex_sprite {
	public:
		//Load an .xp file into a new RexFile.
		//Note: May throw a const char* error message and set errno.
		//Both the error message and the value of errno may be as gzopen or gzread set them.
		//It may also throw an error with code REXSPEEDER_FILE_DOES_NOT_EXIST.
		//Will not throw an error if the file specified by `filename` is not zlib compressed.
		rex_sprite(std::string const& filename);

		//Save this RexFile into a valid .xp file that RexPaint can load (if the ".xp" suffix is present).
		//Note: May throw a const char* error message and set errno.
		//Both the error message and the value of errno may be as gzopen or gzwrite set them.
		void save(std::string const& filename);

		//Create a blank RexFile with the specified attributes.
		//Layers above the first will be made of transparent tiles.
		rex_sprite(int _version, int _width, int _height, int _num_layers);

		//Image attributes
		inline int get_version() { return version; };
		inline int get_width() { return width; };
		inline int get_height() { return height; };
		inline int get_num_layers() { return num_layers; };

		//Returns a pointer to a single tile specified by layer, x coordinate, y coordinate.
		//0,0 is the top-left corner.
		inline vchar* get_tile(int layer, int x, int y) { return &layers[layer].tiles[y + (x * height)]; };

		//Returns a pointer to a single tile specified by layer and the actual index into the array.
		//Useful for iterating through a whole layer in one go for coordinate-nonspecific tasks.
		inline vchar* get_tile(int layer, int index) { return &layers[layer].tiles[index]; };

		//Replaces the data for a tile. Not super necessary, but might save you a couple lines.
		inline void set_tile(int layer, int x, int y, vchar& val) { *get_tile(layer, x, y) = val; };

		//Replaces the data for a tile. Not super necessary, but might save you a couple lines.
		inline void set_tile(int layer, int i, vchar& val) { *get_tile(layer, i) = val; };

		//Combines all the layers of the image into one layer.
		//Respects transparency.
		void flatten();

	private:
		//Image properties
		int version;
		int width, height, num_layers;
		std::vector<rex_layer> layers; //layers[0] is the first layer.

		//Forbid default construction.
		rex_sprite();
	};
}
