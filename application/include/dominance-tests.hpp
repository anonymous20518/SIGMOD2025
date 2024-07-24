/**
 * Various dominance test functions for use with skyline operators
 */

#pragma once

#include <cassert>
#include <vector>

#include "spatial.hpp"

using group_t = index_list_t;                         // A group of points, identified by the index of each point
using group_list_t = std::vector< index_list_t >;     // A group of groups

enum class DominanceTestResult
{
    dominates,
    incomparable,
    equal
};

/**
 * Performs one-sided dominance test between two points which are provided as iterator ranges 
 * to determine if the first point dominates the second and returns a DominanceTestResult.
 * Is linear in the distance from first1 to last1.
 */
template < typename ForwardPointInterator >
DominanceTestResult PointDominanceTest( ForwardPointInterator first1
	                                  , ForwardPointInterator last1
	                                  , ForwardPointInterator first2 )
{
    auto const num_dimensions = std::distance( first1, last1 );
    assert( num_dimensions >= 0 );

    std::size_t num_dimensions_equal = 0;
    std::size_t num_dimensions_better_or_equal = 0;

    for( ; first1 != last1; ++first1, ++first2 )
    {
        if( *first1 == *first2 ) { ++num_dimensions_equal; }
        if( *first1 <= *first2 ) { ++num_dimensions_better_or_equal; }
        else { return DominanceTestResult::incomparable; }
    }

    if( num_dimensions_equal == static_cast< std::size_t >( num_dimensions ) )
    {
        // all dimensions are equal so points are equal
        return DominanceTestResult::equal;
    }
    else if( num_dimensions_better_or_equal == static_cast< std::size_t >( num_dimensions ) )
    {
        // point 1 dominates point 2 because it is better or equal in all
        // dimensions *and* the if statement above was false, i.e., there is
        // at least one dimension in which the two points are not equal
        return DominanceTestResult::dominates;
    }

    // this line should be unreachable because of the early break
    // in the for loop
    assert( "Unreachable statement in dominance test" && false );
    return DominanceTestResult::incomparable;
}



/**
 * Performs one-sided dominance test between two groups of points which are provided as iterator ranges 
 * to determine if the first group dominates the second and returns a boolean.
 * Is quadratic in the distance from first1 to last1 and linear in the dimensions of each point, i.e., O(dg^2).
 * 
 * Note that groups are multisets of points. Dominance is defined over the multiset/bag difference of each group,
 * i.e., after removing their bag intersection. Call those subgroup 1 and subgroup 2. Then group 1 dominates
 * group 2 iff all points in subgroup 2 are dominated by points in subgroup 1.
 * 
 * @pre The points within each group should be sorted monotonically such that the i'th point
 * cannot be dominated by some (i+c)'th point for non-negative c.
 */
template < typename ForwardGroupInterator >
bool GroupDominanceTest( ForwardGroupInterator first1
                       , ForwardGroupInterator last1
                       , ForwardGroupInterator first2
                       , ForwardGroupInterator last2
                       , label_list_t const& labels )
{
    assert( std::distance( first1, last1 ) == std::distance( first2, last2 )
         && "We assume groups have to be the same size in this work, following Li et al. SIGMOD 2018.");

    // we don't calculate bag difference explicitly from here. Instead, just mark off points
    // in group 1 that have been matched to a unique but equal point in group 2
    // and skip the marked off points in subsequent loop iterations
    std::vector< bool > equal_to_point_in_group2( std::distance( first1, last1 ), false );
    bool groups_not_equal = false;
    
    for( ; first2 != last2; ++first2 ) // iterate group 2 members
    {
        // if no point dominates this one, then the whole group is not dominated
        // start by setting a flag to false and flip it to true if the point is dominated at some point
        bool isDominatedOrEqual = false;
        for( auto curr1 = first1; curr1 != last1; ++curr1 ) // iterate group 1 members
        {
            if( ! equal_to_point_in_group2[ std::distance( first1, curr1 ) ] ) // skip points not in bag difference
            {
                auto const it_curr_coords_start   = labels[ *curr1 ].cbegin();
                auto const it_curr_coords_end     = labels[ *curr1 ].cend();
                auto const it_first2_coords_start = labels[ *first2 ].cbegin();
                auto dominance_result = PointDominanceTest( it_curr_coords_start, it_curr_coords_end, it_first2_coords_start );
                if( dominance_result == DominanceTestResult::equal )
                {
                    // remove both of these points from subgroup 1 and subgroup 2, i.e., each big difference
                    // by breaking the loop with isDominatedOrEqual = true for point 2
                    // and setting the flag for point 1
                    equal_to_point_in_group2[ std::distance( first1, curr1 ) ] = true;
                    isDominatedOrEqual = true;
                    break;
                }
                else if( dominance_result == DominanceTestResult::dominates )
                {
                    // this point is dominated. no need to compare to anything else now
                    // also mark that there exists at least one point that is different in each group
                    groups_not_equal = true;
                    isDominatedOrEqual = true;
                    break;
                }
            }
        }
        if( ! isDominatedOrEqual )
        {
            // we found a point in subgroup 2 that is not dominated by
            // a point in subgroup 1!
            return false;
        }
    }
    // if we get here, then all points in subgroup 2 were dominated by
    // some point in subgroup 1 or the groups are identical. Return
    // the groups_not_equal variable to differentiate the cases.
    return groups_not_equal;
}

/**
 * Returns true if any group in the skyline dominates the group_to_test
 */
inline
bool IsDominatedBySkyline( group_t const& group_to_test, group_list_t const& skyline, label_list_t const& coordinates )
{
    for(auto const& skylineGroup : skyline )
    {
        if( GroupDominanceTest( std::cbegin( skylineGroup )
                              , std::cend  ( skylineGroup )
                              , std::cbegin( group_to_test )
                              , std::cend  ( group_to_test )
                              , coordinates ) )
        {
            return true;
        }
    }
    return false;
}

inline
bool IsDominatedByCandidates( point_index_t index, group_list_t const& candidates, std::vector< bool > const& bIsSkyline, label_list_t const& coordinates )
{
    auto it_index_start = candidates[ index ].cbegin();
    auto it_index_end   = candidates[ index ].cend();

    for (size_t i = 0; i < static_cast<size_t>(index); i++) // no group can dominate groups preceding it
    {
        auto it_i_start = candidates[ i ].cbegin();
        auto it_i_end   = candidates[ i ].cend();

        if (bIsSkyline[i] && GroupDominanceTest(it_i_start, it_i_end, it_index_start, it_index_end, coordinates))
        {
            return true;
        }
    }
    return false;
}

inline
bool CanTerminate( label_list_t const& skylineRepresentatives, vertex_label_t const& layerRepresentative)
{
    for (const myLabel& skyRep : skylineRepresentatives)
    {
        if( DominanceTestResult::dominates == PointDominanceTest( std::cbegin( skyRep )
                                                                , std::cend  ( skyRep )
                                                                , std::cbegin( layerRepresentative ) ) )
        {
            return true;
        }
    }
    return false;
}
