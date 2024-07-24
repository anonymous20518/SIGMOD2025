#include <algorithm>
#include <fstream>
#include <omp.h>
#include <ranges>
#include <tuple>

#include "dominance-tests.hpp"
#include "graph.hpp"
#include "graph-structural-operations.hpp"
#include "group-skyline-concepts.hpp"
#include "listing-cousins-first.hpp"
#include "postprocessing.hpp"

#include "ns-functions.hpp"

namespace { // anonymous

template < typename Range, typename ActiveSet >
auto GetLastGroup( Range range, vertex_id_t groupSize, ActiveSet const& is_active_vertex ) -> group_t
{
    group_t newGroup;
    newGroup.reserve( groupSize );
    std::ranges::copy_if( range
                        , std::back_inserter( newGroup )
                        , [ &is_active_vertex ]( auto const i )
                          {
                            return is_active_vertex[ i ];
                          } );
    return newGroup;
}

void UpdateSkyline( group_t const& candidate_group
                  , label_list_t const& labels
                  , group_list_t & skylineCommunities
                  , label_list_t & skylineRepresentatives )
{
    if( ! IsDominatedBySkyline( candidate_group, skylineCommunities, labels ) )
    {
        skylineCommunities.push_back( candidate_group );
        skylineRepresentatives.push_back(GetWorstVirtualPoint( candidate_group, labels ) );
    }
}

} // namespace anonymous


