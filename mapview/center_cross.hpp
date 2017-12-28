#pragma once

#include "ortho_layer.hpp"

class center_cross : public ortho_layer
{
public:
	void draw(Cairo::RefPtr<Cairo::Context> const & cr) override;
};
