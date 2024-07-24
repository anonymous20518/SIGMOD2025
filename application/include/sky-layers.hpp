#pragma once

#include "spatial.hpp"

using SkyLayers = std::vector< PointList >;

auto GenerateSkyLayers( PointList const& points ) -> SkyLayers;
