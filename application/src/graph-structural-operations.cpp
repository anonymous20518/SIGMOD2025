#include <queue>
#include <ranges>
#include <unordered_map>

#include "graph-structural-operations.hpp"

namespace { // anonymous


/**
 * Calculates a list in which element i is the degree of vertex i,
 * given an adjacency vector of vertex neighbours
 */
auto GenerateDegreeList( intVec2D const& adjacency_vectors ) -> vertex_degree_list_t
{
    vertex_degree_list_t degree_list;
    degree_list.reserve( adjacency_vectors.size() );

    std::transform( std::cbegin( adjacency_vectors )
                  , std::cend  ( adjacency_vectors )
                  , std::back_inserter( degree_list )
                  , []( vertex_list_t const& neighbour_list )
    {
        return neighbour_list.size(); // degree of node
    });

    return degree_list;
}

int RemoveNode (int k, int u, intVec2D const& edges, intVec const& process, intVec& sortedIndices, intVec& nodePosition, intVec& binBoundaries, std::queue<int>& remove)
{
    int removedNodesCount = 0 ; // storing the number of removed nodes

    for( vertex_id_t const e : edges[ u ] | std::views::reverse )
    {
        if (process[e]) // if that node is not removed
        { 
            removedNodesCount++;

            const int ePos = nodePosition[e];
            const int binNumber = std::upper_bound(binBoundaries.begin(), binBoundaries.end(), ePos) - binBoundaries.begin() - 1;
            const int binFirstIndex = binBoundaries[binNumber]; // index of the starting position of the bin e belongs to
            const int binFirstNode = sortedIndices[binFirstIndex];

            if (e != binFirstNode){  // swapping in sorted array and updating positions
                nodePosition[e] = binFirstIndex;
                nodePosition[binFirstNode] = ePos;
                sortedIndices[ePos] = binFirstNode;
                sortedIndices[binFirstIndex] = e;
            }

            binBoundaries[binNumber]+=1; //shifting the boundary to right
            if (binNumber - 1 < k) // nodes that fell into invalid bins
            { 
                remove.push(e); 
            }
        }
    }

    return removedNodesCount;
}

struct degree_is
{
    vertex_degree_list_t const& degrees;
    uint32_t   const  target;

    bool operator () ( vertex_id_t const vertex_id ) const
    {
        return degrees[ vertex_id ] == target;
    }
};

struct set_cores
{
    vertex_degree_list_t      & degrees;
    vertex_degree_list_t      & core_vals;
    uint32_t   const  k;

    void operator () ( vertex_id_t const vertex_id )
    {
        assert( degrees[ vertex_id ] == k );
        degrees[ vertex_id ]          = 0u;
        core_vals[ vertex_id ]        = k;
    }
};

/**
 * Decrements the neighbour's degree for every outgoing edge from a vertex in vertex_set
 * and returns the set of affected vertices
 */
vertex_list_t subtract_from_neighbours( adjacency_list_t     const& adjacency_vectors
                                      , vertex_list_t        const& vertex_set
                                      , vertex_degree_list_t      & degrees
                                      , uint32_t             const  threshold )
{
    vertex_list_t neighbours;
    neighbours.reserve( adjacency_vectors.size() );

    for( auto const v : vertex_set )
    {
        std::for_each( adjacency_vectors[v].cbegin()
                     , adjacency_vectors[v].cend()
                     ,  [ &degrees, &neighbours, threshold ]( auto const u )
                        {
                            if( degrees[ u ] > threshold )
                            {
                                --degrees[ u ];
                                neighbours.push_back( u );
                            }
                        } );
    }

    std::sort( std::begin( neighbours ), std::end( neighbours ) );
    neighbours.erase( std::unique( std::begin( neighbours )
                                 , std::end  ( neighbours ) )
                    , std::end( neighbours ) );

    return neighbours;
}

/** Returns a new Container consisting of the elements in input that match pred */
template < typename Container, class UnaryPredicate >
vertex_list_t filter( Container const& input , UnaryPredicate const pred )
{
    Container filtered;
    filtered.reserve( input.size() );

    std::copy_if( std::cbegin       ( input )
                , std::cend         ( input )
                , std::back_inserter( filtered )
                , pred );

    return filtered;
}


} // namespace anonymous

auto GetCoreNumbers( adjacency_list_t const& adjacency_vectors ) -> vertex_degree_list_t
{
    std::size_t const num_vertices = adjacency_vectors.size();

    vertex_degree_list_t degrees = GenerateDegreeList( adjacency_vectors );
    vertex_degree_list_t core_vals( num_vertices, -1);

    vertex_list_t relevant_vertices(num_vertices);
    std::iota(relevant_vertices.begin(), relevant_vertices.end(), 0);

    for( auto level = 1u; relevant_vertices.size() > level; ++level )
    {
        auto vertices_to_delete = filter( relevant_vertices
                                        , degree_is{ degrees, level } );

        while( ! vertices_to_delete.empty() )
        {
            std::for_each( std::begin( vertices_to_delete )
                         , std::end  ( vertices_to_delete )
                         , set_cores{ degrees, core_vals, level } );
            
            vertices_to_delete = filter( subtract_from_neighbours( adjacency_vectors
                                                                 , vertices_to_delete
                                                                 , degrees
                                                                 , level )
                                       , degree_is{ degrees, level } );
        }
      
        relevant_vertices.erase( std::remove_if( std::begin( relevant_vertices )
                                               , std::end  ( relevant_vertices )
                                               , [ &degrees, level ]( auto const v ){ return degrees[ v ] <= level; } )
                               , std::end( relevant_vertices ) );

    }

    return core_vals;
}


