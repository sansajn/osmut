#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <irrXML.h>


/* 
Ak node a wey majú krátkodobý charakter, nie je lepšie mapy 
vytvárať na zásobníku ? 

Jedinou výhodou vytvorenia mapy na halde je jej jednoduché kopírovanie, 
ktoré sa však v nadradenej abstrakcii nepredpokladá. 
*/

typedef std::map<std::string, std::string> tagmap;

struct node 
{
	int id;
	float lat;
	float lon;
	std::shared_ptr<tagmap> tags;
};

struct way
{
	int id;
	std::vector<int> node_ids;
	std::shared_ptr<tagmap> tags;
};

struct node_reader
{
	typedef node value_type;

	static bool tag(char const * node_name);
	static bool stop_tag(char const * node_name);
	static void read_tag(irr::io::IrrXMLReader & xml, node & n);
};

struct way_reader
{
	typedef way value_type;

	static bool tag(char const * node_name);
	static bool stop_tag(char const * node_name);
	static void read_tag(irr::io::IrrXMLReader & xml, way & w);
};

