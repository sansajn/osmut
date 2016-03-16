#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>
#include "osm_consts.h"
#include "reader_impl.h"

using std::map;
using std::unordered_map;
using std::shared_ptr;
using std::string;

static void process_node_element(osmut::xml_reader & osm, node & n);
static void process_way_element(osmut::xml_reader & osm, way & w);
static void process_relation_element(osmut::xml_reader & osm, relation & r);
static void process_tag_element(osmut::xml_reader & osm, tagmap & tags);

static int to_signed_coordinate(char const * float_coordinte);

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

bool relation_reader::tag(std::string const & node_name)
{
	return node_name == RELATION_ELEMENT;
}

bool relation_reader::stop_tag(std::string const & node_name)
{
	return false;  // there is nothing after relation section
}

void relation_reader::read_tag(osmut::xml_reader & osm, relation & r)
{
	r.tags.reset();
	r.members.clear();
	process_relation_element(osm, r);
}


void process_node_element(osmut::xml_reader & osm, node & n)
{
	int n_attribs = osm.attribute_count();

	assert(n_attribs > 2 
		&& "wrong node format (at least 'id', 'lat' and 'lon' "
			"attributes expected)");

	bool empty_element = osm.empty_element();  // must be read before attributes iteration

	unordered_map<string, string> attr_map(n_attribs);
	for (osmut::attribute_range attrs = osm.attributes(); attrs; ++attrs)
	{
		auto keyval = *attrs;
		attr_map[keyval.first] = keyval.second;
	}

	// we only care about id, lat and lon attributes
	auto it = attr_map.find("id");
	assert(it != attr_map.end() && "node's 'id' attribute missing");
	n.id = atoi(it->second.c_str());

	it = attr_map.find("lat");
	assert(it != attr_map.end() && "node's 'lat' attribute missing");
	n.lat = to_signed_coordinate(it->second.c_str());

	it = attr_map.find("lon");
	assert(it != attr_map.end() && "node's 'lon' attribute missing");
	n.lon = to_signed_coordinate(it->second.c_str());

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

			assert((strcmp(node_name, "nd") == 0 || strcmp(node_name, "tag") == 0)
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

void process_relation_element(osmut::xml_reader & osm, relation & r)
{
	assert(!osm.empty_element() && "empty 'relation' element");

	osmut::attribute_range attrs = osm.attributes();

	assert(strcmp((*attrs).first, "id") == 0
		&& "unexpected attribute order ('id' expected)");

	r.id = atoi((*attrs).second);

	while (osm.read())  // read relation data
	{
		int node_type = osm.node_type();

		if (node_type == XML_READER_TYPE_ELEMENT)
		{
			char const * node_name = osm.node_name();

			assert((strcmp(node_name, 	RELATION_MEMBER) == 0 || strcmp(node_name, TAG_ELEMENT) == 0)
				&& "unexpected node inside 'relation' element ('member' or 'tag' allowed)");

			if (strcmp(node_name, RELATION_MEMBER) == 0)
			{
				// type -> ref -> role
				osmut::attribute_range member_attrs = osm.attributes();

				assert(strcmp((*member_attrs).first, "type") == 0
					&& "type attribute expected");

				member m;

				char const * member_type = (*member_attrs).second;
				if (strcmp(member_type, "node") == 0)
					m.type = member::node_type;
				else if (strcmp(member_type, "way") == 0)
					m.type = member::way_type;
				else if (strcmp(member_type, "relation") == 0)
					m.type = member::relation_type;
				else
					assert(true && "unexpected member type ('node', 'way' or 'relation' allowed)");

				++member_attrs;

				assert(strcmp((*member_attrs).first, "ref") == 0
					&& "ref attribute expected");

				m.ref = atoi((*member_attrs).second);
				++member_attrs;

				assert(strcmp((*member_attrs).first, "role") == 0
					&& "role attribute expected");

				m.role = string((*member_attrs).second);

				r.members.push_back(m);
			}
			else if (strcmp(node_name, TAG_ELEMENT) == 0)
			{
				if (!r.tags)
					r.tags = shared_ptr<tagmap>(new tagmap);
				process_tag_element(osm, *r.tags);
			}
		}  // if (node_type
		else if (node_type == XML_READER_TYPE_END_ELEMENT)
			break;
	}  // while
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

int to_signed_coordinate(char const * float_coordinate)
{
	char const * beg = float_coordinate;
	if (*beg == '-' || *beg == '+')
		beg += 1;

	char const * end = float_coordinate + strlen(float_coordinate);

	char const * dot_it = strchr(float_coordinate, '.');
	if (!dot_it)
		dot_it = end;

	assert((dot_it-beg) < 4
		&& "konvertovane cislo je v nespravnom formate");

	// cela časť
	int a = 0;
	int exp = 7;
	char const * dig = dot_it-1;
	while (dig >= beg)
		a += (*(dig--) - '0') * pow(10, exp++);

	// desatina časť
	int b = 0;
	exp = 6;
	dig = dot_it+1;
	while (dig < end)
		b += (*(dig++) - '0') * pow(10, exp--);

	int val = (float_coordinate[0] == '-' ? -1 : 1) * (a+b);

	return val;
}
