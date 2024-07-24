#pragma once

#include "graph.hpp"
#include "spatial.hpp"

auto GetAverageVirtualPoint( vertex_list_t const& group, label_list_t const& labels ) -> vertex_label_t;
auto GetBestVirtualPoint   ( vertex_list_t const& group, label_list_t const& labels ) -> vertex_label_t;
auto GetWorstVirtualPoint  ( vertex_list_t const& group, label_list_t const& labels ) -> vertex_label_t;
