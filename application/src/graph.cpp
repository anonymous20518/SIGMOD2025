#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ranges>
#include <sstream>

#include "graph-structural-operations.hpp"
#include "group-skyline-concepts.hpp"
#include "sky-layers.hpp"

#include "graph.hpp"

namespace { // anonymous

auto TransformEdgeLists( adjacency_list_t const& edges
                       , vertex_list_t const& sortedIndices
                       , vertex_list_t const& newIndices
                       , bool sortInReverse = false ) -> adjacency_list_t
{
    vertex_degree_t const n = edges.size();
    adjacency_list_t resorted_edge_list;
    resorted_edge_list.reserve( n );

    // First reorder the edge lists per the new vertex ordering
    std::transform( std::cbegin( sortedIndices )
                  , std::cend  ( sortedIndices )
                  , std::back_inserter( resorted_edge_list )
                  , [ &edges ]( vertex_id_t const v )
                  {
                    return edges[ v ];
                  } );

    // Then transform each individual neighbour list
    for( vertex_id_t const vertex : std::views::iota( 0, n ) )
    {
        // First relabel the edges
        std::transform( std::cbegin( resorted_edge_list[ vertex ] )
                      , std::cend  ( resorted_edge_list[ vertex ] )
                      , std::begin ( resorted_edge_list[ vertex ] )
                      , [ &newIndices ]( vertex_id_t const neighbour )
                      {
                        return newIndices[ neighbour ];
                      } );

        // Then resort the relabelled edges
        if( sortInReverse )
        {
            sort( std::begin( resorted_edge_list[ vertex ] )
                , std::end  ( resorted_edge_list[ vertex ] )
                , std::greater< vertex_id_t >{} );
        }
        else
        {
            sort( std::begin( resorted_edge_list[ vertex ] )
                , std::end  ( resorted_edge_list[ vertex ] ) );
        }
    }

    return resorted_edge_list;
}

auto InvertMapping( vertex_list_t const& mapping  ) -> vertex_list_t
{
    point_index_t const n = mapping.size();
    vertex_list_t reverse_mapping( n );

    for ( point_index_t newIndex = 0; newIndex < n; ++newIndex )
    {
        point_index_t const oldIndex = mapping[ newIndex ];
        reverse_mapping[ oldIndex ] = newIndex;
    }

    return reverse_mapping;
}

auto ReorderLabels(myLabelVec& labels, vertex_list_t const& sortedIndices ) -> label_list_t
{
	label_list_t sortedLabels;
    sortedLabels.reserve( labels.size() );

    std::transform( std::cbegin( sortedIndices )
    	          , std::cend  ( sortedIndices )
                  , std::back_inserter( sortedLabels )
                  , [ &labels ]( auto const index )
                  {
                    return labels[ index ];
                  } );

    return sortedLabels;
}

} // namespace anonymous



auto BinSortByDegree( adjacency_list_t const& edges ) -> std::tuple< vertex_list_t, vertex_list_t, vertex_list_t >
{
    int const n = edges.size();
    auto maxDegree = std::numeric_limits< int >::lowest();
    
    std::vector< vertex_list_t > bins( n ); // assuming no self loops
    for( int const i : std::views::iota( 0, n ) )
    {
        vertex_degree_t const degree = edges[ i ].size();
        bins[ degree ].push_back( i );
        maxDegree = std::max(maxDegree, degree);
    }

    vertex_list_t sortedIndices( n ), nodePosition( n ), binBoundaries( maxDegree + 1 );

    for(int i = 0, index = 0; i < maxDegree + 1; ++i )
    {
        binBoundaries[i] = index;
        for( int const j : std::views::iota( 0lu, bins[i].size() ) )
        {
            sortedIndices[index] = bins[i][j]; // filling sorted array
            nodePosition[bins[i][j]] = index; // storing the index of the node in the sorted array
            index++;
        }
    }

    return std::make_tuple( sortedIndices, nodePosition, binBoundaries );
}

bool LoadEdges (const std::string& filePath, intVec2D& edges, int nodeSize)
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        std::cout << "Could not open the file...\n";
        return false;
    }

    edges.clear();
    edges.resize(nodeSize);

    std::vector<std::string> row;
    std::string line, word;
    while (getline(inputFile, line))
    {
        row.clear();
        std::stringstream ss(line);
        while (getline(ss, word, ',')) 
        {
            row.push_back(word);
        }
        int u = std::stoi(row[0]);
        int v = std::stoi(row[1]);
        if (u < nodeSize && v < nodeSize){
            edges[u].push_back(v);
            edges[v].push_back(u);
        }
    }
    inputFile.close();

    for (auto& vec: edges) 
    {
        std::sort(vec.begin(), vec.end());
    }
    return true;
}

bool LoadLabels(const std::string& filePath, myLabelVec& labels, int nodeSize)
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        std::cout << "Could not open the file...\n";
        return false;
    }

    labels.clear();
    labels.resize(nodeSize);

    std::string line, word;
    int i = 0;
    while (getline(inputFile, line))
    {
        if (i == nodeSize)
        {
            break;
        }

        myLabel row;
        std::stringstream ss(line);
        while (getline(ss, word, ','))
        {
            row.push_back( std::stoi( word ) ); //vector
        }
        labels[i++] = row;
    }
    inputFile.close();
    return true;
}

void UpdateGraph (Graph& myGraph)
{ 
    myLabelVec& labels = myGraph.labels;
    intVec2D& edges = myGraph.edges;

    const int n = labels.size();
    myGraph.size = n;
    
    PointList sorted_points;
    std::tie(myGraph.toOriginal, sorted_points) = OrderByPartition( labels );

    labels = ReorderLabels( labels, myGraph.toOriginal );
    myGraph.toRelabelled = InvertMapping( myGraph.toOriginal );

    edges = TransformEdgeLists( edges, myGraph.toOriginal, myGraph.toRelabelled, true );

    auto skyLayers = GenerateSkyLayers( sorted_points );
    size_t const num_layers = skyLayers.size();
    
    // Starting position of points in layers
    myGraph.skyLayersBoundaries.resize(num_layers + 1);
    myGraph.skyLayersBoundaries[0] = 0;

    myGraph.layerRepresentatives.resize(num_layers);

    for (size_t i = 0; i < num_layers; ++i)
    {
        myGraph.skyLayersBoundaries[i + 1] = myGraph.skyLayersBoundaries[i] + skyLayers[i].size();

        intVec layerNodes(skyLayers[i].size());
        std::iota(layerNodes.begin(), layerNodes.end(), myGraph.skyLayersBoundaries[i]);
        myGraph.layerRepresentatives[i] = GetBestVirtualPoint(layerNodes, labels);
    }
}
