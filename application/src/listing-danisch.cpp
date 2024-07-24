#include <algorithm>
#include <cassert>
#include <ranges>
#include <numeric>

#include "listing-danisch.hpp"

namespace base {

    AdjacencyMap adjListToMap(AdjacencyList const& adjList)
    {
        AdjacencyMap adjMap;
        adjMap.reserve(adjList.size());

        // transform each neighbour list into an (id, neighbours) pair
        for(auto i = 0lu, n = adjList.size(); i < n; ++i)
        {
            adjMap.push_back(std::make_pair(i, adjList[i]));
        }
        return adjMap;
    }

namespace listing {

        /**
         * Checks whether a range contains a given value.
         * Mostly a wrapper for the std::unordered_set API.
         * If we use other features from cpp20, could consider .contains()
         */
        template <typename Iterator, typename T>
        bool contains(Iterator start, Iterator end, T const& val)
        {
            return std::find(start, end, val) != end;
        }

        /**
         * Filters a list of neighbours to those within a set of valid nodes
         */
        NodeList filterNeighbours(NodeList neighbours, NodeSet const& valid_nodes, Node min_node)
        {
            neighbours.erase(
                std::remove_if(neighbours.begin(), neighbours.end(),
                    [&valid_nodes, min_node](auto const node)
                    {
                        return (!contains(std::cbegin(valid_nodes), std::cend(valid_nodes), node))
                        || (node < min_node);
                    })
                , neighbours.end());

            return neighbours;
        }

        AdjacencyMap induceSubgraph(AdjacencyMap const& adjMap, NodeSet const& vertices_to_keep)
        {
            AdjacencyMap filtered_graph;
            filtered_graph.reserve(adjMap.size());

            for(auto const& [v, neighbours] : adjMap)
            {
                if(contains(std::cbegin(vertices_to_keep), std::cend(vertices_to_keep), v))
                {
                    auto new_neighbours = filterNeighbours(neighbours, vertices_to_keep, v + 1);
                    filtered_graph.push_back(std::make_pair(v, new_neighbours));
                }
            }

            return filtered_graph;
        }

        NodeSet filterToOutNeighbours(NodeList const& neighbours, Node source)
        {
            NodeSet filtered_neighbours;
            std::copy_if(std::crbegin(neighbours), std::crend(neighbours), std::inserter(filtered_neighbours, std::begin(filtered_neighbours)),
                [source](auto const& node1)
                {
                    return node1 > source;
                });
            return filtered_neighbours;
        }

        /**
         * Algorithm 2 from Danisch et al. (WWW 2018) "Listing k-cliques in Sparse Real-World Graphs."
         * Accrues result in input-output parameter cliques.
         */
        void listingRecursive(CliqueList & cliques, size_t remaining, AdjacencyMap const& adjMap, Clique & cur_group)
        {
            assert("Danisch et al. (WWW'18) assume cliques must have at least two elements" && remaining >= 2);

            if(remaining == 2)
            {
                // Base case.
                // Create a clique out of all remaining edges and add to result
                for(auto const& [u, neighbours] : adjMap)
                {
                    cur_group.push_back(u);
                    for(auto const& n : neighbours | std::views::reverse )
                    {
                        cur_group.push_back(n);
                        cliques.push_back(cur_group);
                        cur_group.pop_back();
                    }
                    cur_group.pop_back();
                }
            }
            else
            {
                // Recursive case.
                // For every node remaining in the graph, filter the graph to its neighbours and recurse.
                for(auto const& [u, neighbours] : adjMap)
                {
                    cur_group.push_back(u);
                    listingRecursive(cliques, remaining - 1, induceSubgraph(adjMap, filterToOutNeighbours(neighbours, u)), cur_group);
                    cur_group.pop_back();
                }
            }
        }

        // Wrapper to call recursive function.
        CliqueList getCliquesContainingNode(AdjacencyMap const& adjMap, Node seedNode, size_t cliqueSize)
        {
            assert("seedNode has at least two neighbours" && adjMap.size() >= 2lu);

            CliqueList cliques;
            Clique initial_group;
            initial_group.reserve(cliqueSize);
            initial_group.push_back(seedNode);

            listingRecursive(cliques, cliqueSize - 1, adjMap, initial_group);

            return cliques;
        }

        // Wrapper to call recursive function.
        CliqueList getAllCliques(AdjacencyList const& adjList, size_t cliqueSize)
        {
            CliqueList cliques;
            Clique initial_group;
            initial_group.reserve(cliqueSize);

            listingRecursive(cliques, cliqueSize, adjListToMap(adjList), initial_group);

            return cliques;
        }

    } // namespace listing
} // namespace base
