#pragma once

#include <intrin.h>



namespace Ranger
{



template <typename BaseType>
class BitRange
{
private:
    class ChangedBits
    {
    private:
        BaseType m_mask;
        unsigned char m_count;

    public:
        ChangedBits() noexcept
            : m_mask(0)
            , m_count(0)
        {
        }

        explicit ChangedBits(unsigned char numberOfChangedBits) noexcept
            : m_mask((static_cast<BaseType>(1) << numberOfChangedBits) - 1)
            , m_count(numberOfChangedBits)
        {
        }

        unsigned char count() const noexcept
        {
            return m_count;
        }

        BaseType mask() const noexcept
        {
            return m_mask;
        }
    };

private:
    static BaseType setByMask(BaseType value, BaseType mask) noexcept
    {
        return value | mask;
    }

    static BaseType resetByMask(BaseType value, BaseType mask) noexcept
    {
        return value & (~mask);
    }

    static unsigned char findHighestSettedBit(BaseType value) noexcept
    {
        unsigned long highestSettedBit = 0;
        
        if constexpr (sizeof(BaseType) == sizeof(unsigned long long))
        {
            _BitScanReverse64(&highestSettedBit, value);
        }
        else
        {
            _BitScanReverse(&highestSettedBit, value);
        }

        return static_cast<unsigned char>(highestSettedBit);
    }

    static ChangedBits findChangedBits(BaseType low, BaseType high) noexcept
    {
        const BaseType xored = low ^ high;
        if (!xored)
        {
            return {}; // There are no changing bits
        }

        // Find the most significant bit (bit scan reverse):
        const auto highestSettedBit = findHighestSettedBit(xored);
        return ChangedBits(highestSettedBit + 1);
    }

    static bool isInRange(BaseType value, BaseType begin, BaseType end) noexcept
    {
        return (value >= begin) && (value <= end);
    }

private:
    BaseType m_base;
    BaseType m_mask;
    BaseType m_baseAndMask;

public:
    //
    // Creates a bit range from a given string.
    // Example: "11??'001?"
    // Supported delimiters: " ", "_" and "'".
    // Drops all bits that are greater than the highest bit of the BaseType.
    //
    template <size_t size>
    static constexpr BitRange make(const char(&mask)[size]) noexcept
    {
        constexpr BaseType k_bitCount = sizeof(BaseType) * 8;

        BitRange bitMask{};
        unsigned char bitNumber = 0;
        for (auto i = size - sizeof('\0'); i > 0; --i)
        {
            if (bitNumber > (k_bitCount - 1))
            {
                break;
            }

            switch (mask[i - 1])
            {
            case '0':
            {
                bitMask.m_base &= ~(BaseType(1) << BaseType(bitNumber));
                bitMask.m_mask |= BaseType(1) << BaseType(bitNumber);
                break;
            }
            case '1':
            {
                bitMask.m_base |= BaseType(1) << BaseType(bitNumber);
                bitMask.m_mask |= BaseType(1) << BaseType(bitNumber);
                break;
            }
            case '?':
            {
                bitMask.m_base &= ~(BaseType(1) << BaseType(bitNumber));
                bitMask.m_mask &= ~(BaseType(1) << BaseType(bitNumber));
                break;
            }
            case '\'':
            case ' ':
            case '_':
            {
                continue;
            }
            }

            ++bitNumber;
        }

        bitMask.m_baseAndMask = bitMask.m_base & bitMask.m_mask;
        return bitMask;
    }

    //
    // Performs converting to a human-readable format ("0?11??00" without delimiters).
    //
    template <size_t size = sizeof(BaseType) * 8 + sizeof('\0')>
    void format(char(&str)[size]) const noexcept
    {
        static_assert(size >= (sizeof(BaseType) * 8 + sizeof('\0')), "The given buffer is too small to format this bitrange.");

        unsigned char pos = 0;
        for (BaseType bit = static_cast<BaseType>(1) << ((sizeof(BaseType) * 8) - 1); bit != 0; bit >>= 1)
        {
            str[pos] = (mask() & bit) ? ((base() & bit) ? '1' : '0') : '?';
            ++pos;
        }
        str[sizeof(BaseType) * 8] = '\0';
    }

    constexpr BitRange() noexcept
        : m_base(0)
        , m_mask(0)
        , m_baseAndMask(0)
    {
    }

