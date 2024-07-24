#pragma once

#include <optional>

#include "graph.hpp"


template < typename ForwardIterator1, typename ForwardIterator2 >
auto GetIntersection( ForwardIterator1 group1_start
				    , ForwardIterator1 group1_end
				    , ForwardIterator2 group2_start
				    , ForwardIterator2 group2_end ) -> vertex_list_t
{
    // assuming both vectors are sorted
    vertex_list_t result;
    result.reserve( std::min( std::distance( group1_start, group1_end ), std::distance( group2_start, group2_end ) ) );
    std::set_intersection( group1_start, group1_end, group2_start, group2_end, std::back_inserter( result ) );
    return result;
}


inline
void GetIntersection( vertex_list_t const& group1, vertex_list_t const& group2, vertex_list_t & result )
{
    // assuming both vectors are sorted
    result.clear();
    result.reserve(std::min(group1.size(), group2.size()));
    std::set_intersection(group1.cbegin(), group1.cend(), group2.cbegin(), group2.cend(), std::back_inserter(result));
}

inline
auto GetIntersection( vertex_list_t const& group1, vertex_list_t const& group2 ) -> vertex_list_t
{
	return GetIntersection( std::cbegin( group1 ), std::cend( group1 ), std::cbegin( group2 ), std::cend( group2 ) );
}

/**
 * Uses classic "peeling" algorithm to determine the coreness of each vertex
 * Naive implementation that does not use bin sort because this is not a hot
 * spot in the code.
 */
auto GetCoreNumbers( adjacency_list_t const& adjacency_vectors ) -> vertex_degree_list_t;

/**
 * Optionally removes a vertex and then performs peeling to determine which vertices
 * are no longer part of the maximum k-core and sets them to false in active_vertices.
 * Correspondingly updates the bin sort variables: sortedIndices, nodePosition, and binBoundaries.
 * 
 * @returns a pair indicating the total number of vertices and the total number of edges removed
 */
auto ShrinkToMaxKCoreVertices( int k
	                         , std::optional< vertex_id_t > initial_vertex_to_remove
	                         , adjacency_list_t const& edges
	                         , vertex_list_t& sortedIndices
	                         , vertex_list_t& nodePosition
	                         , vertex_list_t& binBoundaries
	                         , vertex_list_t& active_vertices ) -> std::pair<int,int>;

/**
 * Retrieves the subset of vertices that are within k hops of u, restricted to those
 * that are in an "active set." If num_hops is not given, returns the entire connected
 * component.
 * O(|E|) execution time and O(|V|) space.
 */
auto GetKHopNeighbourhood( vertex_id_t u
                         , adjacency_list_t const& edges
                         , vertex_list_t const& active_vertices
                         , std::optional< vertex_degree_t > num_hops = std::nullopt ) -> vertex_list_t;

/**
 * Retrieves the subset of vertices that are in the connected component of u, restricted to those
 * that are in an "active set."
 * O(|E|) execution time and O(|V|) space.
 */
inline
auto GetConnectedComponent( vertex_id_t u, adjacency_list_t const& edges, vertex_list_t const& active_vertices ) -> vertex_list_t
{
	return GetKHopNeighbourhood( u, edges, active_vertices );
}

/**
 * Checks whether the graph induced on vertex_subset from the given edges is "connected",
 * i.e., every vertex is reachable from every other vertex
 */
bool IsConnected( vertex_list_t const& vertex_subset, adjacency_list_t const& edges );

/**
 * Checks whether the graph induced on vertex_subset from the given edges has a minimum degree of k
 */
bool IsKCore( vertex_list_t const& vertex_subset, adjacency_list_t const& edges, std::size_t k );

/**
 * Checks whether the graph induced on vertex_subset from the given edges has a minimum degree of k
 * and is connected, i.e., all vertices are reachable from each other
 */
template < class Func >
bool IsConnectedKCore( vertex_list_t const& vertex_subset, adjacency_list_t const& edges, std::size_t k, Func Intersect )
{
    for( vertex_id_t const nodeID : vertex_subset)
    {
        vertex_list_t const neighbours = Intersect( std::crbegin( edges[ nodeID ] )
                                                  , std::crend  ( edges[ nodeID ] )
                                                  , std::cbegin ( vertex_subset )
                                                  , std::cend   ( vertex_subset ) );
        // if there is at least one node with degree
        // less than k then it is not a k-core;
        // if there is at least one node with degree 0,
        // then it is not connected.
        if( neighbours.size() < k || neighbours.size() == 0lu ) 
        {
            return false;
        }
    }
    return true;
}
