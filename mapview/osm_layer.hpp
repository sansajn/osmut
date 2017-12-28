#pragma once
#include <array>
#include <memory>
#include <glm/glm.hpp>
#include <pangomm.h>
#include "mapview_layer.hpp"
#include "tile_source.hpp"

class osm_layer : public mapview_layer
{
public:
	osm_layer(std::unique_ptr<tile_source> tiles);
	void draw(Cairo::RefPtr<Cairo::Context> const & cr) override;
	void resize(int w, int h) override;
	void zoom(size_t z) override;
	void map_to_window_transform(glm::dvec2 const & T) override;

private:
	std::array<glm::uvec2, 2> visible_tiles() const;

	std::unique_ptr<tile_source> _tiles;
	glm::dvec2 _window;
	size_t _zoom;
	glm::dvec2 _map_to_widow;

	Pango::FontDescription _font;  // TODO: what about to move fonts to some king of font manager ?
};
