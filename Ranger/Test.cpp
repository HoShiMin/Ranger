#include "../include/Ranger/Ranger.hpp"

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <bitset>

#define assert_true(cond)  if (!(cond)) { __int2c(); }
#define assert_false(cond) if (cond) { __int2c(); }
#define raise_assert() { __int2c(); }

template <typename BaseType>
bool intersectsByEnum(BaseType begin, BaseType end, const Ranger::BitRange<BaseType> range)
{
    constexpr BaseType k_max = static_cast<BaseType>(~0);

    for (BaseType value = begin; value <= end; ++value)
    {
        if (range.intersects(value))
        {
            return true;
        }

        if (value == k_max)
        {
            break;
        }
    }

    return false;
}

template <typename BaseType>
void enumAllCombinations()
{
    constexpr BaseType k_max = static_cast<BaseType>(~0);

    for (BaseType begin = 0; begin <= k_max; ++begin)
    {
        std::cout << std::setfill('0') << std::setw(sizeof(BaseType) * 8) << std::bitset<sizeof(BaseType) * 8>(begin) << " Begin" << std::endl;
        for (BaseType end = begin + 1; end <= k_max; ++end)
        {
            Ranger::BitRange<BaseType> range;
            for (BaseType base = 0; base <= k_max; ++base)
            {
                range.setBase(base);
                for (BaseType mask = 0; mask <= k_max; ++mask)
                {
                    range.setMask(mask);
                    const bool mustMatch = intersectsByEnum(begin, end, range);
                    const bool matchesByRanger = range.intersects(begin, end);
                    if (matchesByRanger != mustMatch) {
                        std::cout << "==========================" << std::endl;
                        std::cout << "Assertion failure." << std::endl;
                        std::cout << "Must match: " << mustMatch << std::endl;
                        std::cout << "Matches by Ranger: " << matchesByRanger << std::endl;
                        std::cout << std::endl;
                        std::cout << std::setfill('0') << std::setw(sizeof(BaseType) * 8) << std::bitset<sizeof(BaseType) * 8>(base)  << " Base" << std::endl;
                        std::cout << std::setfill('0') << std::setw(sizeof(BaseType) * 8) << std::bitset<sizeof(BaseType) * 8>(mask)  << " Mask" << std::endl;
                        char formattedRange[65]{};
                        range.format(formattedRange);
                        std::cout << formattedRange << " Range" << std::endl;
                        std::cout << std::endl;
                        std::cout << std::setfill('0') << std::setw(sizeof(BaseType) * 8) << std::bitset<sizeof(BaseType) * 8>(begin) << " Begin" << std::endl;
                        std::cout << std::setfill('0') << std::setw(sizeof(BaseType) * 8) << std::bitset<sizeof(BaseType) * 8>(end)   << " End" << std::endl;
                        raise_assert();
                    }

                    if (mask == k_max)
                    {
                        break;
                    }
                }

                if (base == k_max)
                {
                    break;
                }
            }

            if (end == k_max)
            {
                break;
            }
        }

        if (begin == k_max)
        {
            break;
        }
    }
}

int main()
{
    {
        const auto range = Ranger::BitRange<uint8_t>::make("011?'??10");
        
        assert_true(range.base() == 0b0110'0010);
        assert_true(range.mask() == 0b1110'0011);

        assert_true(range.intersects(0b0110'1010));
        assert_true(range.intersects(0b0111'0110));
        assert_false(range.intersects(0b11111110));

        assert_true(range.intersects(0b00100000, 0b10000000));
        assert_false(range.intersects(0b10000000, 0b11111111));

        char formattedRange[9]{};
        range.format(formattedRange);
        assert_true(strcmp(formattedRange, "011???10") == 0);
    }

    {
        const auto range = Ranger::BitRange<uint64_t>::make("1?101000_100???01_1101????_011???10");

        assert_true(range.base() == 0b10101000'10000001'11010000'01100010);
        assert_true(range.mask() == 0b10111111'11100011'11110000'11100011);

        assert_true(range.intersects(0b11101000'10010101'11011100'01110110));
        assert_false(range.intersects(0b01101000'10010101'11011100'01110110));
    }

    enumAllCombinations<unsigned char>();

    printf("Ok, press any key to exit.\n");
    return getchar();
}