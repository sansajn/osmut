#include <catch.hpp>
#include "road_reader.hpp"

using std::vector;

TEST_CASE("read roads from osm file")
{
	box bbox;
	vector<road> roads;
	read_roads("roads.osm", roads, bbox);

	REQUIRE(roads.size() == 27);
}
