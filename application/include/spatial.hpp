/**
 * Spatial operations that relate to numeric, multi-variate vertex labels.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <vector>

using coordinate_t = int;                               // Data type for an individual coordinate of a point
using point_index_t = int;                              // Data type for recording the index of a point
using dimension_t  = int;                               // Data type for the number of dimensions in a point
using vertex_label_t = std::vector< coordinate_t >;     // A point is a list of coordinates. Ideally replace with std::array for contiguity
using label_list_t = std::vector< vertex_label_t >;     // A list of points stored contiguously
using partition_mask_t = uint32_t;                      // A bit mask used to identify a point relative to medians
using mask_list_t = std::vector< partition_mask_t >;    // A contiguous list of bit masks indicating partition for each point i
using index_list_t = std::vector< point_index_t >;      // A contiguous list of point indexes


// Deprecated typedefs.
// Please use the ones above and systematically
// replace these with the semantic identifiers above.
typedef std::vector<coordinate_t> myLabel; // float
typedef std::vector<myLabel> myLabelVec;


/**
 * Determines whether the vertex label on the left is strictly smaller
 * than the vertex label on the right. For use with sorting.
 */
inline
bool operator < ( vertex_label_t const& left, vertex_label_t const& right )
{
    for( auto i = 0lu, n = left.size(); i < n; ++i )
    {
        if( right[ i ] < left[ i ] )
        {
            return false;
        }
        else if( left[ i ] < right[ i ] )
        {
            return true;
        }
    }
    return false;
}

/**
 * Determines whether the vertex label on the left is smaller than or
 * equal to the vertex label on the right. For use with sorting.
 */
inline
bool operator <= ( vertex_label_t const& left, vertex_label_t const& right )
{
    for( auto i = 0lu, n = left.size(); i < n; ++i )
    {
        if( right[ i ] < left[ i ] )
        {
            return false;
        }
        else if( left[ i ] < right[ i ] )
        {
            return true;
        }
    }
    return true;
}

// used in sky layers and created in spatial.
struct Point
{
    point_index_t id;
    partition_mask_t mask;
    vertex_label_t label;
};
using PointList = std::vector< Point >;



/**
 * Calculates a bit mask that indicates whether a point is smaller than
 * a given "pivot" point with respect to each dimension
 */
inline
auto DeterminePartition( vertex_label_t const& point, vertex_label_t const& pivot ) -> partition_mask_t
{
    partition_mask_t partition = 0;

    for( dimension_t d = 0, num_dimensions = point.size(); d < num_dimensions; ++d )
    {
        if( point[ d ] > pivot[ d ] ) 
        {
            partition |= ( 1 << d );
        }
    }
    return partition;
}

/**
 * Determines an ascending sort order for a list of points based on partitioning the data by medians.
 * The first sort key is the number of dimensions on which a given point is less than the median.
 * Ties thereupon are broken by the first dimension in which a give point is less than the median
 * and the point to which it is compared is not. Further times are broken by the smaller L1 norm
 * (i.e., sum) of coordinates. Finally, any remaining ties are broken by the point that has the
 * smaller value in the first dimension on which they differ.
 * 
 * @returns A pair in which the element is a mapping from index to rank in the sort order (i.e.,
 * new index) and the second element is a list of Point structs that have been reordered per that mapping.
 */
auto OrderByPartition( label_list_t const& points ) -> std::pair< index_list_t, PointList >;
