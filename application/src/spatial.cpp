#include <ranges>

#include "spatial.hpp"

namespace { // anonymous
namespace PartitionByMedian {

using partition_rank_t = partition_mask_t;

/**
 * Calculates the dimension-wise median of a set of points
 */
auto GetPivot( label_list_t const& labels ) -> vertex_label_t
{
    assert( "Need points to calculate a pivot!" && labels.size() > 0 );

    std::size_t const n = labels.size();
    dimension_t const num_dimensions = std::cbegin( labels )->size();

    auto pivot = vertex_label_t( num_dimensions );

    // Iterate dimension by dimension to get median coordinate of the pivot
    for( dimension_t d = 0; d < num_dimensions; ++d )
    {
        // first gather all coordinates for this dimension from all points
        vertex_label_t labels_for_this_dimension;
        labels_for_this_dimension.reserve( n );

        std::transform( std::cbegin( labels )
                      , std::cend  ( labels )
                      , std::back_inserter( labels_for_this_dimension )
                      , [ d ]( vertex_label_t const& point )
                        {
                            return point[ d ];
                        } );

        // then sort and extract median
        std::sort( std::begin( labels_for_this_dimension )
                 , std::end  ( labels_for_this_dimension ) );

        pivot[ d ] = labels_for_this_dimension[ n / 2 ]; // median
    }

    return pivot;
}

/**
 * Given a set of points, calculates a mapping from each point index
 * to a partition mask indicating on which dimensions its coordinate
 * is larger than the median
 */
auto PartitionData( label_list_t const& points ) -> mask_list_t
{
    mask_list_t partition_for_each_point;
    partition_for_each_point.reserve( points.size() );

    vertex_label_t const pivot = GetPivot( points );

    std::transform( std::cbegin( points )
                  , std::cend  ( points )
                  , std::back_inserter( partition_for_each_point )
                  , [ &pivot ]( vertex_label_t const& point )
                    {
                        return DeterminePartition( point, pivot );
                    } );

    return partition_for_each_point;
}


/**
 * A temporary struct for storing sort keys for points when sorting them
 * by partition
 */
struct ExtendedPoint
{
    point_index_t id;
    coordinate_t sum; // L1 norm
    partition_mask_t mask; // mask value
    partition_rank_t setBitsCount; // number of set bits in mask
    vertex_label_t label;
};

/**
 * Functor that orders two points per ascending |mask|, mask, L1
 */
bool operator < ( ExtendedPoint const& a, ExtendedPoint const& b )
{
    if     ( a.setBitsCount < b.setBitsCount ) { return true;  }
    else if( a.setBitsCount > b.setBitsCount ) { return false; }

    else if( a.mask < b.mask ) { return true;  }
    else if( a.mask > b.mask ) { return false; }

    else if( a.sum < b.sum ) { return true;  }
    else if( a.sum > b.sum ) { return false; }

    return a.label < b.label;
}

/**
 * Determines for a set of points and matching point partitions a mapping from index
 * to sort rank if ordered by:
 *   1. popcount of the partition mask
 *   2. integer representation of the partition mask
 *   3. L1 norm of the points' coordinates
 *   4. final tie-breaking on the points themselves
 */ 
auto GetIndicesSortedByPartition( label_list_t const& points
                                , mask_list_t const& point_partitions ) -> index_list_t
{
    index_list_t sorted_point_indexes;
    sorted_point_indexes.reserve( points.size() );

    // transform each point into a materialised "extended point" for sorting
    std::vector< ExtendedPoint > sort_keys;
    sort_keys.reserve( points.size() );

    std::ranges::transform( std::views::iota( 0u, points.size() )
                          , std::back_inserter( sort_keys )
                          , [ &points, &point_partitions ]( point_index_t const index )
                          {
                            return ExtendedPoint{ index
                                                , std::accumulate( std::cbegin( points[ index ] )
                                                                 , std::cend  ( points[ index ] )
                                                                 , 0 )
                                                , point_partitions[ index ]
                                                , static_cast< partition_rank_t >( std::popcount( point_partitions[ index ] ) )
                                                , points[ index ] };
                          } );

    // Perform the actual sorting
    std::sort( std::begin( sort_keys ), std::end( sort_keys ) );

    // Extract out just the sorted indices, since we don't need all the other stuff anymore
    std::transform( std::cbegin( sort_keys )
                  , std::cend  ( sort_keys )
                  , std::back_inserter( sorted_point_indexes )
                  , []( ExtendedPoint const& point )
                    {
                        return point.id;
                    } );

    return sorted_point_indexes;
}

} // namespace PartitionByMedian

/**
 * Given a list of points and a corresponding list of point partitions,
 * where the i'th point partition is that of the i'th point, as well
 * a mapping from current index to desired index, produces a list of Point
 * structs that have been reordered per that mapping.
 */
auto ReorderPointsByIndex( label_list_t const& points
                         , mask_list_t const& point_partitions
                         , index_list_t const& sorted_indexes ) -> PointList
{
    PointList sorted_points;
    sorted_points.reserve( points.size() );

    std::ranges::transform( std::views::iota( 0u, points.size() )
                          , std::back_inserter( sorted_points )
                          , [ &points, &point_partitions, &sorted_indexes ]( point_index_t const index )
                            {
                                return Point{ index
                                            , point_partitions[ sorted_indexes[ index ] ]
                                            , points[ sorted_indexes[ index ] ] };
                            } );
    return sorted_points;
}

} // namespace anonymous


auto OrderByPartition( label_list_t const& points ) -> std::pair< index_list_t, PointList >
{

    mask_list_t  const point_partitions = PartitionByMedian::PartitionData( points );
    index_list_t const sorted_indices   = PartitionByMedian::GetIndicesSortedByPartition( points, point_partitions );

    return std::make_pair( sorted_indices
                         , ReorderPointsByIndex( points, point_partitions, sorted_indices ) );
}
