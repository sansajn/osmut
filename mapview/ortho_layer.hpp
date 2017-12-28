#pragma once
#include "mapview_layer.hpp"

class ortho_layer : public mapview_layer
{
public:
	void resize(int w, int h) override;

protected:
	int _w, _h;
};
