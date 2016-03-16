#pragma once

template <class Range, class Filter>
class filter_range
{
public:
	typedef typename Range::pointer pointer;
	typedef typename Range::reference reference;
	typedef typename Range::const_reference const_reference;

	filter_range(Range range, Filter filter)
		: _curr(range), _filter(filter)
	{
		if (_curr && !_filter(*_curr))
			next();
	}

	reference operator*() {return *_curr;}
	const_reference operator*() const {return *_curr;}
	pointer operator->() const {return _curr.operator->();}
	void operator++() {	next();}
	explicit operator bool() const {return bool(_curr);}
	bool operator==(Range const & rhs) const {return rhs == _curr;}
	bool operator!=(Range const & rhs) const {return !(*this == rhs);}

	void next()
	{
		do
		{
			++_curr;
		}
		while (_curr && !_filter(*_curr));
	}

private:
	Range _curr;
	Filter _filter;
};


template <class Range, class Filter>
filter_range<Range, Filter> filtered_range(Range r, Filter f)
{
	return filter_range<Range, Filter>(r, f);
}
