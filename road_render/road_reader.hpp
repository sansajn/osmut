#include <vector>
#include <string>
#include <boost/geometry.hpp>

using point = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
using box = boost::geometry::model::box<point>;


enum class road_type  //!< see https://wiki.openstreetmap.org/wiki/Key:highway for more details
{
	motorway,
	trunk,
	primary,
	secondary,
	tertiary,
	unclassified,
	residential,
	service
};

struct road
{
	road_type type;
	std::vector<point> geometry;
};


void read_roads(std::string const & osm_file, std::vector<road> & roads,
	box & bounding_box);
