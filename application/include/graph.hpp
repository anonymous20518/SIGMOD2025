/**
 * Definitions of graph classes and operations thereupon
 */

#pragma once

#include <cassert>
#include <string>
#include <vector>

#include "spatial.hpp"

using vertex_id_t = int;
using vertex_degree_t = int; // should be unsigned.
using coordinate_t = int;

typedef std::vector<int> intVec;
typedef std::vector<std::vector<int>> intVec2D;

using vertex_list_t = std::vector< vertex_id_t >;
using vertex_degree_list_t = std::vector< vertex_degree_t >;
using group_list_t = std::vector< vertex_list_t >;
using adjacency_list_t = std::vector< vertex_list_t >;


struct Graph
{
    int size;
    myLabelVec labels;
    adjacency_list_t edges;
    intVec toOriginal;
    intVec toRelabelled;
    intVec skyLayersBoundaries; // starting position of each layer
    myLabelVec layerRepresentatives; // L- for each layer
};

void UpdateGraph( Graph & myGraph );
bool LoadEdges (const std::string& filePath, intVec2D& edges, int nodeSize); 
bool LoadLabels(const std::string& filePath, myLabelVec& labels, int nodeSize); 

/**
 * Sorts the vertices by degree using "Bin Sort" and returns the three objects---sortedIndices,
 * nodePosition, binBoundaries---so that the sort can be incrementally maintained. Used mostly
 * for maximum k-core calculations.
 */
auto BinSortByDegree( adjacency_list_t const& edges ) -> std::tuple< vertex_list_t, vertex_list_t, vertex_list_t >;

/**
 * Returns the number of edges incident to a given vertex
 */
inline
auto GetDegree( adjacency_list_t const& edges, vertex_id_t vertex ) -> vertex_degree_t
{
    return edges[ vertex ].size();
}

/**
 * Removes all edges incident to vertex_to_remove. Requires time propotional
 * to the degree of vertex_to_remove.
 *
 * @pre Assumes no vertices have neighbours with ids < vertex_to_remove
 * @pre Assumes neighbours are sorted in descending order
 */
inline
void RemoveVertex( adjacency_list_t & edges, vertex_id_t vertex_to_remove )
{
    for( vertex_id_t const neighbour : edges[ vertex_to_remove ] )
    {
        assert( "vertex_to_remove is at back of edge list" && edges[ neighbour ].back() == vertex_to_remove );
        edges[ neighbour ].pop_back();
    }
    edges[ vertex_to_remove ].clear();
}
