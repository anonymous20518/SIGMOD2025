/**
 * Implementation of Algorithm 2 from Danisch et al. (WWW 2018),
 * "Listing k-cliques in Sparse Real-World Graphs."
 * Discovers all cliques of a fixed size k in an input graph G
 * in time O(km(c(G)/2)^{k-2} + m).
 */

#include <stddef.h>        // size_t
#include <vector>          // std::vector<>
#include <unordered_set>   // std::unordered_set<>

namespace base {

        using Node = int;
        using NodeList = std::vector<Node>;
        using NodeSet = std::unordered_set<Node>;
        using AdjacencyList = std::vector<NodeList>;
        using AdjacencyMap = std::vector<std::pair<Node, NodeList>>;
        using Clique = std::vector<Node>;
        using CliqueList = std::vector<Clique>;

        /**
         * Converts an AdjacencyList into an AdjacencyMap in which the size of each neighbour list
         * must be at least cliqueSize (simple early pruning condition for clique listing)
         */
        AdjacencyMap adjListToMap(AdjacencyList const& adjList);


namespace listing {

        /**
         * Induces a subgraph on the set of neighbours of a given node and only contains
         * nodes with a degree at least as large as min_neighbours
         * (i.e., is a k-core for k=min_neighbours).
         * Heuristically-determined, not exact. Just used to reduce computation, not for correctness.
         */
        AdjacencyMap induceSubgraph(AdjacencyMap const& adjMap, NodeSet const& vertices_to_keep);
        
        CliqueList getCliquesContainingNode(AdjacencyMap const& adjMap, Node seedNode, size_t cliqueSize);
        CliqueList getAllCliques(AdjacencyList const& adjList, size_t cliqueSize);

    } // namespace listing
} // namespace base
