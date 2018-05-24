#include <vector>
#include <string>
#include <boost/geometry.hpp>
#include <glm/vec2.hpp>
#include "agg_color_rgba.h"
#include "glmadapt.hpp"

using vec2 = glm::dvec2;
using box = boost::geometry::model::box<vec2>;


enum class road_type  //!< see https://wiki.openstreetmap.org/wiki/Key:highway for more details
{
	motorway,
	trunk,
	primary,
	secondary,
	tertiary,
	unclassified,
	residential,
	service,
	living_street,
	pedestrian,
	track,
	bus_guideway,
	escape,
	raceway,
	road,
	footway,
	bridleway,
	steps,
	path,
	cycleway,
	construction,
};

struct road
{
	road_type type;
	std::vector<vec2> geometry;
};

struct road_style
{
	agg::rgba color1, color2;
	double width;

	road_style(agg::rgba const & color1, agg::rgba const & color2, double width)
		: color1{color1}, color2{color2}, width{width}
	{}
};

void read_roads(std::string const & osm_file, std::vector<road> & roads,
	box & bounding_box);
