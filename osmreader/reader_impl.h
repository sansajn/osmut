#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include "xml_reader.h"


/* \note Ak node a wey majú krátkodobý charakter, nie je lepšie mapy 
vytvárať na zásobníku ? 

\note počet tagou je v rádoch jednotiek mapa má výhodu iba z pohľadu 
programátora (jednoduché hľadanie).

\note Jedinou výhodou vytvorenia mapy na halde je jej jednoduché 
kopírovanie, ktoré sa však v nadradenej abstrakcii nepredpokladá. 
Nadradená aplikácia pravdepodobne zvolý inú formu reprezentácie 
tagu. */

typedef std::map<std::string, std::string> tagmap;

struct node 
{
	int id;
	int lat;  //!< latitude multiplied by 1e7
	int lon;
	std::shared_ptr<tagmap> tags;
};

/* \note každí way by mal mať tag (s tochto pohľadu nemá shared_ptr
opodstatnenie) na druuhú stranu veľké množstvo way bude mať rovnaké 
tagi (je ich teda možné zdieať). 

\note životnosť way je ale mienená iba po kým sa v slučke nespracuje a 
neprevedie na konkrétnu štruktúru (zohladnujúcu potreby aplikácie), preto 
optimalizácie nie sú na mieste. */
struct way
{
	int id;
	std::vector<int> node_ids;
	std::shared_ptr<tagmap> tags;
};

struct node_reader
{
	typedef node value_type;

	//! Vráti true ak je reader schopný tag prečítať.
	static bool tag(std::string const & node_name);

	//! Vráti true ak boli už spracovane všetky podporovane tagy.
	static bool stop_tag(std::string const & node_name);

	//! Prečíta tag.
	static void read_tag(osmut::xml_reader & osm, node & n);
};

struct way_reader
{
	typedef way value_type;

	static bool tag(std::string const & node_name);
	static bool stop_tag(std::string const & node_name);
	static void read_tag(osmut::xml_reader & osm, way & w);
};

