#pragma once

#include "esp_system.h"

// Block IDs
static uint8_t BLOCK_T_ID = 0;
static uint8_t BLOCK_J_ID = 1;
static uint8_t BLOCK_Z_ID = 2;
static uint8_t BLOCK_O_ID = 3;
static uint8_t BLOCK_S_ID = 4;
static uint8_t BLOCK_L_ID = 5;
static uint8_t BLOCK_I_ID = 6;

// Collision  masks used to save data with each tetro needing only 16 bits
//  compared to 4 sets of xy offsets with 8 bits for each axis (barring some 4 bit type trickery) resulting in 64 bits per tetro
// Storing tetromino layout using a bit matrix means collision detection is a simple matter of ANDing the tetromino bits with a bit matrix of the desired game board position

// Values of 1 indicate an occupied square
// These arent used for drawing in order to optimize the number of write operations to the screen needed to draw the piece
static const uint16_t TETROMINOS[7] = { // store tetrominoes as 16 bits representing a 4x4 matrix
    // T
    0x0072,
        // 0, => 0000
        // 0, => 0000
        // 7, => 0111
        // 2  => 0010
    // J
    0x0071,
        // 0,  => 0000
        // 0,  => 0000
        // 7,  => 0111
        // 1   => 0001
    // Z
    0x0063,
        // 0,  => 0000
        // 0,  => 0000
        // 6,  => 0110
        // 3   => 0011
    // O
    0x0066,
        // 0,  => 0000
        // 0,  => 0000
        // 6,  => 0110
        // 6   => 0110
    // S
    0x0036,
        // 0,  => 0000
        // 0,  => 0000
        // 6,  => 0011
        // 6   => 0110
    // L
    0x0074,
        // 0,  => 0000
        // 0,  => 0000
        // 7,  => 0111
        // 4   => 0100
    // I
    0x00F0
        // 0,  => 0000
        // 0,  => 0000
        // F,  => 1111
        // 0   => 0000
};

// Doing a 3x3 transforms in a 4x4 bit space so had to math this out a bit
// Transform active area mask (top and left row only apply to I which has its own edge handling)
// 0000
// 0111
// 0111
// 0111

#if(0)
// Operations for clockwise transform through cardinal degrees
// deg:     0 =>      1 =>      2 =>      3
// ========================================
//    <<    8   >>    2   >>    8   <<    2
// 0,0 0x0400 => 0x0100 => 0x0001 => 0x0004
//    <<    3   >>    5   >>    3   <<    5
// 1,0 0x0200 => 0x0010 => 0x0002 => 0x0040
//    >>    2   >>    8   <<    2   <<    8
// 2,0 0x0100 => 0x0001 => 0x0004 => 0x0400

//    <<    5   <<    3   >>    5   >>    3
// 0,1 0x0040 => 0x0200 => 0x0010 => 0x0002
//    n/a       n/a       n/a       n/a
// 1,1 0x0020 => 0x0020 => 0x0020 => 0x0020
//    >>    5   >>    3   <<    5   <<    3
// 2,1 0x0010 => 0x0002 => 0x0040 => 0x0200

//    <<    2   <<    8   >>    2   >>    8
// 0,2 0x0004 => 0x0400 => 0x0100 => 0x0001
//    >>    3   <<    5   <<    3   >>    5
// 1,2 0x0002 => 0x0040 => 0x0200 => 0x0010
//    >>    8   <<    2   <<    8   >>    2
// 2,2 0x0001 => 0x0004 => 0x0400 => 0x0100

// Table of coord to mask per rotation. Used to validate the function

// Draft 1, full data transform and store. If right is false, rotate left
static uint16_t rotate(uint16_t src, bool right)
{
    // Check for I block as its transform is an edge case
    // If topmost or leftmost row contains a block in the only possible position,
    //  then it is one of the I transposes, return the opposite transpose (only has 2)
    if(src & 0x2000)
    {
        return 0x00F0;
    }
    if(src & 0x0080)
    {
        return TETROMINOS[BLOCK_I_ID];
    }

    // Otherwise, top row and left col are empty
    // Center/origin tile is always filled
    uint16_t retVal = 0x0020;

    if(right)
    {
        // Set each bit of retval to its 1 deg transpose
        // Top row
        retVal |= (src & 0x0400) >> 2;
        retVal |= (src & 0x0200) >> 5;
        retVal |= (src & 0x0100) >> 8;

        // Mid row
        retVal |= (src & 0x0040) << 3;
        // No need to set center
        retVal |= (src & 0x0010) >> 3;

        // Bottom row
        retVal |= (src & 0x0004) << 2;
        retVal |= (src & 0x0002) << 5;
        retVal |= (src & 0x0001) << 8;
    }
    else
    {
        // Set each bit of retval to its 3 deg transpose 
        // (note we have to reverse the shift direction of what is in the chart above as direction is counter clockwise)
        // Top row
        retVal |= (src & 0x0400) >> 8;
        retVal |= (src & 0x0200) >> 3;
        retVal |= (src & 0x0100) << 2;

        // Mid row
        retVal |= (src & 0x0040) >> 5;
        // No need to set center
        retVal |= (src & 0x0010) << 5;

        // Bottom row
        retVal |= (src & 0x0004) >> 2;
        retVal |= (src & 0x0002) << 3;
        retVal |= (src & 0x0001) << 8;

    }

    return retVal;
}

