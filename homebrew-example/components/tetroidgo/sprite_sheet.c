#include "sprite_sheet.h"

#include "esp_system.h"

// Get coordinate for sprite of single digit
sheet_coord getNumCoord(uint8_t number)
{
    return (sheet_coord){ number % 8, number / 8};
}

sheet_coord getCharCoord(char character)
{
    if(character == '-')
        return (sheet_coord){4, 4};
    if(character > '`') // char value 97, a = 98.
        character -= '`';
    if(character > 'Z' || character < 'A')
    {
        printf("Invalid character %c", character);
        abort();
    }
    return getNumCoord((uint8_t)character - 55);
}

sheet_coord getTetrominoCoord(uint8_t tetromino_id, bool rotated, short offset)
{
    assert(tetromino_id < 7);
    // rotated and offset are only used for I piece
    if(tetromino_id == 6)
    {
        // offset indicates which part of the I to get, -1 means bottom or left side, 0 means middle, 1 means top or right side
        // piece spawns in flat so we consider vertical to be rotated
        return (sheet_coord){ 6 + offset, rotated ? 4 : 6 };
    }

    return (sheet_coord){ 1 + tetromino_id, 5 };
}

uint8_t getTetrominoTileNum(uint8_t tetromino_id, bool rotated, short offset)
{
    assert(tetromino_id < 7);
    // rotated and offset are only used for I piece
    if(tetromino_id == 6)
    {
        // offset indicates which part of the I to get, -1 means bottom or left side, 0 means middle, 1 means top or right side
        // piece spawns in flat so we consider vertical to be rotated
        return (rotated ? 38: 54) + offset;
    }

    return 41 + tetromino_id;
}

uint16_t getSpriteRow(sheet_coord target_sprite, uint8_t pixelRow)
{
    return (TILE_WIDTH * target_sprite.y + pixelRow) * SHEET_WIDTH + target_sprite.x * TILE_WIDTH ;
}

uint16_t getSpriteRowTileNum(uint8_t tileNum, uint8_t pixelRow)
{
    return ((tileNum / TILES_PER_ROW * TILE_WIDTH  + pixelRow)* SHEET_WIDTH) + (tileNum % TILES_PER_ROW * TILE_WIDTH);
}