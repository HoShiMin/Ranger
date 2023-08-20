#include "../include/Ranger/Ranger.hpp"
#include <cstdio>

int main()
{
    constexpr auto k_testBitCount = 8ull;
    constexpr auto k_max = (1ull << k_testBitCount) - 1;

    constexpr auto k_addrFrom = 0ull;
    constexpr auto k_addrTo = k_max;

    static_assert(k_addrFrom <= k_addrTo);

    for (auto startAddr = k_addrFrom; startAddr <= k_addrTo; ++startAddr)
    {
        printf("Start addr: %lli\n", startAddr);
        for (auto endAddr = startAddr + 1; endAddr <= k_addrTo; ++endAddr)
        {
            using Type = unsigned long long;
            Ranger::BitRange<Type> range;
            for (auto base = 0ull; base <= k_max; ++base)
            {
                range.setBase(base);
                for (auto mask = 0ull; mask <= k_max; ++mask)
                {
                    range.setMask(mask);
                    bool mustMatch = false;
                    for (auto addr = startAddr; addr <= endAddr; ++addr)
                    {
                        if (range.isMatches(addr))
                        {
                            mustMatch = true;
                            break;
                        }
                    }

                    const bool matchesByTestAlgo = range.isMatches(startAddr, endAddr);
                    if (mustMatch != matchesByTestAlgo)
                    {
                        printf("Failed.\n");
                        __int2c();
                    }
                }
            }
        }
    }

    printf("Ok, press any key to exit.\n");
    return getchar();
}