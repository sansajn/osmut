#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "tile.hpp"

class tile_source
{
public:
	virtual std::shared_ptr<tile> get(size_t zoom, size_t x, size_t y) = 0;
};
