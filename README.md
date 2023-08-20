# 📐 Ranger
## The sparsed range intersector.
A sparsed range is a bitmap that describes several ranges using two values: base and mask.  
Base is a set of ones and zeroes, and mask is a bitmap of meaningful bits in the base.  
If a bit in the mask is 0 - the corresponding bit at the same position is treated as "any bit", and if the bit is 1, it means the the corresponding bit must be the same as in the base at the same position.

Example:
```
0110'0100 Base
0111'1100 Mask
--------------
?110'01?? Range

'?' may be any bit.
```

We can check whether a value matches the range using ANDing with mask:
```cpp
bool isMatches(auto base, auto mask, auto value)
{
    return (value & mask) == (base & mask);
}
```

Example:
```
?110'01?? Range
^      ^^
May be either 0 or 1 in any combination.

1110'0100 Matches
0110'0111 Matches too

0010'0111 Doesn't match!
 ^
 This bit must be 1
```

But what if we want check whether a given contiguous range `[Begin..End]` intersects with a sparsed range and want do it in a performant manner?

It could be achieved using the next steps:
1. Determine the volatile bits of the `[Begin..End]`:
    ```
    1000'1101 Begin
    1110'0001 End
    11xx'xxxx
      -------
      Volatile bits
    ```
2. Check the constant part: does it match the sparsed range?
    ```
    ?1|10'01??
    11|xx'xxxx
    ^^
    Check only this constant part
    ```
    If the constant part doesn't match, the whole range is apriory mismatched and there is no point in continuing.
3. Trim the constant part:
    ```
    00|10'01?? Range
    00|00'1101 Begin
    00|10'0001 End
    ^^
    Reset the constant part
    ```
4. Now we should try to find such value of "any" bits in the range that either fit into the `[Begin..End]` or will be outside that tells us the the whole range doesn't match the sparsed range. We can do it from the highest "any" bit to the lowest one. We set the highest "any" bit to 1 and reset lower "any" bits to zero and compare this value with the `End` bound. If the value is greater than `End`, we should invert values of "any" bits and compare it with the `Begin` bound. If it will be less than the `Begin` - ranges don't intersect. Otherwise "fix" the highest "any" bit and perform these steps for the next "any" bit until the value fits the `[Begin..End]` range or will be outside of it that tells us that these range don't intersect.

All these steps are implemented in this library.

### Usage:
Just include the `include/Ranger/Ranger.hpp` and you're good to go!
```cpp
#include <Ranger/Ranger.hpp>
#include <cassert>

int main()
{
    const auto range = Ranger::BitRange<uint8_t>::make("011?'??10");
    assert(range.isMatches(0b01101010));
    assert(range.isMatches(0b01110110));
    assert(range.isMatches(0b00100000, 0b10000000));

    return 0;
}
```