#pragma once

#include "graph.hpp"

namespace base
{
    namespace zhang
    {
        void GetSkylineCommunities(Graph& myGraph, int coreSize, int groupSize, group_list_t& skylineCommunities);
    }
    namespace zhangBaseline
    {
        void GetSkylineCommunities(Graph& myGraph, int coreSize, int groupSize, group_list_t & skylineCommunities);
    }
}
