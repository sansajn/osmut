#pragma once
#include "tile.hpp"

class local_tile : public tile
{
public:
	local_tile(fs::path const & fname)
		: _p{fname}
	{}

	fs::path path() override {return _p;}

private:
	fs::path _p;
};
