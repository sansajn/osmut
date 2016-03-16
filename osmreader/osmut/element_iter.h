/*! element_iter, zaklad pre implementaciu node/way/relation iteratoru */
#pragma once
#include <memory>
#include <cstring>
#include <cassert>
#include "osm_consts.h"
#include "xml_reader.h"


template <class ReaderPolicy>
class element_iterator
{
public:
	typedef typename ReaderPolicy::value_type value_type;
	typedef std::shared_ptr<value_type> pointer;
	typedef value_type & reference;
	typedef value_type const & const_reference;

	element_iterator() : _osm(nullptr), _buf(nullptr) {}
	element_iterator(osmut::xml_reader & xml);
	element_iterator & operator++() {next(); return *this;}
	reference operator*() {return *_buf;}
	const_reference & operator*() const {return *_buf;}
	pointer operator->() const {return _buf;}
	bool operator==(element_iterator const & rhs) const;

private:
	typedef ReaderPolicy __RP;

	void next();
	bool process_node();

	osmut::xml_reader * _osm;
	std::shared_ptr<value_type> _buf;
};

template <class ReaderPolicy>
bool operator!=(element_iterator<ReaderPolicy> const & lhs,
	element_iterator<ReaderPolicy> const & rhs)
{
	return !(lhs == rhs);
}

template <class ReaderPolicy>
element_iterator<ReaderPolicy>::element_iterator(osmut::xml_reader & osm)
	: _osm(&osm), _buf(new value_type)
{
	if (_osm->node_type() == XML_READER_TYPE_ELEMENT && __RP::tag(_osm->node_name()))
		__RP::read_tag(*_osm, *_buf);
	else
		next();
}

template <class ReaderPolicy>
void element_iterator<ReaderPolicy>::next()
{
	if (_buf && _osm)
	{
		bool res;
		do
		{
			res = false;
			if (!_osm->read())
				_buf = nullptr;
			else
				res = !process_node();
		}
		while (res);
	}
}

template <class ReaderPolicy>
bool element_iterator<ReaderPolicy>::process_node()
{
	char const * node_name = _osm->node_name();

	switch (_osm->node_type())
	{
		case XML_READER_TYPE_ELEMENT:
		{
			if (__RP::tag(node_name))
				__RP::read_tag(*_osm, *_buf);
			else if (__RP::stop_tag(node_name))
				_buf = nullptr;  // end
			else
				return false;

			return true;  // everithing ok
		}

		case XML_READER_TYPE_END_ELEMENT:
		{
			if (strcmp(node_name, OSM_ELEMENT) == 0)  // </osm>
				_buf = nullptr;
			else
				return false;

			return true;
		}
	}
	return false;
}

template <class ReaderPolicy>
bool element_iterator<ReaderPolicy>::operator==(element_iterator const & rhs) const
{
	return _buf == rhs._buf;
}