bool IsConnected( vertex_list_t const& vertex_subset, adjacency_list_t const& edges )
{
    std::unordered_map <int,int> hashMap;
    for (size_t i = 0, n = vertex_subset.size(); i < n; i++)
    {
        hashMap[vertex_subset[i]] = i; // assigning each node an ID
    }
    intVec visited(vertex_subset.size(),0);
    size_t count = 0;
    std::queue<int> queue;
    queue.push(0);
    while (!queue.empty()) // traversing the graph to check if it's connected
    { 
        int node = queue.front();
        queue.pop();
        if (visited[node] == 0)
        {
            visited[node] = 1;
            count++;
            for( vertex_id_t const e : edges[ node ] | std::views::reverse ) 
            {
                queue.push(hashMap[e]); //ID of the destination node
            }
        }
    }
    return count == vertex_subset.size();
}

bool IsKCore( vertex_list_t const& vertex_subset, adjacency_list_t const& edges, std::size_t k )
{
    for( vertex_id_t const nodeID : vertex_subset)
    {
        vertex_list_t const neighbours = GetIntersection( std::crbegin( edges[ nodeID ] )
        												, std::crend  ( edges[ nodeID ] )
        												, std::cbegin ( vertex_subset )
        												, std::cend   ( vertex_subset ) );
        // if there is at least one node with degree
        // less than k then it is not a k-core 
        if( neighbours.size() < k ) 
        {
            return false;
        }
    }
    return true;
}

auto ShrinkToMaxKCoreVertices( int k
	                         , std::optional< vertex_id_t > initial_vertex_to_remove
	                         , adjacency_list_t const& edges
	                         , vertex_list_t & sortedIndices
	                         , vertex_list_t & nodePosition
	                         , vertex_list_t & binBoundaries
	                         , vertex_list_t & active_vertices ) -> std::pair<int,int>
{
    std::queue< vertex_id_t > vertices_to_remove;

    if( initial_vertex_to_remove.has_value() )
    {
        vertices_to_remove.push( initial_vertex_to_remove.value() );
    }
    else
    {
        if (binBoundaries.size() >= static_cast< size_t> (k+1))
        {
            for (int i = 0; i < binBoundaries[k]; i++)
            {
                vertices_to_remove.push(sortedIndices[i]); //all nodes with degree less than k should be removed, binBoundaries[k] is the first node in sorted list with degree at least k
            }
        }
        else
        {
            std::fill(active_vertices.begin(), active_vertices.end(), 0);
            return std::make_pair(edges.size(), 0);
        }
    }

    int removedNodesCount = 0; //storing the number of removed nodes
    int removedEdgesCount = 0; //storing the number of removed edges
    while(!vertices_to_remove.empty())
    {
        const int v = vertices_to_remove.front();
        vertices_to_remove.pop();
        if (active_vertices[v])  // if it's not removed yet
        {
            active_vertices[v]=0;
            removedEdgesCount += RemoveNode(k, v, edges, active_vertices, sortedIndices, nodePosition, binBoundaries, vertices_to_remove);
            removedNodesCount += 1;
        }
    }
    return std::make_pair(removedNodesCount, removedEdgesCount);
}

auto GetKHopNeighbourhood( vertex_id_t u
                         , adjacency_list_t const& edges
                         , vertex_list_t const& active_vertices
                         , std::optional< vertex_degree_t > num_hops ) -> vertex_list_t
{
    vertex_id_t const remaining = edges.size() - u; // number of nodes id of which >= nodeId
    vertex_degree_t const k = num_hops.has_value() ? num_hops.value() : remaining; // if no max on hops, use remaining which exceeds graph diameter

    // initialise empty result set
	vertex_list_t result;
    result.reserve( remaining );

    using queue_element = std::pair< vertex_id_t, vertex_degree_t >; // a vertex and the number of hops to get there

    // initialise breadth-first traversal
    std::vector< bool > visited( remaining, false ); // keep track of nodes with id >= nodeId that are reached
    std::queue< queue_element > q;
    q.push( { u, 0 } );
    vertex_id_t num_visited = 0;

    // breadth-first traversal to find all reachable vertices
    while ( ! q.empty() && num_visited < remaining )
    { 
        auto const [ v, hops ] = q.front();
        q.pop();

        if( ! visited[ v - u ] ) // subtract off u because we only check those vertices after u.
        {
            visited[ v - u ] = true;
            ++num_visited;

            if( hops < k )
            {
                for( vertex_id_t const neighbour : edges[ v ] ) // no need for reverse; reordered later.
                {
                    // Check if vertex has been pruned but also for the sake of parallel code
                    // whether there are neighbours before u that have not yet been
                    // removed from the graph yet because they are being concurrently processed
                    if( active_vertices[ neighbour ] )
                    {
                        if( neighbour > u && ! visited[ neighbour - u ] )
                        {
                            q.push( { neighbour, hops + 1 } );
                        }
                    }
                }
            }
        }
    }

    // Create a sorted output list
    std::ranges::copy_if( std::views::iota( u, u + remaining )
                        , std::back_inserter( result )
                        , [ &visited, u ]( auto const i ){ return visited[ i - u ]; } );

    return result;
}
