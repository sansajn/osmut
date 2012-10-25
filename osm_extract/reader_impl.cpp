#include <cstdlib>
#include <cstring>
#include <cassert>
#include "reader_impl.h"
#include "osmff.h"

using std::map;
using std::shared_ptr;
using std::string;
using namespace irr::io;

void process_node_tag(IrrXMLReader & xml, node & n);
void process_way_tag(IrrXMLReader & xml, way & w);
void process_tag_tag(IrrXMLReader & xml, tagmap & tags);


bool node_reader::tag(char const * node_name)
{
	return strcmp(NODE_TAG, node_name) == 0;
}

bool node_reader::stop_tag(char const * node_name)
{
	return strcmp(WAY_TAG, node_name) == 0 ||	
		strcmp(RELATION_TAG, node_name) == 0;
}

void node_reader::read_tag(irr::io::IrrXMLReader & xml, node & n)
{
	n.tags.reset();
	process_node_tag(xml, n);
}

bool way_reader::tag(char const * node_name) 
{
	return strcmp(WAY_TAG, node_name) == 0;
}

bool way_reader::stop_tag(char const * node_name)
{
	return strcmp(RELATION_TAG, node_name) == 0;
}

void way_reader::read_tag(irr::io::IrrXMLReader & xml, way & w)
{
	w.tags.reset();
	w.node_ids.clear();
	process_way_tag(xml, w);
}

void process_node_tag(IrrXMLReader & xml, node & n)
{
	int n_attribs = xml.getAttributeCount();

	assert(n_attribs > 2 
		&& "wrong node format (at least 'id', 'lat' and 'lon' "
			"attributes expected)");

	assert(strcmp("id", xml.getAttributeName(0)) == 0 
		&& "unexpected attribute order");

	assert(strcmp("lat", xml.getAttributeName(1)) == 0 
		&& "unexpected attribute order");

	assert(strcmp("lon", xml.getAttributeName(2)) == 0 
		&& "unexpected attribute order");

	n.id = atoi(xml.getAttributeValue(0));
	n.lat = atof(xml.getAttributeValue(1));
	n.lon = atof(xml.getAttributeValue(2));

	if (!xml.isEmptyElement())
	{
		while (xml.read())
		{
			EXML_NODE type = xml.getNodeType();

			if (type == EXN_ELEMENT)
			{
				char const * node_name = xml.getNodeName();

				if (strcmp(TAG_TAG, node_name) == 0)
				{
					if (!n.tags)
						n.tags = shared_ptr<tagmap>(new tagmap);

					process_tag_tag(xml, *n.tags);
				}

				assert(strcmp("tag", xml.getNodeName()) == 0 &&
					"unexpected node inside 'node' (only 'tag' allowed)");
			}
			else if (type == EXN_ELEMENT_END)
				break;
		}
	}
}

void process_way_tag(IrrXMLReader & xml, way & w)
{
	assert(strcmp("id", xml.getAttributeName(0)) == 0 
		&&	"unexpected attribute order ('id' expected)");

	w.id = atoi(xml.getAttributeValue(0));

	while (xml.read())
	{
		EXML_NODE type = xml.getNodeType();

		if (type == EXN_ELEMENT)
		{
			char const * node_name = xml.getNodeName();
			assert(
				strcmp("nd", node_name) == 0 ||
				strcmp("tag", node_name) == 0 &&
				"unexpected node inside 'way' ('nd' or 'tag' allowed)");

			if (strcmp(NODEREF_TAG, node_name) == 0)
			{
				assert(strcmp("ref", xml.getAttributeName(0)) == 0
					&& "unexpected attribute order ('ref' expected)");

				w.node_ids.push_back(atoi(xml.getAttributeValue(0)));
			}
			else if (strcmp(TAG_TAG, node_name) == 0)
			{
				if (!w.tags)
					w.tags = shared_ptr<tagmap>(new tagmap);
				process_tag_tag(xml, *w.tags);
			}

		}
		else if (type == EXN_ELEMENT_END)
			break;
	}

//	assert(w.tags && "'way' node without tag");

	assert(!w.node_ids.empty() && "empty 'way' node");
}

void process_tag_tag(IrrXMLReader & xml, tagmap & tags)
{
	assert(xml.getAttributeCount() == 2 && 
		"unexpected number of 'tag' arguments (two expected)");

	assert(strcmp("k", xml.getAttributeName(0)) == 0 &&
		"unexpected 'tag' attribute ('k' expected)");

	assert(strcmp("v", xml.getAttributeName(1)) == 0 &&
		"unexpected 'tag' attribute ('v' expected)");

	tags[xml.getAttributeValue(0)] = xml.getAttributeValue(1);
}

