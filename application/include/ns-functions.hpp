#pragma once

#include <string>

#include "graph.hpp"

namespace base
{   
    namespace sequential
    {
        auto GetSkylineCommunities( Graph & myGraph, int coreSize, int groupSize ) -> group_list_t;

        auto GenerateSummerPlot( Graph & myGraph, int coreSize) -> std::vector<size_t>;
        void to_file( const std::vector<size_t>& vector, const std::string filePath);
    }
    namespace parallel
    {
        auto GetSkylineCommunities( Graph & myGraph, int coreSize, int groupSize, int nThreads ) -> group_list_t;
    }
}
