#pragma once
#include <utility>
#include <libxml/xmlreader.h>


namespace osmut
{

//! Rozsah iterujúci atributy xml uzlu.
class attribute_range
{
public:
	attribute_range(xmlTextReader * reader) : _reader(reader)
	{
		operator++();
	}

	std::pair<char const *, char const *> operator*() {
		return std::make_pair(
			reinterpret_cast<char const *>(xmlTextReaderConstName(_reader)),
			reinterpret_cast<char const *>(xmlTextReaderConstValue(_reader)));
	}
				
	void operator++() {_valid = xmlTextReaderMoveToNextAttribute(_reader) == 1;}
	explicit operator bool() const {return _valid;}

private:
	bool _valid;
	xmlTextReader * _reader;
};


//! Umožnuje prechádzať xml dokumentom.
class xml_reader
{
public:
	xml_reader(char const * filename)
		: _reader(xmlReaderForFile(filename, NULL, 0))
	{
		assert(_reader && "can't create xml reader");
	}

	~xml_reader() {xmlFreeTextReader(_reader);}

	bool read() {return xmlTextReaderRead(_reader) == 1;}

	char const * node_name() 
	{
		return reinterpret_cast<char const *>(xmlTextReaderConstName(_reader));
	}

	//! \saa http://www.xmlsoft.org/html/libxml-xmlreader.html#xmlReaderTypes
	int node_type() {return xmlTextReaderNodeType(_reader);}

	int attribute_count() {return xmlTextReaderAttributeCount(_reader);}
	attribute_range attributes() {return attribute_range(_reader);}

	bool empty_element() {return xmlTextReaderIsEmptyElement(_reader) == 1;}

private:
	xmlTextReader * _reader;
};

}  // osmut
