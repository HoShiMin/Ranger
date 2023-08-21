# 📐 Ranger
## The sparsed range intersector.
A sparsed range is a bitmap that describes several ranges using two values: `Base` and `Mask`.  
`Base` is a set of ones and zeroes, and `Mask` is a bitmap of meaningful bits in the base.  
If a bit in the mask is 0 - the corresponding bit at the same position is treated as "any bit", and if the bit is 1, it means the the corresponding bit must be the same as in the base at the same position.

Example:
```
0110'0100 Base
0111'1100 Mask
--------------
?110'01?? Range

'?' may be any bit.
```

### One shot check.
We can check whether an arbitrary value intersects the range using this function:
```cpp
bool intersects(auto base, auto mask, auto value)
{
    return (value & mask) == (base & mask);
}
```

The function above ensures that all meaningful bits of the range are the same in the given value:
```
?110'01?? Range
^      ^^
May be either 0 or 1 in any combination.

Sample values:
1110'0100 Matches
0110'0111 Matches too
^      ^^
These bits are ignored.

0010'0111 Doesn't match!
 ^
 This bit must be 1 as it's a meaningful bit
 and must be the same as in the range.
```

### Range check.
Let's complicate the task. Now we need to check not a single value, but a range of values `[Begin..End]`, and we must check this in a performant manner.

This project presents the following algorithm:

0. Suppose we have these initial conditions:
    ```
    0010'0000 Base
    1010'0100 Mask
    --------------
    0?1?'?0?? Range

    1000'1101 Begin
    1010'0001 End

    Whether any value in the [Begin..End] belongs to the given range?
    ```
1. Determine the volatile bits of the `[Begin..End]`:
    ```
    1000'1101 Begin
    1010'0001 End
    10xx'xxxx
      -------
       Volatile bits
    ```
2. Drop the volatile bits and check the constant part only:
    ```
    ?0|1?'?0??
    10|xx'xxxx The constant part from the previous step
    ^^
    Check the constant part only.
    ```
    If the constant part doesn't match, the whole range is apriory mismatched and there is no point in continuing.  
    Otherwise go to the next step.
3. Trim the constant part as it was already checked and we don't need to check it anymore:
    ```
    00|1?'?0?? Range
    00|00'1101 Begin
    00|10'0001 End
    ^^
    Reset the constant part.
    ```
4. So, we have these ranges:
   ```
   1?'?0?? Range

   00'1101 Begin
   10'0001 End
   ```
   In the next steps we will work with the range only.  
   We should find such combination of bits instead of "?" so that the resulting value falls into the `[Begin..End]` if possible.  
   
   We will use the following strategy:

   1. Select the highest "any-bit" (marked as "?") and set it to 1 and reset all lower any-bits:
        ```
        1?'?0?? -> 11'0000
        ^ ^ ^^
        1 0 00
        ```
        We've got the probe value.
   2. We need to understand where the probe value is in relation to the range.  
        It can be in three places:  
        - To the left of the `Begin`. In this case we can increase the value more by setting lower "any-bits" to ones.Fix the highest "any-bit" as one and repeat all these steps for the next "any-bit".
        - To the right of the `End`. In this case we can't lower the probe value by the lower "any-bits" as they'realready zeroes. It means that the only case is to reset the highest "any-bit" to zero, set the lower "any-bits" to ones and try again:
            ```
            1?'?0?? -> 10'1011
            ^ ^ ^^
            0 1 11
            ```
        Repeat comparation with this probe value. If it will be lower than the `Begin` - it is impossible to increaseit more and it means that ranges don't intersect. Otherwise fix the highest "any-bit" as zero and repeat allthese steps for the next "any-bit".
        - Inside the `[Begin..End]` - in this case we can end our search as at least one value from the `[Begin..End]`falls withing the sparsed range.
      

      

All these steps are implemented in this library.

### Usage:
Just include the `include/Ranger/Ranger.hpp` and you're good to go!
```cpp
#include <Ranger/Ranger.hpp>
#include <cassert>

int main()
{
    // Create the sparsed range:
    auto range = Ranger::BitRange<uint8_t>::make("011?'??10");
    assert(range.isMatches(0b01101010));
    assert(range.isMatches(0b01110110));

    //
    // [0010'0000..1000'0000]
    //    Begin       End
    //
    assert(range.isMatches(0b00100000, 0b10000000));
    
    // Set the base and the mask manually:
    range.setBase(0b0110'1000);
    range.setMask(0b0111'1100);

    // Format the sparsed range to the string:
    char formattedRange[sizeof("xxxxxxxx")]{};
    range.format(formattedRange);
    assert(strcmp(formattedRange, "?11010??") == 0);

    return 0;
}
```