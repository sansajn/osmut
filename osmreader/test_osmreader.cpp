#include <gtest/gtest.h>
#include "osmut/osm_iter.h"

char const * romanova_osm = "../assets/maps/romanova.osm";
char const * romanova_josm_osm = "../assets/maps/romanova_josm.osm";

TEST(osm_reader_test, node_way_and_relation_iters)
{
	osmut::xml_reader osm{romanova_osm};

	unsigned count = 0;
	for (node_iterator it{osm}; it != node_iterator{}; ++it, ++count)
		;

	EXPECT_EQ(565, count);

	count = 0;
	for (way_iterator it{osm}; it != way_iterator{}; ++it, ++count)
		;

	EXPECT_EQ(81, count);

	count = 0;
	for (relation_iterator it{osm}; it != relation_iterator{}; ++it, ++count)
		;

	EXPECT_EQ(7, count);
}

TEST(osm_reader_test, josm_node_way_and_relation_iters)
{
	osmut::xml_reader osm{romanova_josm_osm};

	unsigned count = 0;
	for (node_iterator it{osm}; it != node_iterator{}; ++it, ++count)
		;

	EXPECT_EQ(565, count);

	count = 0;
	for (way_iterator it{osm}; it != way_iterator{}; ++it, ++count)
		;

	EXPECT_EQ(81, count);

	count = 0;
	for (relation_iterator it{osm}; it != relation_iterator{}; ++it, ++count)
		;

	EXPECT_EQ(7, count);
}

int main(int argc, char * argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
