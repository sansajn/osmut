#pragma once
#include "element_iter.h"
#include "reader_impl.h"

typedef element_iterator<way_reader> way_iterator;
typedef element_iterator<node_reader> node_iterator;
typedef element_iterator<relation_reader> relation_iterator;