    constexpr BitRange(BaseType base, BaseType mask) noexcept
        : m_base(base)
        , m_mask(mask)
        , m_baseAndMask(base& mask)
    {
    }

    constexpr BitRange& setBase(BaseType base) noexcept
    {
        m_base = base;
        m_baseAndMask = m_base & m_mask;
        return *this;
    }

    constexpr BitRange& setMask(BaseType mask) noexcept
    {
        m_mask = mask;
        m_baseAndMask = m_base & m_mask;
        return *this;
    }

    constexpr BaseType base() const noexcept
    {
        return m_base;
    }

    constexpr BaseType mask() const noexcept
    {
        return m_mask;
    }

    constexpr BaseType baseAndMask() const noexcept
    {
        return m_baseAndMask;
    }

    //
    // Checks whether the value intersects with this range.
    // 
    // Examples:
    // ??01'10?0 Range
    // 0101'1010 Matches
    // 1001'1000 Matches
    // 1010'1000 Doesn't match
    //
    constexpr bool intersects(BaseType value) const noexcept
    {
        return (value & mask()) == baseAndMask();
    }

    //
    // Checks whether at least one value from the [begin..end] intersects with this range.
    // 
    // Example:
    // 1?01'1??? Range
    // 1000'0010 Begin
    // 1111'0000 End
    // Matches as there are many values in the [begin..end] that are matches the given range.
    // For example, these values:
    // 1001'1000..1001'1111
    // 1101'1000..1101'1111
    //
    constexpr bool intersects(BaseType begin, BaseType end) const noexcept
    {
        if (begin == end)
        {
            return intersects(begin);
        }

        //
        // Check the constant part of [Begin..End].
        // 
        // ?10|0'??10 Range
        // 
        // 010|0'1001 Begin
        // 010|1'0001 End
        // 010|x'xxxx
        //  \     \
        //   \    Volatile part
        //   Constant part
        // 
        // Check the constant part with the range:
        // ?10|... Range part with the same length as the constant part
        // 010|... constant part
        //
        const ChangedBits changedBits = findChangedBits(begin, end);
        if (((begin & mask()) >> changedBits.count()) != (baseAndMask() >> changedBits.count()))
        {
            // It's guaranteed that the range doesn't match.
            return false;
        }

        //
        // Reduce the whole range to the length of the volatile part:
        // 000|0'1001 Begin
        // 000|1'0001 End
        // 000|?'??10 Range
        //  \     \
        //   \    Reduced part
        //   Reset the constant part
        //
        const BaseType reducedBegin = begin & changedBits.mask();
        const BaseType reducedEnd = end & changedBits.mask();
        const BitRange<BaseType> reducedRange(base() & changedBits.mask(), mask() & changedBits.mask());

        //
        // Try to find the value from the range mask that is belongs to [Begin..End].
        //
        BaseType anyBitMask = (~reducedRange.mask()) & changedBits.mask(); // Mask representing position of any bits
        if (!anyBitMask)
        {
            return isInRange(reducedRange.base(), reducedBegin, reducedEnd);
        }

        //
        // Enum each "any" bit in the range and choose the exact value for it.
        //
        BaseType probeMask = reducedRange.baseAndMask();
        for (BaseType probingBit = static_cast<BaseType>(1) << findHighestSettedBit(anyBitMask); anyBitMask != 0; probingBit >>= 1)
        {
            if ((anyBitMask & probingBit) == 0)
            {
                // It's not an "any" bit.
                continue;
            }

            const BaseType rightProbe = resetByMask(probeMask, anyBitMask) | probingBit;
            if (isInRange(rightProbe, reducedBegin, reducedEnd))
            {
                return true;
            }

            if (rightProbe < reducedBegin)
            {
                // Keep this bit setted:
                probeMask |= probingBit;
                anyBitMask ^= probingBit;
                continue;
            }

            //
            // Here leftProbe > end, we need to try the left-side probe.
            //

            const BaseType leftProbe = setByMask(probeMask, anyBitMask) ^ probingBit;
            if (isInRange(leftProbe, reducedBegin, reducedEnd))
            {
                return true;
            }

            if (leftProbe < reducedBegin)
            {
                return false;
            }

            // Keep this bit resetted:
            probeMask &= ~probingBit;
            anyBitMask ^= probingBit;
        }

        return false;
    }
};



} // namespace Ranger
