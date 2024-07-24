#include <iostream>

#include "graph.hpp"
#include "timer.hpp"
#include "ns-functions.hpp"
#include "zhang19.hpp"

namespace { // anonymous

enum class Algorithm
{
    skcore = 0,
    pkcore = 1,
    baseline = 2
};

} // namespace anonymous


int main (int argc, char** argv)
{
    if (argc < 7)
    {
        std::cout << "not enough input parameters...\n";
        return -1;
    }   

    const int kPlexSize = std::stoi(argv[1]); // k
    const int groupSize = std::stoi(argv[2]); // g
    const int coreSize = groupSize - kPlexSize - 1; // convert to co-plex for backwards compatability with earlier code design choices.
    const int dimension = std::stoi(argv[3]); // d
    const int dataset = std::stoi(argv[4]); // 4:YouTube, 5:case study, 10:LiveJournal, 11:DBLP, 12:Amazon, 13:WikiTalk, 14:CitPatent
    const int labelType = std::stoi(argv[5]); // 0:indep, 1:corr, 2:anti-corr
    const Algorithm algorithm = static_cast< Algorithm >( std::stoi(argv[6]) ); // 0:skcore, 1:pkcore, 2:baseline
    int nThreads = 0;

    if (groupSize > 2*coreSize+1 || groupSize <= coreSize)
    {
        std::cout << "invalid k and/or g parameters...\n";
        return -1;
    }

    std::cout << "coreSize = " << coreSize << std::endl;
    std::cout << "groupSize = " << groupSize << std::endl;
    std::cout << "dimension = " << dimension << std::endl;

    int nodeSize = 0 ; // to be updated based on "dataset", this is the maximum nodeID which may be different than the actual number of nodes
    std::string edgesFilePath = ""; // to be updated based on "dataset"

     // to be updated based on "d" and "labelType"
    std::string labelsFilePath = "../../datasets/";
    switch(dataset)
    {
        case 4:
            edgesFilePath = "../../datasets/com-youtube.ungraph_undirected.csv";
            nodeSize = 1157828;
            std::cout << "dataset = YouTube" << std::endl;
            break;
        case 5:
            edgesFilePath = "../../datasets/case_study_edges.csv";
            nodeSize = 5856;
            std::cout << "dataset = Casestudy" << std::endl;
            break;
        case 10:
            edgesFilePath = "../../datasets/LiveJournal_undirected.csv";
            nodeSize = 4847571;
            std::cout << "dataset = LiveJournal" << std::endl;
            break;
        case 11:
            edgesFilePath = "../../datasets/dblp_undirected.csv";
            nodeSize = 425957;
            std::cout << "dataset = DBLP" << std::endl;
            break;
        case 12:
            edgesFilePath = "../../datasets/amazon_undirected.csv";
            nodeSize = 735324;
            std::cout << "dataset = Amazon" << std::endl;
            break;
        case 13:
            edgesFilePath = "../../datasets/wiki_talk_undirected.csv";
            nodeSize = 2394385;
            std::cout << "dataset = WikiTalk" << std::endl;
            break;
        case 14:
            edgesFilePath = "../../datasets/cit_patents_undirected.csv";
            nodeSize = 6009555;
            std::cout << "dataset = CitPatent" << std::endl;
            break;
        default:
            std::cout << "invalid dataset...\n";
            return -1;
    }

    switch(labelType)
    {
        case 0:
            labelsFilePath += "indep-";
            std::cout << "label type = independent" << std::endl;
            break;
        case 1:
            labelsFilePath += "corr-scale=0.5-";
            std::cout << "label type = correlated" << std::endl;
            break;
        case 2:
            labelsFilePath += "anticorr-";
            std::cout << "label type = anti-correlated" << std::endl;
            break;
        default:
            std::cout << "invalid label type...\n";
            return -1;
    }

    labelsFilePath += std::to_string(dimension) + "d.csv";

    if (algorithm == Algorithm::pkcore)
    {
        if (argc < 8 || std::stoi(argv[7]) < 1)
        {
            std::cout << "invalid number of threads...\n";
            return -1;
        }
        nThreads = std::stoi(argv[7]);
        std::cout << "running in parallel - > #threads: " << nThreads << std::endl;
    }
    else
    {
        std::cout << "running sequentially...\n";
    }

    std::cout << "--**--**--**--**\n";

    Graph myGraph;
    {
        Time time("Preprocessing Time: ");
        if (LoadEdges(edgesFilePath, myGraph.edges, nodeSize) && LoadLabels(labelsFilePath, myGraph.labels, nodeSize))
        {
            std::cout << "Data loaded...\n";
        }
        else
        {
            return -1;
        }
        UpdateGraph(myGraph);
        std::cout << "Preprocessing done...\n";
    }

    intVec2D communities;
    if (algorithm == Algorithm::skcore)
    {
        Time time("SK-Core Execution Time: ");
        communities = base::sequential::GetSkylineCommunities( myGraph, coreSize, groupSize );
    }
    else if (algorithm == Algorithm::pkcore)
    {
        auto const num_threads = std::atoi( argv[ 7 ] );
        Time time("PK-Core Execution Time: ");
        communities = base::parallel::GetSkylineCommunities( myGraph, coreSize, groupSize, num_threads );
    }
    else if (algorithm == Algorithm::baseline)
    {
        Time time("Baseline Execution Time: ");
        base::zhang::GetSkylineCommunities(myGraph, coreSize, groupSize, communities);
    }
    else 
    {
        std::cout << "invalid algorithm...\n";
        return -1;
    }

    std::cout << "#Skyline Groups: " <<  communities.size() << std::endl;

    return 0; 
}