// Operations transform from degree 0 to the target degree in single operation
// deg:     0 =>      1 =>      2 =>      3
// ========================================
//              >>    2   >>   10   >>    8
// 0,0 0x0400 => 0x0100 => 0x0001 => 0x0004
//              >>    5   >>    8   >>    3
// 1,0 0x0200 => 0x0010 => 0x0002 => 0x0040
//              >>    8   >>    6   <<    2
// 2,0 0x0100 => 0x0001 => 0x0004 => 0x0400

//              <<    3   >>    2   >>    5
// 0,1 0x0040 => 0x0200 => 0x0010 => 0x0002
//    n/a       n/a       n/a       n/a
// 1,1 0x0020 => 0x0020 => 0x0020 => 0x0020
//              >>    3   <<    2   <<    5
// 2,1 0x0010 => 0x0002 => 0x0040 => 0x0200

//              <<    8   <<    6   >>    2
// 0,2 0x0004 => 0x0400 => 0x0100 => 0x0001
//              <<    5   <<    8   <<    3
// 1,2 0x0002 => 0x0040 => 0x0200 => 0x0010
//              <<    2   <<   10   <<    8
// 2,2 0x0001 => 0x0004 => 0x0400 => 0x0100

// Draft 2, on the fly transform from starting state where deg = x % 4 * 90
static uint16_t rotate(uint8_t block_id, uint8_t deg)
{
    // Check for I block as its transform is an edge case
    if(block_id == BLOCK_I_ID)
    {
        // If deg is odd, return the alternate transpose, else return the original (only has 2)
        if(deg % 2)
        {
            return 0x00F0;
        }

        return TETROMINOS[BLOCK_I_ID];
    }

    // Otherwise, top row and left col are empty
    // Center/origin tile is always filled
    uint16_t retVal = 0x0020;
    switch(deg % 4) 
    {
        case 0:
            return TETROMINOS[block_id];
        case 1:
            // Set each bit of retval to its 1 deg transpose
            // Top row
            retVal |= (TETROMINOS[block_id] & 0x0400) >> 2;
            retVal |= (TETROMINOS[block_id] & 0x0200) >> 5;
            retVal |= (TETROMINOS[block_id] & 0x0100) >> 8;

            // Mid row
            retVal |= (TETROMINOS[block_id] & 0x0040) << 3;
            // No need to set center
            retVal |= (TETROMINOS[block_id] & 0x0010) >> 3;

            // Bottom row
            retVal |= (TETROMINOS[block_id] & 0x0004) << 2;
            retVal |= (TETROMINOS[block_id] & 0x0002) << 5;
            retVal |= (TETROMINOS[block_id] & 0x0001) << 8;
            return retVal;
        case 2:
            // Set each bit of retval to its 2 deg transpose
            // Top row
            retVal |= (TETROMINOS[block_id] & 0x0400) >> 10;
            retVal |= (TETROMINOS[block_id] & 0x0200) >> 8;
            retVal |= (TETROMINOS[block_id] & 0x0100) >> 6;

            // Mid row
            retVal |= (TETROMINOS[block_id] & 0x0040) >> 2;
            // No need to set center
            retVal |= (TETROMINOS[block_id] & 0x0010) << 2;

            // Bottom row
            retVal |= (TETROMINOS[block_id] & 0x0004) << 6;
            retVal |= (TETROMINOS[block_id] & 0x0002) << 8;
            retVal |= (TETROMINOS[block_id] & 0x0001) << 10;
            return retVal;
        case 3:
            // Set each bit of retval to its 3 deg transpose 
            // Top row
            retVal |= (TETROMINOS[block_id] & 0x0400) >> 8;
            retVal |= (TETROMINOS[block_id] & 0x0200) >> 3;
            retVal |= (TETROMINOS[block_id] & 0x0100) << 2;

            // Mid row
            retVal |= (TETROMINOS[block_id] & 0x0040) >> 5;
            // No need to set center
            retVal |= (TETROMINOS[block_id] & 0x0010) << 5;

            // Bottom row
            retVal |= (TETROMINOS[block_id] & 0x0004) >> 2;
            retVal |= (TETROMINOS[block_id] & 0x0002) << 3;
            retVal |= (TETROMINOS[block_id] & 0x0001) << 8;
            return retVal;

    }
}
#endif

