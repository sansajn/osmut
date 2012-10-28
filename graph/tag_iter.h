#pragma once
#include <cstring>
#include <cassert>
#include <irrXML.h>
#include "osmff.h"

template <class ReaderPolicy>
class tag_iterator
{
public:
	typedef typename ReaderPolicy::value_type value_type;

	tag_iterator() : _xml(NULL), _buf(NULL) {}
	tag_iterator(irr::io::IrrXMLReader * xml, value_type * buf);
	tag_iterator & operator++() {next(); return *this;}
	value_type & operator*() {return *_buf;}
	value_type const & operator*() const {return *_buf;}
	value_type * operator->() const {return _buf;}
	bool operator==(tag_iterator const & rhs) const;

	void next();

private:
	bool process_node();

private:
	irr::io::IrrXMLReader * _xml;
	value_type * _buf;
};

template <class ReaderPolicy>
bool operator!=(tag_iterator<ReaderPolicy> const & lhs, 
	tag_iterator<ReaderPolicy> const & rhs)
{
	return !(lhs == rhs);
}

template <class ReaderPolicy>
tag_iterator<ReaderPolicy>::tag_iterator(irr::io::IrrXMLReader * xml, 
	value_type * buf)	: _xml(xml), _buf(buf)
{
	irr::io::EXML_NODE node_type = _xml->getNodeType();

	if (node_type == irr::io::EXN_ELEMENT && 
		ReaderPolicy::tag(_xml->getNodeName()))
	{
		ReaderPolicy::read_tag(*_xml, *buf);
	}
	else
		next();
}

template <class ReaderPolicy>
void tag_iterator<ReaderPolicy>::next()
{
	while (_buf && _xml && _xml->read() && !process_node())
		continue;
}

template <class ReaderPolicy>
bool tag_iterator<ReaderPolicy>::process_node()
{
	char const * node_name = _xml->getNodeName();

	switch (_xml->getNodeType())
	{
		case irr::io::EXN_ELEMENT:
		{
			if (ReaderPolicy::tag(node_name))
				ReaderPolicy::read_tag(*_xml, *_buf);
			else if (ReaderPolicy::stop_tag(node_name))  
				_buf = NULL;  // end
			else
				return false;

			return true;
		}

		case irr::io::EXN_ELEMENT_END:
		{
			if (strcmp(OSM_TAG, node_name) == 0)
				_buf = NULL;
			else
				return false;

			return true;
		}

		case irr::io::EXN_TEXT:
		case irr::io::EXN_COMMENT:
		case irr::io::EXN_UNKNOWN:
			return false;

		default:
			assert(false && "unexpected element type inside 'osm'");
	}
	return false;
}

template <class ReaderPolicy>
bool tag_iterator<ReaderPolicy>::operator==(tag_iterator const & rhs) const
{
	return _buf == rhs._buf;
}

