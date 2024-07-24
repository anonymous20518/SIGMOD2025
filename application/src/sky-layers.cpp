#include <iostream>

#include "dominance-tests.hpp"
#include "sky-layers.hpp"

namespace { // anonymous

struct LMInfo
{
    int firstPos; // position of the first point in layer l and mask m
    int lastPos; // position of the last point in layer l and mask m
    int size; // number of points in layer l and mask m
};

bool AreIncomparable(int mask1, int mask2)
{
    return ((__builtin_popcount(mask1) >= __builtin_popcount(mask2)) && (mask1 != mask2)) || ((mask1 & mask2) < mask1);
}



void UpdateSkyLayers(std::vector<std::vector<Point>>& skyLayers, std::vector<std::vector<LMInfo>>& lmInfo, const Point& newPoint, int maxMask, int layerNumber)
{
    if (static_cast< std::size_t >( layerNumber ) == skyLayers.size()) // adding a new layer
    { 
        lmInfo.push_back(std::vector<LMInfo>(maxMask+1));
        lmInfo.back()[newPoint.mask].size = 1;
        skyLayers.push_back({newPoint});
        skyLayers.back().back().mask = 0; // modifying the mask of the new added point
    }

    else  // updating an existing layers
    {
        LMInfo& info = lmInfo[layerNumber][newPoint.mask];
        const int position = skyLayers[layerNumber].size();
        skyLayers[layerNumber].push_back(newPoint);
        info.lastPos = position;
        if (info.size++ == 0)  // size += 1
        {
            info.firstPos = position;
            skyLayers[layerNumber].back().mask = 0; // modifying the mask of the new added point
        }
        else
        {
            skyLayers[layerNumber].back().mask = DeterminePartition(newPoint.label, (skyLayers[layerNumber][info.firstPos]).label); // modifying the mask of the new added point
        }
    }
}

} // namespace anonymous


auto GenerateSkyLayers( PointList const& points ) -> SkyLayers
{
    SkyLayers skyLayers;
    SkyLayers skyLayers_no_duplicates;

    // Information about each mask (positions of starting and ending points) in each layer
    std::vector<std::vector<LMInfo>> lmInfo_no_duplicates;

    partition_mask_t const maxMask = (2 << ( points[0].label.size() - 1 )) - 1;

    for (auto const& currentPoint : points)
    {
        // Layer number of the current point; to-be-updated
        int layerNumber = 0;

        // This initializations are useful for the first point only.
        // Set it as dominated so that its layer number is set correctly.
        // Set it as not equal to make sure we update the no-duplicate sky layer structure.
        bool bIsDominated = true;  
        bool bIsEqual = false;

        for (std::size_t j = 0; j < skyLayers_no_duplicates.size(); j++)
        {  
            bIsDominated = false;
            for ( partition_mask_t m = 0; m <= currentPoint.mask; m++)
            {
                LMInfo const& info = lmInfo_no_duplicates[j][m];
                if (info.size == 0)
                {
                    continue;
                }

                if (!AreIncomparable(m, currentPoint.mask))
                {
                    auto const tmpMask = DeterminePartition(currentPoint.label, skyLayers_no_duplicates[j][info.firstPos].label);
                    bIsDominated = (tmpMask == maxMask);
                    if (bIsDominated)
                    {
                        break;
                    }
                    for (int p = info.firstPos; p <= info.lastPos; p++){
                        Point const& comparePoint = skyLayers_no_duplicates[j][p];
                        if (!AreIncomparable(comparePoint.mask, tmpMask))
                        {
                            auto const dt_result = PointDominanceTest(comparePoint.label.cbegin(), comparePoint.label.cend(), currentPoint.label.cbegin());
                            bIsDominated = ( dt_result  == DominanceTestResult::dominates ); 
                            bIsEqual = ( dt_result  == DominanceTestResult::equal ); 
                            if ( bIsDominated || bIsEqual )
                            {
                                break;
                            }
                        }
                    }
                    if ( bIsDominated || bIsEqual )
                    {
                        break; 
                    }
                }
            }
            if ( !bIsDominated || bIsEqual )
            {
                layerNumber = j;
                skyLayers[j].push_back(currentPoint);
                break;
            }
        }
        if ( bIsDominated )
        {
            layerNumber = skyLayers_no_duplicates.size();
            skyLayers.push_back({currentPoint});
        }
        if( ! bIsEqual )
        {
            UpdateSkyLayers(skyLayers_no_duplicates, lmInfo_no_duplicates, currentPoint, maxMask, layerNumber);
        }
    }

    return skyLayers;
}
