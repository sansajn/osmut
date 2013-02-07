#include <cstdlib>
#include <cstring>
#include <cassert>
#include "osm_consts.h"
#include "reader_impl.h"

using std::map;
using std::shared_ptr;
using std::string;

void process_node_element(osmut::xml_reader & osm, node & n);
void process_way_element(osmut::xml_reader & osm, way & w);
void process_tag_element(osmut::xml_reader & osm, tagmap & tags);


bool node_reader::tag(std::string const & node_name)
{
	return node_name == NODE_ELEMENT;
}

bool node_reader::stop_tag(std::string const & node_name)
{
	return node_name == WAY_ELEMENT || node_name == RELATION_ELEMENT;
}

void node_reader::read_tag(osmut::xml_reader & osm, node & n)
{
	n.tags.reset();
	process_node_element(osm, n);
}

bool way_reader::tag(std::string const & node_name)
{
	return node_name == WAY_ELEMENT;
}

bool way_reader::stop_tag(std::string const & node_name)
{
	return node_name == RELATION_ELEMENT;
}

void way_reader::read_tag(osmut::xml_reader & osm, way & w)
{
	w.tags.reset();
	w.node_ids.clear();
	process_way_element(osm, w);
}

void process_node_element(osmut::xml_reader & osm, node & n)
{
	int n_attribs = osm.attribute_count();

	assert(n_attribs > 2 
		&& "wrong node format (at least 'id', 'lat' and 'lon' "
			"attributes expected)");

	// must be read before attributes iteration
	bool empty_element = osm.empty_element();

	// we only care about id, lat and lon attributes
	osmut::attribute_range attrs = osm.attributes();

	n.id = atoi((*attrs).second);
	assert(strcmp((*attrs).first, "id") == 0
		&& "unexpected attribute order 'id' expected");
	++attrs;

	n.lat = atof((*attrs).second);
	assert(strcmp((*attrs).first, "lat") == 0
		&& "unexpected attribute order 'lat' expected");
	++attrs;

	n.lon = atof((*attrs).second);
	assert(strcmp((*attrs).first, "lon") == 0
		&& "unexpected attribute order, 'lon' epexted");

	if (empty_element)
		return;  // no else node element data to read

	while (osm.read())  // raed node tag data
	{
		int node_type = osm.node_type();

		if (node_type == XML_READER_TYPE_ELEMENT)
		{
			char const * node_name = osm.node_name();

			if (strcmp(node_name, TAG_ELEMENT) == 0)
			{
				if (!n.tags)
					n.tags = shared_ptr<tagmap>(new tagmap);

				process_tag_element(osm, *n.tags);
			}

			assert(strcmp(node_name, "tag") == 0
				&&	"unexpected element inside 'node' (only 'tag' allowed)");
		}
		else if (node_type == XML_READER_TYPE_END_ELEMENT)
			break;
	}
}

void process_way_element(osmut::xml_reader & osm, way & w)
{
	assert(!osm.empty_element() && "empty 'way' element");

	osmut::attribute_range attrs = osm.attributes();

	w.id = atoi((*attrs).second);
	assert(strcmp((*attrs).first, "id") == 0
		&&	"unexpected attribute order ('id' expected)");	

	while (osm.read())  // read ways data elements
	{
		int node_type = osm.node_type();

		if (node_type == XML_READER_TYPE_ELEMENT)
		{
			char const * node_name = osm.node_name();

			assert(strcmp(node_name, "nd") == 0 || strcmp(node_name, "tag") == 0
				&&	"unexpected node inside 'way' element ('nd' or 'tag' allowed)");

			if (strcmp(node_name, NODEREF_ELEMENT) == 0)
			{
				osmut::attribute_range noderef_attrs = osm.attributes();
				w.node_ids.push_back(atoi((*noderef_attrs).second));

				assert(strcmp((*noderef_attrs).first, "ref") == 0
					&& "unexpected attribute order in node-ref element ('ref' expected)");
			}
			else if (strcmp(node_name, TAG_ELEMENT) == 0)
			{
				if (!w.tags)
					w.tags = shared_ptr<tagmap>(new tagmap);
				process_tag_element(osm, *w.tags);				
			}

		}
		else if (node_type == XML_READER_TYPE_END_ELEMENT)
			break;
	}

//	assert(w.tags && "'way' node without tag");

	assert(!w.node_ids.empty() && "empty 'way' element (some node ids expected)");
}

void process_tag_element(osmut::xml_reader & osm, tagmap & tags)
{
	assert(osm.attribute_count() == 2 &&
		"unexpected number of 'tag' arguments (two 'k' and 'v' expected)");

	osmut::attribute_range attrs = osm.attributes();

	string key = (*attrs).second;
	assert(strcmp((*attrs).first, "k") == 0 &&
		"unexpected 'tag' attribute ('k' expected)");
	++attrs;

	tags[key] = (*attrs).second;

	assert(strcmp((*attrs).first, "v") == 0 &&
		"unexpected 'tag' attribute ('v' expected)");
}

