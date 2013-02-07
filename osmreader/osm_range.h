#pragma once
#include "osm_iter.h"
#include "xml_reader.h"

template <class Iterator>
class osm_range
{
public:
	typedef typename Iterator::pointer pointer;
	typedef typename Iterator::reference reference;
	typedef typename Iterator::const_reference const_reference;

	osm_range(Iterator first, Iterator last)
		: _curr(first), _end(last)
	{}

	reference operator*() {return *_curr;}
	const_reference operator*() const {return *_curr;}
	pointer operator->() const {return _curr.operator->();}

	void operator++() 
	{
		assert(_curr != _end && "increments not valid range"); 
		++_curr;	
	}

	operator bool() const {return _curr != _end;}

	bool operator==(osm_range const & rhs) const 
	{
		return _curr == rhs._curr && _end == rhs._end;
	}

	bool operator!=(osm_range const & rhs) const {return !(*this == rhs);}

private:
	Iterator _curr, _end;
};


typedef osm_range<way_iterator> way_range;
typedef osm_range<node_iterator> node_range;


inline way_range make_way_range(osmut::xml_reader & osm)
{
	return way_range(way_iterator(osm), way_iterator());
}

inline node_range make_node_range(osmut::xml_reader & osm)
{
	return node_range(node_iterator(osm), node_iterator());
}
