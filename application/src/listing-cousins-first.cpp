#include <iostream>
#include <ranges>

#include "graph-structural-operations.hpp"

#include "listing-cousins-first.hpp"

namespace { // anonymous

/**
 * Wrapper struct for passing information about groups between recursive calls.
 */
struct GroupInfo
{
    group_t group = {};
    std::vector< int > missedConnectionsCount = {};
    vertex_list_t tailset = {};

    using TailsetIterator = decltype( std::cbegin( tailset ) );
};

/**
 * Factory function to produce a new GroupInfo struct
 * for the next round of recursion
 */
auto GetNewGroupInfo( group_t const& original_group
                    , std::vector< int > missedConnectionsCount
                    , GroupInfo::TailsetIterator start
                    , GroupInfo::TailsetIterator end
                    , vertex_list_t const& neighbors
                    , adjacency_list_t const& edges
                    , int newNode
                    , int maxMissedConnections
                    , int groupSize ) -> GroupInfo
{
    GroupInfo newGInfo{ original_group, missedConnectionsCount, vertex_list_t( start, end ) };
    size_t const original_group_size = original_group.size();
    size_t neighborsIndex = 0;

    for( size_t const groupIndex : std::views::iota( 0lu, original_group_size ) )
    {
        if (neighborsIndex < neighbors.size() && original_group[groupIndex] == neighbors[neighborsIndex])
        {
            neighborsIndex++;
        }
        else // there is a node in group that is not in neighbors (note that neighbors is a subset of group)
        {
            if ( ++( newGInfo.missedConnectionsCount[groupIndex] ) == maxMissedConnections )
            {
                newGInfo.tailset = GetIntersection( std::crbegin( edges[ newGInfo.group[ groupIndex ] ] )
                                                  , std::crend  ( edges[ newGInfo.group[ groupIndex ] ] )
                                                  , std::cbegin ( newGInfo.tailset )
                                                  , std::cend   ( newGInfo.tailset ) );
                if (newGInfo.tailset.size() + original_group_size + 1 < static_cast< size_t >(groupSize)) // there is not enough nodes in tailset anymore
                {
                    newGInfo.tailset.clear();
                    return newGInfo;
                }
            }
        }
    }

    newGInfo.group.push_back(newNode);
    newGInfo.missedConnectionsCount.push_back(original_group_size - neighbors.size());
    if (original_group_size - neighbors.size() == static_cast< size_t >(maxMissedConnections))
    {
        newGInfo.tailset = GetIntersection( std::crbegin( edges[ newNode ] )
                                          , std::crend  ( edges[ newNode ] )
                                          , std::cbegin ( newGInfo.tailset )
                                          , std::cend   ( newGInfo.tailset ) );
    }
    return newGInfo;
}



void ListKCoresRecursive( GroupInfo gInfo, adjacency_list_t const& edges, group_list_t & candidates, int groupSize, vertex_degree_t coreSize )
{
    vertex_degree_t const maxMissedConnections = groupSize - coreSize - 1;

    for( auto it_newNode = std::cbegin( gInfo.tailset ); it_newNode != std::cend( gInfo.tailset ); ++it_newNode )
    {
        vertex_id_t   const newNode = *it_newNode;
        vertex_list_t const neighbours = GetIntersection( std::crbegin( edges[ newNode ] )
                                                        , std::crend  ( edges[ newNode ] )
                                                        , std::cbegin ( gInfo.group )
                                                        , std::cend   ( gInfo.group ) );

        vertex_degree_t const num_nodes_not_in_intersection = gInfo.group.size() - neighbours.size();
        if( num_nodes_not_in_intersection <= maxMissedConnections )
        {
            if( gInfo.group.size() == static_cast< size_t >( groupSize - 1 ) )
            {
                // We have a group!! Push it on, copy it to the solution set, and pop it off
                // so that we can reuse the group.
                gInfo.group.push_back( newNode );
                candidates.push_back( gInfo.group );
                gInfo.group.pop_back();
            }
            else // there is more than one remaining slot to fill
            {
                // Recurse
                ListKCoresRecursive( GetNewGroupInfo( gInfo.group
                                                    , gInfo.missedConnectionsCount
                                                    , it_newNode + 1
                                                    , std::cend( gInfo.tailset )
                                                    , neighbours
                                                    , edges
                                                    , newNode
                                                    , maxMissedConnections
                                                    , groupSize )
                                   , edges
                                   , candidates
                                   , groupSize
                                   , coreSize );
            }
        }
        // else cannot form any valid groups with these neighbours
    }
}

} // namespace anonymous


namespace base {
namespace listing {
namespace cousins_first {

auto ListKCoresWithPrefix( vertex_list_t const& nodes, adjacency_list_t const& edges, int groupSize, int coreSize ) -> group_list_t
{
    group_list_t kcores;

    // Launch recursion
    if( static_cast< vertex_degree_t >( nodes.size() ) >= groupSize )
    {
        const vertex_id_t prefix_node = nodes[0];
        vertex_list_t tailset;

        if (groupSize == coreSize + 1)
        {
            // If we're looking for cliques, use only the neighbours of the
            // prefix node as the tailset, since all nodes should be connected.
            tailset = vertex_list_t( edges[prefix_node].crbegin(), edges[prefix_node].crend() );
        }
        else
        {
            tailset = vertex_list_t( nodes.cbegin() + 1, nodes.cend() );
        }

	    ListKCoresRecursive( GroupInfo{ { prefix_node }, { 0 }, tailset }
	                       , edges
	                       , kcores
	                       , groupSize
	                       , coreSize );
	}

    return kcores;
}


auto ListAllKCores( adjacency_list_t edges, vertex_degree_t groupSize, vertex_degree_t coreSize ) -> group_list_t
{
    group_list_t kcores;

    std::size_t const num_vertices = edges.size();

    // Initialise variables for maintaining maximum k-cores
    auto [ sortedIndices, nodePosition, binBoundaries ] =  BinSortByDegree( edges );
    vertex_list_t in_max_kcore( num_vertices, 1 ); // should be bools. update later throughout code base.

    // Compute the maximum kcore of the original graph.
    ShrinkToMaxKCoreVertices( coreSize, std::nullopt, edges, sortedIndices, nodePosition, binBoundaries, in_max_kcore );

    for( vertex_id_t vertex : std::views::iota( 0lu, num_vertices - groupSize ) )
    {
        if (in_max_kcore[vertex] == 1)
        {
            vertex_list_t const nodes = GetKHopNeighbourhood( vertex
                                                            , edges
                                                            , in_max_kcore
                                                            , 2u ); // max diameter of two per Conte et al. (KDD 2018)

            if( static_cast< vertex_degree_t >( nodes.size() ) >= groupSize )
            {
                group_list_t const kcores_with_new_prefix = ListKCoresWithPrefix( nodes
                                                                                , edges
                                                                                , groupSize
                                                                                , coreSize );

                kcores.reserve( kcores.size() + kcores_with_new_prefix.size() );
                kcores.insert( std::end   ( kcores )
                             , std::cbegin( kcores_with_new_prefix )
                             , std::cend  ( kcores_with_new_prefix ) );
            }

            // Update graph by peeling off vertex and recomputing maximum k-core
            ShrinkToMaxKCoreVertices( coreSize, vertex, edges, sortedIndices, nodePosition, binBoundaries, in_max_kcore );
        }
        RemoveVertex( edges, vertex );
    }

	return kcores;
}

} // namespace cousins_first
} // namespace listing
} // namespace base
