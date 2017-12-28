#pragma once
#include <glm/glm.hpp>
#include <cairomm/cairomm.h>

class mapview_layer
{
public:
	virtual void draw(Cairo::RefPtr<Cairo::Context> const & cr) = 0;
	virtual void resize(int w, int h) {}
	virtual void map_to_window_transform(glm::dvec2 const & T) {}
	virtual void zoom(size_t z) {}
};
