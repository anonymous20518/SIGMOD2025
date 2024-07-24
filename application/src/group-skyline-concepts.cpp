#include <algorithm>
#include <limits>

#include "group-skyline-concepts.hpp"

namespace { // anonymous

template < typename Func >
auto calculateRepresentative( vertex_list_t const& group, label_list_t const& labels, int init, Func f ) -> vertex_label_t
{
    auto representative = vertex_label_t( std::cbegin( labels )->size(), init );

    for( auto const& vertex : group )
    {
    	std::transform( std::cbegin( representative )
    	              , std::cend  ( representative )
    	              , std::cbegin( labels[ vertex ] )
    	              , std::begin ( representative )
    	              , f );
    }

    return representative;
}

} // namespace anonymous



// Don't bother dividing by count because comparisons of averages are equivalent
// to comparison of sums for fixed-size groups and average may require a cast to a non-integral type
auto GetAverageVirtualPoint( vertex_list_t const& group, label_list_t const& labels) -> vertex_label_t
{
    return calculateRepresentative(group, labels, 0, std::plus<int>());
}

auto GetBestVirtualPoint( vertex_list_t const& group, label_list_t const& labels) -> vertex_label_t
{
    return calculateRepresentative(group, labels, std::numeric_limits<int>::max(), [](auto const x, auto const y){ return std::min(x, y); });
}

auto GetWorstVirtualPoint( vertex_list_t const& group, label_list_t const& labels) -> vertex_label_t
{
    return calculateRepresentative(group, labels, std::numeric_limits<int>::lowest(), [](auto const x, auto const y){ return std::max(x, y); });
}
