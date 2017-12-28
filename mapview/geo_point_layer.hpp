#pragma once
#include <vector>
#include <utility>
#include <glm/glm.hpp>
#include "mapview_layer.hpp"

class geo_point_layer : public mapview_layer
{
public:
	geo_point_layer();
	void draw(Cairo::RefPtr<Cairo::Context> const & cr) override;
	void map_to_window_transform(glm::dvec2 const & T) override;
	void zoom(size_t z) override;
	void add_point(glm::dvec2 const & p);  //!< param[in] p latlon position

private:
	std::vector<glm::dvec2> _points;  // [(lat, lon), ...]
	glm::dvec2 _map_to_window;
	size_t _zoom;
};