namespace base
{

auto InitialiseToMaxKCore( Graph & myGraph, int coreSize ) -> std::tuple< vertex_degree_t, vertex_list_t, vertex_list_t, vertex_list_t, vertex_list_t >
{
    auto [ sortedIndices, nodePosition, binBoundaries ] =  BinSortByDegree( myGraph.edges );
    vertex_list_t in_maximal_kcore(myGraph.size, 1);
    int num_remaining_vertices = myGraph.size - ShrinkToMaxKCoreVertices(coreSize, std::nullopt, myGraph.edges, sortedIndices, nodePosition, binBoundaries, in_maximal_kcore).first;

    return std::make_tuple( num_remaining_vertices, in_maximal_kcore, sortedIndices, nodePosition, binBoundaries );
}

bool CheckBoundaryCases( Graph & myGraph
                       , vertex_degree_t num_vertices
                       , vertex_id_t next_vertex
                       , int groupSize
                       , vertex_list_t const& in_maximal_kcore
                       , group_list_t & skylineCommunities
                       , label_list_t & skylineRepresentatives
                       , int & layerNumber )
{
    if( num_vertices < groupSize ) // no remaining groups
    {
        return true;
    }
    else if( num_vertices == groupSize ) // exactly one remaining group
    {
        UpdateSkyline( GetLastGroup( std::views::iota( next_vertex, myGraph.size ), groupSize, in_maximal_kcore )
                     , myGraph.labels
                     , skylineCommunities
                     , skylineRepresentatives );
        return true;
    }
    else if( next_vertex == myGraph.skyLayersBoundaries[ layerNumber ] )
    {
        if( CanTerminate( skylineRepresentatives, myGraph.layerRepresentatives[ layerNumber ] ) )
        {
            return true;
        }
        else
        {
            ++layerNumber;
        }
    }
    return false;
}

auto FilterVertices( vertex_id_t next_vertex
                   , adjacency_list_t const& edges
                   , label_list_t const& labels
                   , vertex_degree_t groupSize
                   , vertex_degree_t minCoreness
                   , vertex_list_t const& in_maximal_kcore
                   , group_list_t & skylineCommunities
                   , label_list_t & skylineRepresentatives ) -> vertex_list_t
{
    vertex_list_t const nodes = GetKHopNeighbourhood( next_vertex
                                                    , edges
                                                    , in_maximal_kcore
                                                    , groupSize - minCoreness == 1 ? 1 : 2 );
    if( nodes.size() == static_cast< size_t >( groupSize ) ) // if nodes is the only kcore connected group, check dominance
    {
        if( IsKCore(nodes, edges, minCoreness ) )
        {
            UpdateSkyline( nodes
                         , labels
                         , skylineCommunities
                         , skylineRepresentatives );
        }
    }

    return nodes;
}

void ListAndCheckGroups( vertex_list_t const& vertices
                       , adjacency_list_t const& edges
                       , label_list_t const& labels
                       , vertex_degree_t groupSize
                       , vertex_degree_t coreSize
                       , group_list_t & skylineCommunities
                       , label_list_t & skylineRepresentatives )
{
    group_list_t candidates = listing::cousins_first::ListKCoresWithPrefix( vertices, edges, groupSize, coreSize );

    std::vector< bool > bIsSkyline( candidates.size(), true );
    for( size_t const i : std::views::iota( 0lu, candidates.size() ) )
    {
        bIsSkyline[ i ] = !IsDominatedBySkyline(candidates[i], skylineCommunities, labels) && !IsDominatedByCandidates( i, candidates, bIsSkyline, labels );
        if( bIsSkyline[ i ] ) 
        {
            skylineCommunities.push_back(candidates[i]); // adding to skyline communitites
            skylineRepresentatives.push_back(GetWorstVirtualPoint(candidates[i], labels));
        }
    }
}

namespace sequential
{   
    auto GetSkylineCommunities( Graph & myGraph, int coreSize, int groupSize ) -> group_list_t
    {
        group_list_t skylineCommunities;      // result set of skyline k-cores that will be built up
        label_list_t skylineRepresentatives;  // maximum extent of each skyline k-core to be used for pruning
        int layerNumber = 0;                  // counter for layers to detect when pruning conditions should be checked

        // Initialise by computing maximal k-core and marking as false all vertices not in it
        auto [ num_remaining_vertices, in_maximal_kcore, sortedIndices, nodePosition, binBoundaries ] = InitialiseToMaxKCore( myGraph, coreSize );

        // iterate each sorted vertex and if it's still in the continually shrinking max k-core
        // list all k-cores involving it and higher-id vertices and check them against the skyline
        for ( vertex_id_t next_vertex : std::views::iota( 0, myGraph.size) )
        {
            if( in_maximal_kcore[ next_vertex ] )
            {
                if( CheckBoundaryCases( myGraph, num_remaining_vertices, next_vertex, groupSize, in_maximal_kcore, skylineCommunities, skylineRepresentatives, layerNumber ) )
                {
                    break;
                }

                // Induce a sub-graph with this vertex to decrease listing time
                auto const filtered_vertices = FilterVertices( next_vertex, myGraph.edges, myGraph.labels, groupSize, coreSize, in_maximal_kcore, skylineCommunities, skylineRepresentatives );
                if( filtered_vertices.size() > static_cast< size_t >( groupSize ) )
                {
                    // Update the skyline with all new groups involving this vertex
                    ListAndCheckGroups( filtered_vertices, myGraph.edges, myGraph.labels, groupSize,  coreSize, skylineCommunities, skylineRepresentatives );
                    num_remaining_vertices -= ShrinkToMaxKCoreVertices( coreSize, next_vertex, myGraph.edges, sortedIndices, nodePosition, binBoundaries, in_maximal_kcore ).first;
                }
            }
            // else this vertex is not involved in any k-core groups.

            RemoveVertex( myGraph.edges, next_vertex ); // physically update adjacency lists to shrink graph size
        }

        // similar to remove-erase idiom; get rid of false positives
        skylineCommunities.erase( postprocess( std::begin( skylineCommunities )
                                             , std::end( skylineCommunities )
                                             , myGraph.labels )
                                , std::end( skylineCommunities ) );
        return skylineCommunities;
    }

    auto GenerateSummerPlot( Graph & myGraph, int coreSize) -> std::vector<size_t>
    {
        std::vector<size_t> vertex_count = {myGraph.size};

        auto [ num_remaining_vertices, in_maximal_kcore, sortedIndices, nodePosition, binBoundaries ] = InitialiseToMaxKCore( myGraph, coreSize );
        vertex_count.push_back(num_remaining_vertices);

        // iterate each sorted vertex and if it's still in the continually shrinking max k-core
        // list all k-cores involving it and higher-id vertices and check them against the skyline
        for ( vertex_id_t next_vertex : std::views::iota( 0, myGraph.size) )
        {
            if( in_maximal_kcore[ next_vertex ] && num_remaining_vertices > 0)
            {
                num_remaining_vertices -= ShrinkToMaxKCoreVertices( coreSize, next_vertex, myGraph.edges, sortedIndices, nodePosition, binBoundaries, in_maximal_kcore ).first;
                vertex_count.push_back(num_remaining_vertices);
            }
        }
    
        return vertex_count;
    }

