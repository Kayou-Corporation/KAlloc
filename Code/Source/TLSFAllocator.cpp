#include "TLSFAllocator.hpp"

#include <algorithm>
#include <bit>


namespace Kayou::Memory
{
    void TLSFAllocator::Mapping(std::size_t size, std::size_t& fli, std::size_t& sli) const
    {
        const std::size_t adjustedSize = std::max(size, minBlockSize);
        const std::size_t highestBit = std::bit_width(adjustedSize) - 1;

        if (highestBit < 4)
        {
            fli = 0;
            sli = adjustedSize / (minBlockSize / SLICount);

            if (sli >= SLICount)
                sli = SLICount - 1;

            return;
        }

        fli = highestBit - 4;

        const std::size_t base = std::size_t{ 1 } << highestBit;
        const std::size_t offset = adjustedSize - base;
        const std::size_t sliceSize = base / SLICount;

        sli = offset / sliceSize;

        if (fli >= FLICount)
            fli = FLICount - 1;

        if (sli >= SLICount)
            sli = SLICount - 1;
    }
}
