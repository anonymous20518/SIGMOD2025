/**
 * Implementation of Algorithm 2 from Submission 840.
 * Discovers all k-cores of a fixed size g in an input graph G.
 */

#pragma once

#include "graph.hpp"

using group_t = index_list_t;                         // A group of points, identified by the index of each point
using group_list_t = std::vector< index_list_t >;     // A group of groups


namespace base {
namespace listing {
namespace cousins_first {

/**
 * Given a set of nodes that define a subgraph to induce on a given adjacency list, returns an ordered list of
 * all k-cores of a specific size, predicated on containing the first node. That is to say, it returns all subsets
 * S of (nodes.begin+1 .... nodes.end) of size groupSize - 1 for which (S union {nodes[0]}) induces a subgraph on
 * edges in which the minimum degree is coreSize.
 */
auto ListKCoresWithPrefix( vertex_list_t const& nodes, adjacency_list_t const& edges, int groupSize, int coreSize ) -> group_list_t;

/**
 * Returns an ordered list of **all** subsets of a given graph that have groupSize vertices and a minimum degree
 * of coreSize, i.e., all size-g k-cores.
 */
auto ListAllKCores( adjacency_list_t edges, int groupSize, int coreSize ) -> group_list_t;

} // namespace cousins_first
} // namespace listing
} // namespace base
