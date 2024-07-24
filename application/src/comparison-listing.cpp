#include <boost/program_options.hpp> // for handling input arguments
#include <iostream>

#include "listing-cousins-first.hpp"
#include "timer.hpp"

namespace { // anonymous

const char* ARG_HELP      = "help,h";
const char* ARG_INPUT     = "input-file,f";
const char* ARG_GROUP     = "group-size,g";
const char* ARG_DEGREE    = "group-coreness,k";
const char* ARG_ALGORITHM = "algorithm,a";

} // namespace anonymous

int main (int argc, char** argv)
{
    std::string     algorithms;
    std::string     input_file;
    vertex_degree_t group_size;
    vertex_degree_t min_coreness;

    // HACK ALERT!!!! 🙈🙈🙈🙈🙈🙈
    // these values should be read from the input file, not stored in a map 🙈🙈🙈🙈🙈🙈
    std::unordered_map< std::string, vertex_degree_t > node_counts = {{ "../../../Data/Datasets/Casestudy/case_study_edges.csv",                5856 }
                                                                    , { "../../../Data/Datasets/Youtube/com-youtube.ungraph_undirected.csv", 1157828 }
                                                                    , { "../../../Data/Datasets/LiveJournal/LiveJournal_undirected.csv",     4847571 }
                                                                    , { "../../../Data/Datasets/DBLP/dblp_undirected.csv",                   425957 }
                                                                    , { "../../../Data/Datasets/Amazon/amazon_undirected.csv",               735324 }
                                                                    , { "../../../Data/Datasets/WikiTalk/wiki_talk_undirected.csv",          2394385 }
                                                                    , { "../../../Data/Datasets/CitPatent/cit_patents_undirected.csv",       6009555 } };
 
    try
    {
        namespace po = boost::program_options;

        po::options_description desc("Execution arguments for comparing size-g k-core listing algorithms");
        desc.add_options()
            (ARG_HELP, "show usage instructions")
            (ARG_ALGORITHM, po::value<std::string>( &algorithms )->required(), "space-delimited list of algorithm; choices are: {cousins-first}")
            (ARG_DEGREE,    po::value<vertex_degree_t>( &min_coreness )->required(), "minimum degree in result sub-graph, i.e., subgraph coreness")
            (ARG_GROUP,     po::value<vertex_degree_t>( &group_size )->required(), "number of vertices in each group")
            (ARG_INPUT,     po::value<std::string>( &input_file )->required(), "path to file with edge list")
            ;

        po::variables_map vm;
        po::store( po::parse_command_line( argc, argv, desc ), vm );

        // If someone needs help, nothing else matters.
        if ( vm.count(ARG_HELP) || argc == 1 )
        {
            std::cout << desc << std::endl;
            return 0;
        }
        po::notify( vm );


        if( min_coreness < 1 )
        {
            std::cout << "Please specify a strictly positive minimum degree." << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        if( group_size < min_coreness + 1 )
        {
            std::cout << "There are no self-loops. Group size must be at least min_coreness + 1" << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        if( group_size > 2 * min_coreness + 1 )
        {
            std::cout << "We do not support k-cores where the group size is more than 1 + 2 * min_coreness.  "
                      << "See paper for details." << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        if( node_counts.count( input_file ) < 1 )
        {
            std::cout << "I have never seen this file path before. Sorry." << std::endl;
            std::cout << desc << std::endl;
            return 0;
        }

        std::cout << "Input: " << input_file << std::endl;
        std::cout << "Group size: " << group_size << std::endl;
        std::cout << "Minimum coreness: " << min_coreness << std::endl;


        // Create input graph
        Graph input_graph;
        {
            Time time("Preprocessing Time: ");
            if( LoadEdges( input_file, input_graph.edges, node_counts[ input_file ] ) )
            {
                input_graph.size = input_graph.edges.size();

                // HACK!!! Reversing edge lists here cause it's done in UpdateGraph but I don't have labels.
                for( auto & neighbour_list : input_graph.edges )
                {
                    std::sort( std::begin( neighbour_list ), std::end( neighbour_list ), std::greater< vertex_id_t >{} );
                }
            }
            else
            {
                std::cerr << "Error loading input file: " << input_file << std::endl;
                std::cerr << "If it is relevant, we expected " << node_counts[ input_file ] << " contiguous vertex ids." << std::endl;
                return -1;
            }
        }

        std::stringstream algorithm_list( algorithms );
        std::string next_algorithm;

        while( algorithm_list >> next_algorithm )
        {
            if( next_algorithm.compare( "cousins-first" ) == 0 )
            {
                Time time("Cousins-first Listing Time: ");
                auto const all_kcores = base::listing::cousins_first::ListAllKCores( input_graph.edges, group_size, min_coreness );
                std::cout << "  #k-cores: " <<  all_kcores.size() <<"\n";
            }
            else 
            {
                std::cout << "Unrecognised algorithm: " << next_algorithm
                          << ".  Skipping algorithm." << std::endl;
            }
        }
    }
    catch ( std::exception const& e )
    {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Exception of unknown type!" << std::endl;
        return 1;
    }

    return 0; 
}
