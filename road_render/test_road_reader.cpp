#include <catch.hpp>
#include "road.hpp"

using std::vector;

TEST_CASE("read roads from osm file")
{
	box bbox;
	vector<road> roads;
	read_roads("../assets/maps/roads.osm", roads, bbox);

	REQUIRE(roads.size() == 27);
}
