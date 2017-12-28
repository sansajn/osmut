#include "ortho_layer.hpp"

void ortho_layer::resize(int w, int h)
{
	mapview_layer::resize(w, h);

	_w = w;
	_h = h;
}
