#include <cstdlib>
#include <cstring>
#include <cassert>
#include "reader_impl.h"
#include "osmff.h"

using std::map;
using std::shared_ptr;
using std::string;

void process_node_tag(osmut::parser & osm, node & n);
void process_way_tag(osmut::parser & osm, way & w);
void process_tag_tag(osmut::parser & osm, tagmap & tags);


bool node_reader::tag(std::string const & node_name)
{
	return node_name == NODE_TAG;
}

bool node_reader::stop_tag(std::string const & node_name)
{
	return node_name == WAY_TAG || node_name == RELATION_TAG;
}

void node_reader::read_tag(osmut::parser & osm, node & n)
{
	n.tags.reset();
	process_node_tag(osm, n);
}

bool way_reader::tag(std::string const & node_name)
{
	return node_name == WAY_TAG;
}

bool way_reader::stop_tag(std::string const & node_name)
{
	return node_name == RELATION_TAG;
}

void way_reader::read_tag(osmut::parser & osm, way & w)
{
	w.tags.reset();
	w.node_ids.clear();
	process_way_tag(osm, w);
}

void process_node_tag(osmut::parser & osm, node & n)
{
	int n_attribs = osm.attribute_count();

	assert(n_attribs > 2 
		&& "wrong node format (at least 'id', 'lat' and 'lon' "
			"attributes expected)");

	assert(osm.attribute_name(0) == "id" && "unexpected attribute order");
	assert(osm.attribute_name(1) == "lat" && "unexpected attribute order");
	assert(osm.attribute_name(2) == "lon" && "unexpected attribute order");

	n.id = atoi(osm.attribute_value(0).c_str());
	n.lat = atof(osm.attribute_value(1).c_str());
	n.lon = atof(osm.attribute_value(2).c_str());

	while (osm.read())
	{
		osmut::parser::e_node type = osm.node_type();

		if (type == osmut::parser::start_node)
		{
			std::string const & node_name = osm.node_name();

			if (node_name == TAG_TAG)
			{
				if (!n.tags)
					n.tags = shared_ptr<tagmap>(new tagmap);

				process_tag_tag(osm, *n.tags);
			}

			assert(node_name == "tag" &&
				"unexpected node inside 'node' (only 'tag' allowed)");
		}
		else if (type == osmut::parser::end_node || type == osmut::parser::empty_node)
			break;
	}
}

void process_way_tag(osmut::parser & osm, way & w)
{
	assert(osm.attribute_name(0) == "id"
		&&	"unexpected attribute order ('id' expected)");

	w.id = atoi(osm.attribute_value(0).c_str());

	while (osm.read())
	{
		osmut::parser::e_node type = osm.node_type();

		if (type == osmut::parser::start_node)
		{
			std::string const & node_name = osm.node_name();
			assert(
				node_name == "nd" ||
				node_name == "tag" &&
				"unexpected node inside 'way' ('nd' or 'tag' allowed)");

			if (node_name == NODEREF_TAG)
			{
				assert(osm.attribute_name(0) == "ref"
					&& "unexpected attribute order ('ref' expected)");

				w.node_ids.push_back(atoi(osm.attribute_value(0).c_str()));
			}
			else if (node_name == TAG_TAG)
			{
				if (!w.tags)
					w.tags = shared_ptr<tagmap>(new tagmap);
				process_tag_tag(osm, *w.tags);
			}

		}
		else if (type == osmut::parser::end_node || type == osmut::parser::empty_node)
			break;
	}

//	assert(w.tags && "'way' node without tag");

	assert(!w.node_ids.empty() && "empty 'way' node");
}

void process_tag_tag(osmut::parser & osm, tagmap & tags)
{
	assert(osm.attribute_count() == 2 &&
		"unexpected number of 'tag' arguments (two expected)");

	assert(osm.attribute_name(0) == "k" &&
		"unexpected 'tag' attribute ('k' expected)");

	assert(osm.attribute_name(1) == "v" &&
		"unexpected 'tag' attribute ('v' expected)");

	tags[osm.attribute_value(0)] = osm.attribute_value(1);
}