// Operations transform from degree 0 to the target degree in single operation
// deg:     0 =>      1 =>      2 =>      3
// ========================================
//              >>    2   >>   10   >>    8
// 0,0 0x0400 => 0x0100 => 0x0001 => 0x0004
//              >>    5   >>    8   >>    3
// 1,0 0x0200 => 0x0010 => 0x0002 => 0x0040
//              >>    8   >>    6   <<    2
// 2,0 0x0100 => 0x0001 => 0x0004 => 0x0400

//              <<    3   >>    2   >>    5
// 0,1 0x0040 => 0x0200 => 0x0010 => 0x0002
//    n/a       n/a       n/a       n/a
// 1,1 0x0020 => 0x0020 => 0x0020 => 0x0020
//              >>    3   <<    2   <<    5
// 2,1 0x0010 => 0x0002 => 0x0040 => 0x0200

//              <<    8   <<    6   >>    2
// 0,2 0x0004 => 0x0400 => 0x0100 => 0x0001
//              <<    5   <<    8   <<    3
// 1,2 0x0002 => 0x0040 => 0x0200 => 0x0010
//              <<    2   <<   10   <<    8
// 2,2 0x0001 => 0x0004 => 0x0400 => 0x0100

static uint16_t getRotation(uint8_t block_id, uint8_t rot)
{
    // Check for O block as it has no rotation
    if(block_id == BLOCK_O_ID)
        return TETROMINOS[BLOCK_O_ID];
    // Check for I block as its transform is an edge case
    if(block_id == BLOCK_I_ID)
    {
        // If rot is odd, return the alternate transpose, else return the original (only has 2)
        if(rot & 1)
        {
            // This is the only static non-spawn rotation. All other rotations are covered dynamically by the logic below
            return 0x2222;
        }

        return TETROMINOS[BLOCK_I_ID];
    }

    // Otherwise, top row and left col are empty
    const uint8_t deg = (block_id == BLOCK_Z_ID || block_id == BLOCK_S_ID) ? ((rot & 1) * 3) : rot % 4; // Limit S and Z to only 2 rotations, 0 and 3 deg
    switch(deg) 
    {
        case 0:
            return TETROMINOS[block_id];
        case 1:
            // Set each bit of retval to its 1 deg transpose
            return 0x0020 // Center/origin tile is always filled
                // Top row
                | (TETROMINOS[block_id] & 0x0400) >> 2
                | (TETROMINOS[block_id] & 0x0200) >> 5
                | (TETROMINOS[block_id] & 0x0100) >> 8

                // Mid row
                | (TETROMINOS[block_id] & 0x0040) << 3
                // No need to set center
                | (TETROMINOS[block_id] & 0x0010) >> 3

                // Bottom row
                | (TETROMINOS[block_id] & 0x0004) << 8
                | (TETROMINOS[block_id] & 0x0002) << 5
                | (TETROMINOS[block_id] & 0x0001) << 2;
        case 2:
            // Set each bit of retval to its 2 deg transpose
            return 0x0020 // Center/origin tile is always filled
                // Top row
                | (TETROMINOS[block_id] & 0x0400) >> 10
                | (TETROMINOS[block_id] & 0x0200) >> 8
                | (TETROMINOS[block_id] & 0x0100) >> 6

                // Mid row
                | (TETROMINOS[block_id] & 0x0040) >> 2
                // No need to set center
                | (TETROMINOS[block_id] & 0x0010) << 2

                // Bottom row
                | (TETROMINOS[block_id] & 0x0004) << 6
                | (TETROMINOS[block_id] & 0x0002) << 8
                | (TETROMINOS[block_id] & 0x0001) << 10;
        case 3:
            // Set each bit of retval to its 3 deg transpose 
            return 0x0020 // Center/origin tile is always filled
                // Top row
                | (TETROMINOS[block_id] & 0x0400) >> 8
                | (TETROMINOS[block_id] & 0x0200) >> 3
                | (TETROMINOS[block_id] & 0x0100) << 2

                // Mid row
                | (TETROMINOS[block_id] & 0x0040) >> 5
                // No need to set center
                | (TETROMINOS[block_id] & 0x0010) << 5

                // Bottom row
                | (TETROMINOS[block_id] & 0x0004) >> 2
                | (TETROMINOS[block_id] & 0x0002) << 3
                | (TETROMINOS[block_id] & 0x0001) << 8;
    }
    // Normally never executes
    return 0;
}