#pragma once

#include "dominance-tests.hpp"

/**
 * Postprocesses a skyline result to remove false positives by copying over top of them
 * in place and returning an iterator to the end of the valid list of results.
 * Specifically to manage edge case where groups begin with vertices that are distinct
 * but coincident, which, due to the connectivity constraint, can break down the sort
 * order assumption that justifies one-sided dominance tests.
 * 
 * @pre Assumes begin < end
 * @pre Assumes container content is sorted such that groups that have the same first
 * member appear contiguously
 */
template < typename ForwardIterator, typename PointList >
ForwardIterator postprocess( ForwardIterator first, ForwardIterator last, PointList const& labels )
{
    // Three pointer algorithm. Pointer first indicates the end of the range
    // of already confirmed values, i.e., the destination to which should be writen
    // newly confirmed groups.
    // Pointer curr indicates the the group currently being processed, i.e., the next
    // one after start that could appear in the solution
    // Finally, pointer next indicates the next group to compare against curr in order
    // to validate it.
    // Algorithm moves curr from first to last to process all input groups, copying to
    // start++ those that are not dominated. For each group that is processed, pointer
    // next iterates through all groups that might dominate it.
    for( ForwardIterator curr = first; curr != last; ++curr )
    {
        ForwardIterator next = curr + 1;
        bool isSkyline = true;

        while( next != last )
        {
            int const next_first_node_id = *( next->cbegin() );
            int const curr_first_node_id = *( curr->cbegin() );
            auto const dt_result = PointDominanceTest( labels[ next_first_node_id ].cbegin()
                                                     , labels[ next_first_node_id ].cend()
                                                     , labels[ curr_first_node_id ].cbegin() );

            if( dt_result != DominanceTestResult::equal )
            {
                break;
            }
            else if( GroupDominanceTest( next->cbegin(), next->cend(), curr->cbegin(), curr->cend(), labels ) )
            {
                isSkyline = false;
                break;
            }
            else
            {
                ++next;
            }
        }
        if( isSkyline )
        {
            if( first != curr ) // skip unnecessary copy if source == destination
            {
                *first = *curr;
            }
            ++first;
        }
    }

    return first;
}
