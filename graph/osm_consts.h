// osm file format constants
#pragma once

#define OSM_ELEMENT "osm"
#define NODE_ELEMENT "node"
#define WAY_ELEMENT "way"
#define RELATION_ELEMENT "relation"
#define TAG_ELEMENT "tag"
#define NODEREF_ELEMENT "nd"
#define RELATION_MEMBER "member"


enum e_highway_values {
	e_motorway,
	e_motorway_link,
	e_trunk,
	e_trunk_link,
	e_primary,
	e_primary_link,
	e_secondary,
	e_secondary_link,
	e_tertiary,
	e_tertiary_link,
	e_living_street,
	e_pedestrian,
	e_residential,
	e_unclassified,
	e_service,
	e_track,
	e_bus_guideway,
	e_raceway,
	e_road,
	e_path,
	e_footway,
	e_cycleway,
	e_bridleway,
	e_steps,
	e_proposed,
	e_construction,
	e_unknown
};