    void to_file( const std::vector<size_t>& vector, const std::string filePath)
    {
        std::ofstream file(filePath);
        for (size_t v : vector) {
            file << v << "\n";
        }
    }
}

namespace parallel
{
    auto GetSkylineCommunities( Graph & myGraph, int coreSize, int groupSize, int nThreads ) -> group_list_t
    {
        group_list_t skylineCommunities;      // result set of skyline k-cores that will be built up
        label_list_t skylineRepresentatives;  // maximum extent of each skyline k-core to be used for pruning
        int layerNumber = 0;                  // counter for layers to detect when pruning conditions should be checked

        omp_set_num_threads( nThreads );
        intVec threadWorkingIndex(nThreads);
        std::vector<intVec2D> threadLocalCandidates(nThreads);
        intVec threadLocalCandidatesNum(nThreads);
        int bTerminate = false; // global early termination


        // Initialise by computing maximal k-core and marking as false all vertices not in it
        auto [ num_remaining_vertices, in_maximal_kcore, sortedIndices, nodePosition, binBoundaries ] = InitialiseToMaxKCore( myGraph, coreSize );

        int indexToProcess = 0;
        int indexToRemove = 0;

        #pragma omp parallel
        {
        while (!bTerminate &&  indexToProcess < myGraph.size)
        {
            vertex_id_t const first_vertex_in_iteration = indexToProcess; // so we know later where to start edge removals from.

            #pragma omp barrier
            #pragma omp single
            {
                std::fill(threadWorkingIndex.begin(), threadWorkingIndex.end(), myGraph.size);
                while (indexToRemove < indexToProcess)
                {
                    num_remaining_vertices -= ShrinkToMaxKCoreVertices(coreSize, indexToRemove, myGraph.edges, sortedIndices, nodePosition, binBoundaries, in_maximal_kcore).first;
                    if (num_remaining_vertices < groupSize)
                    {
                        bTerminate = true;
                        break;
                    }
                    indexToRemove++;
                }
                if (!bTerminate)
                {
                    if (num_remaining_vertices == groupSize)
                    {
                        UpdateSkyline( GetLastGroup( std::views::iota( indexToProcess, myGraph.size ), groupSize, in_maximal_kcore )
                                     , myGraph.labels
                                     , skylineCommunities
                                     , skylineRepresentatives );
                        bTerminate = true;
                    }
                    else
                    {
                        int tID = 0;
                        std::fill(threadWorkingIndex.begin(), threadWorkingIndex.end(), myGraph.size);
                        while(tID < nThreads && indexToProcess < myGraph.size)
                        {
                            if (in_maximal_kcore[indexToProcess])
                            {
                                threadWorkingIndex[tID++] = indexToProcess++;
                            }
                            else
                            {
                                indexToProcess++;
                            }
                        }
                        while(myGraph.skyLayersBoundaries[layerNumber] < indexToProcess && indexToProcess < myGraph.size)
                        {
                            if (CanTerminate(skylineRepresentatives, myGraph.layerRepresentatives[layerNumber]))
                            {
                                for (size_t id = 0; id < static_cast<size_t>(nThreads); id++)
                                {
                                    threadWorkingIndex[id] += myGraph.size * (threadWorkingIndex[id] >= myGraph.skyLayersBoundaries[layerNumber]);
                                }
                                bTerminate = true; // global
                                break;
                            }
                            layerNumber++;
                        }
                    }
                }
            }
            // implicit barrier

            const size_t threadID = omp_get_thread_num();
            const int index = threadWorkingIndex[threadID];
            threadLocalCandidates[threadID].clear();

            if( index < myGraph.size && GetDegree( myGraph.edges, index ) >= coreSize )
            {
                vertex_list_t const nodes = GetKHopNeighbourhood( index
                                                , myGraph.edges
                                                , in_maximal_kcore
                                                , groupSize - coreSize == 1 ? 1 : 2 );
                if (nodes.size() == static_cast< size_t >(groupSize) && IsKCore(nodes, myGraph.edges, coreSize) && !IsDominatedBySkyline(nodes, skylineCommunities, myGraph.labels)) // if nodes is the only kcore connected group
                { 
                    threadLocalCandidates[threadID].push_back(nodes);
                }
                else if (nodes.size() > static_cast< size_t >(groupSize))
                {
                    threadLocalCandidates[ threadID ] = listing::cousins_first::ListKCoresWithPrefix( nodes, myGraph.edges, groupSize, coreSize );
                }

                std::vector< bool > bIsSkyline(threadLocalCandidates[threadID].size(), true);
                for (size_t i = 0; i < threadLocalCandidates[threadID].size(); i++) // comparing to peers and known skylines
                { 
                    bIsSkyline[i] = !IsDominatedBySkyline(threadLocalCandidates[threadID][i], skylineCommunities, myGraph.labels) && !IsDominatedByCandidates(i, threadLocalCandidates[threadID], bIsSkyline, myGraph.labels);
                }
                for (int i = threadLocalCandidates[threadID].size() - 1; i >= 0; i--) // removing non-skyline candidates
                { 
                    if (!bIsSkyline[i])
                    {
                        threadLocalCandidates[threadID].erase(threadLocalCandidates[threadID].begin() + i);
                    }
                }
            }
            #pragma omp barrier

            intVec bIsSkyline(threadLocalCandidates[threadID].size(), true);
            for (size_t i = 0; i < threadLocalCandidates[threadID].size(); i++) // comparing to potential skylines found by threads with lower IDs
            {
                for (size_t j = 0; j < threadID && bIsSkyline[i]; j++)
                {
                    bIsSkyline[i] = !IsDominatedBySkyline(threadLocalCandidates[threadID][i], threadLocalCandidates[j], myGraph.labels);
                }
            }
            #pragma omp barrier

            for (int i = threadLocalCandidates[threadID].size() - 1; i >= 0; i--) // removing non-skyline candidates
            { 
                if (!bIsSkyline[i])
                {
                    threadLocalCandidates[threadID].erase(threadLocalCandidates[threadID].begin() + i);
                }
            }
            threadLocalCandidatesNum[threadID] = threadLocalCandidates[threadID].size();
            #pragma omp barrier 

            #pragma omp single
            {
                int startPosition = skylineCommunities.size();
                for (size_t tID = 0; tID < static_cast<size_t>(nThreads); tID++)
                {
                    const int tmp = threadLocalCandidatesNum[tID];
                    threadLocalCandidatesNum[tID] = startPosition;
                    startPosition += tmp;
                }
                skylineCommunities.resize(startPosition);
                skylineRepresentatives.resize(startPosition);

                // physically update adjacency lists to shrink graph size for all vertices in this batch iteration
                std::ranges::for_each( std::views::iota( first_vertex_in_iteration
                                                       , std::min( indexToProcess, myGraph.size ) )
                                     , [ &edges = myGraph.edges ]( vertex_id_t const vertex_to_remove )
                                     {
                                        RemoveVertex( edges, vertex_to_remove );
                                     } );
            }

            intVec2D representatives;
            representatives.reserve(threadLocalCandidates[threadID].size());
            for (const intVec& newSkyline : threadLocalCandidates[threadID]) // computing representatives
            { 
                representatives.push_back(GetWorstVirtualPoint(newSkyline, myGraph.labels));
            }
            std::copy(threadLocalCandidates[threadID].cbegin(), threadLocalCandidates[threadID].cend(), skylineCommunities.begin() + threadLocalCandidatesNum[threadID]);
            std::copy(representatives.cbegin(), representatives.cend(), skylineRepresentatives.begin() + threadLocalCandidatesNum[threadID]);

        } //  end of main for loop
        } // end of parallel region


        // similar to remove-erase idiom; get rid of false positives
        skylineCommunities.erase( postprocess( std::begin( skylineCommunities )
                                             , std::end( skylineCommunities )
                                             , myGraph.labels )
                                , std::end( skylineCommunities ) );
        return skylineCommunities;
    } // end of function
} // end of parallel namespace
} // end of base namespace