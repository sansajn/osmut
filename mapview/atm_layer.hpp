#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "fs.hpp"
#include "geo_point_layer.hpp"

class atm_layer : public geo_point_layer
{
public:
	struct atm_desc
	{
		size_t loc_idx;
		std::string name;
	};

	atm_layer(fs::path const & atm_osm);

private:

	std::vector<atm_desc> _atms;
};
