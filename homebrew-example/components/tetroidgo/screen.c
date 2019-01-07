#include "screen.h"

#include "esp_system.h"
#include "esp_event.h"

#include "../odroid/odroid_display.h"

#include "random.h"
#include "sprite_sheet.h"
#include "tetrominoes.h"

#include <string.h>

uint16_t *tileRowBuffer;

#define SCREEN_WIDTH (320)
#define SCREEN_HEIGHT (240)

void drawTitle() // Draws title 'art' to the ili screen
{
    // Draw background bricks
    // Allocate buffer for single tile row
    // TODO: make this utilize send_continue_line directly
    tileRowBuffer = heap_caps_malloc(SCREEN_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawTitle: tileRowBuffer=%p\n", tileRowBuffer);

    // For each row of pixels in the 12x12 brick pattern tile
    for(uint8_t y = 0; y < TILE_WIDTH; ++y)
    {
        // Screen width is not evenly divisible by 12 so we have to draw 1/3rd blocks at the left and right sides
        // Draw left fringe to left side of buffer
        memcpy(
            // Copy to the left side of the current pixel row
            tileRowBuffer + (y * SCREEN_WIDTH), 
            // Reference the current sprite row. This is a fringe tile so we only need 4 pixels
            (uint16_t *)&SPRITE_SHEET + (getSpriteRow(BLOCK_SHINE_1, y) + 8),
            4 * sizeof(uint16_t)
        );
        
        // Draw as many whole blocks as will fit evenly
        for(uint8_t tile_x = 0; tile_x < 26; ++tile_x)
        {
            memcpy(
                // Copy to the x offset of the current pixel row
                tileRowBuffer + (y * SCREEN_WIDTH + tile_x * TILE_WIDTH) + 4, 
                // Reference the current sprite row
                (uint16_t *)&SPRITE_SHEET + getSpriteRow(BLOCK_SHINE_1, y),
                TILE_WIDTH * sizeof(uint16_t)
            );
        }

        // Draw right fringe
        memcpy(
            // Copy to the right side of the current pixel row (note pixel_y + 1 and the - TILE_WIDTH)
            // so 'select' the left side of next row then back it up by tile width
            tileRowBuffer + y * SCREEN_WIDTH + 316, 
            // Reference the current sprite row. This is a fringe tile so we only need 4 pixels
            (uint16_t *)&SPRITE_SHEET + getSpriteRow(BLOCK_SHINE_1, y),
            4 * sizeof(uint16_t)
        );
    }

    for (uint8_t tile_y = 0; tile_y < 20; ++tile_y)
    {
        // Draw buffer to screen
        ili9341_write_frame_rectangleLE(0, tile_y * TILE_WIDTH, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);
    }

    // Press start. This is at the bottom but is optimal to draw here given current buffer state
    // goes on row 216
    for(uint8_t y = 0; y < TILE_WIDTH; ++y)
    {
        // PRESS
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 94, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('P'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 106, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('R'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 118, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 130, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('S'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 142, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('S'), y), TILE_WIDTH * sizeof(uint16_t));

        // whitespace
        memset(tileRowBuffer + y * SCREEN_WIDTH + 154, '\337', TILE_WIDTH * sizeof(uint16_t));

        // START
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 166, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('S'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 178, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('T'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 190, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('A'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 202, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('R'), y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + y * SCREEN_WIDTH + 214, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('T'), y), TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(0, 216, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);

    printf("rolling numbers\n");
    // Dont want to include I piece in random selection
    uint8_t reroll = 3;
    uint8_t titleTiles[7];
    for(uint8_t i = 0; i < 7; ++i)
    {
        titleTiles[i] = getNextPiece();
        if(titleTiles[i] == BLOCK_I_ID)
            titleTiles[i] = reroll++ % 6;
    }

    // set buffer to black
    memset(tileRowBuffer, 0, 320 * TILE_WIDTH * sizeof(uint16_t));

    //draw black row
    uint8_t start_y = 2 * TILE_WIDTH;
    ili9341_write_frame_rectangleLE(0, start_y, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);

    printf("drawing top text row\n");
    // Top row
    start_y += TILE_WIDTH;
    for(uint8_t pixel_y = 0; pixel_y < 12; ++pixel_y)
    {
        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 4, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[0], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 16, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[0], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 28, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[0], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // E
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 52, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 64, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 76, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 100, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[2], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 112, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[2], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 124, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[2], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // R
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 148, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 160, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        if(pixel_y > 5)
        {
            memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 172, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y - 6), TILE_WIDTH * sizeof(uint16_t) );
        }

        // O
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 196, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 208, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 220, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // I
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 244, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[5], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 256, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[5], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // D
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 280, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 292, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        if(pixel_y > 5)
        {
            memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 304, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y - 6), TILE_WIDTH * sizeof(uint16_t) );
        }
    }

    ili9341_write_frame_rectangleLE(0, start_y, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);

    // set buffer to black between rows
    memset(tileRowBuffer, 0, 320 * TILE_WIDTH * sizeof(uint16_t));

    printf("drawing second text row\n");
    // Second row
    start_y += TILE_WIDTH;
    for(uint8_t pixel_y = 0; pixel_y < 12; ++pixel_y)
    {
        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 16, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[0], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // E
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 52, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        if(pixel_y > 5)
        {
            memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 64, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y -6), TILE_WIDTH * sizeof(uint16_t) );
        }

        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 112, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[2], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // R
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 148, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 172, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), (pixel_y + 6) % 12), TILE_WIDTH * sizeof(uint16_t) );

        // O
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 196, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 220, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // I stratles two columns here
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 250, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[5], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // D
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 280, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 304, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), (pixel_y + 6) % 12), TILE_WIDTH * sizeof(uint16_t) );
    }

    ili9341_write_frame_rectangleLE(0, start_y, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);
    
    // set buffer to black between rows
    memset(tileRowBuffer, 0, 320 * TILE_WIDTH * sizeof(uint16_t));

    printf("drawing third text row\n");
    // Third row
    start_y += TILE_WIDTH;
    for(uint8_t pixel_y = 0; pixel_y < 12; ++pixel_y)
    {
        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 16, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[0], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // E
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 52, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        if(pixel_y < 6)
        {
            memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 64, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y + 6), TILE_WIDTH * sizeof(uint16_t) );
        }

        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 112, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[2], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // R
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 148, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 160, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        if(pixel_y < 6)
        {
            memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 172, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y + 6), TILE_WIDTH * sizeof(uint16_t) );
        }

        // O
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 196, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 220, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // I stratles two columns here
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 250, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[5], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // D
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 280, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 304, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), (pixel_y + 6) % 12), TILE_WIDTH * sizeof(uint16_t) );
    }

    ili9341_write_frame_rectangleLE(0, start_y, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);

    // set buffer to black between rows
    memset(tileRowBuffer, 0, 320 * TILE_WIDTH * sizeof(uint16_t));

    printf("drawing bottom text row\n");
    // Bottom row
    start_y += TILE_WIDTH;
    for(uint8_t pixel_y = 0; pixel_y < 12; ++pixel_y)
    {
        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 16, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[0], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // E
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 52, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 64, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 76, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[1], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // T
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 112, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[2], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        
        // R
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 148, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 172, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[3], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        

        // O
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 196, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 208, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 220, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[4], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // I
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 244, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[5], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 256, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[5], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );

        // D
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 280, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 292, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t) );
        // half tile handling
        if(pixel_y < 6)
        {
            memcpy(tileRowBuffer + pixel_y * SCREEN_WIDTH + 304, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(titleTiles[6], false, 0), pixel_y + 6), TILE_WIDTH * sizeof(uint16_t) );
        }
    }

    ili9341_write_frame_rectangleLE(0, start_y, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);

    // set buffer to black for last black row
    memset(tileRowBuffer, 0, 320 * TILE_WIDTH * sizeof(uint16_t));
    start_y += TILE_WIDTH;
    ili9341_write_frame_rectangleLE(0, start_y, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);
    
    free(tileRowBuffer);
}

void drawGameScreen() // Draws static UI elements like well, frames for score and next piece
{
    // The screen will be drawn in two vertical halves as that is the most optimal organiztion of repeating elements
 
    // Allocate a single line buffer for a simple tile that not worth putting in the sprite sprite sheet
    uint16_t *tileLineBuffer = heap_caps_malloc(TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileLineBuffer)
    {
        abort();
    }
    printf("drawGameScreen: tileLineBuffer=%p\n", tileLineBuffer);

    // Only drawing the well half of the screen with this buffer
    tileRowBuffer = heap_caps_malloc(192 * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawGameScreen: tileRowBuffer=%p\n", tileRowBuffer);

    // Set up custom tile
    memset(tileLineBuffer, 0, 8 * sizeof(uint16_t));
    memset(tileLineBuffer + 8, '\337', 4 * sizeof(uint16_t));

    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Black tile
        memset(tileRowBuffer + pixel_y * 192, 0, TILE_WIDTH * sizeof(uint16_t));
        // Custom tile
        memcpy(tileRowBuffer + pixel_y * 192 + 12, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));
        // Brick tiles
        memcpy(tileRowBuffer + pixel_y * 192 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BLOCK_BRICK, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 192 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BLOCK_BRICK, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        // White tiles
        memset(tileRowBuffer + pixel_y * 192 + 48, '\337', 10 * TILE_WIDTH * sizeof(uint16_t));
        // Brick tiles
        memcpy(tileRowBuffer + pixel_y * 192 + 168, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BLOCK_BRICK, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 192 + 180, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BLOCK_BRICK, pixel_y), TILE_WIDTH * sizeof(uint16_t));
    }

    for (uint8_t tile_y = 0; tile_y < 20; ++tile_y)
    {
        // Draw buffer to screen
        ili9341_write_frame_rectangleLE(0, tile_y * TILE_WIDTH, 192, TILE_WIDTH, tileRowBuffer);
    }
    free(tileRowBuffer);

    // Only drawing score/next piece half of the screen with this buffer
    tileRowBuffer = heap_caps_malloc(128 * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawGameScreen: tileRowBuffer=%p\n", tileRowBuffer);

    // Set up custom tile for other side
    memset(tileLineBuffer, '\337', 4 * sizeof(uint16_t));
    memset(tileLineBuffer + 4, 0, 8 * sizeof(uint16_t));
    
    // Top of border around word score
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile
        memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile
        memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Score top left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_TL, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        // Score top
        for(uint8_t i = 0; i < 5; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 36 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_T, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }
        // Score top right
        memcpy(tileRowBuffer + pixel_y * 128 + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_TR, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Black fringe
        memset(tileRowBuffer + pixel_y * 128 + 108, 0, 20 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 0, 128, TILE_WIDTH, tileRowBuffer);

    // Middle of border around word score
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Score value border top top left
        memcpy(tileRowBuffer + pixel_y * 128, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP1_FRINGE, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score value border top top
        memcpy(tileRowBuffer + pixel_y * 128 + 12, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP1, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score word left border
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_L, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score word
        memcpy(tileRowBuffer + pixel_y * 128 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('S'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 48, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('C'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 60, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('O'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 72, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('R'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 84, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score word right border
        memcpy(tileRowBuffer + pixel_y * 128 + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_R, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Score top top and fringe
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP1, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 120, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP1, pixel_y), 8 * sizeof(uint16_t));
    }


    ili9341_write_frame_rectangleLE(192, 1 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row above score
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Score value border top top left
        memcpy(tileRowBuffer + pixel_y * 128, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP2_FRINGE, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score value border top top
        memcpy(tileRowBuffer + pixel_y * 128 + 12, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP2, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score word left border
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_BL, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Score bottom
        for(uint8_t i = 0; i < 5; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 36 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_B, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }

        // Score word right border
        memcpy(tileRowBuffer + pixel_y * 128 + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_BR, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Score top top and fringe
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP2, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 120, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_TOP2, pixel_y), 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 2 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Score value row (all white)
    memset(tileRowBuffer, '\337', 128 * TILE_WIDTH * sizeof(uint16_t));

    ili9341_write_frame_rectangleLE(192, 3 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row below score
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Score value border bottom left
        memcpy(tileRowBuffer + pixel_y * 128, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_BOTTOM_FRINGE, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Score value border bottom
        for(uint8_t i = 0; i < 9; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 12 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_BOTTOM, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }

        // right side 2/3 tile
        memcpy(tileRowBuffer + pixel_y * 128 + 120, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_SCORE_VAL_BOTTOM, pixel_y), 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 4 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Top border of level
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile
        memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile
        memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Level top left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_TL, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        // Level top
        for(uint8_t i = 0; i < 6; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 36 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_T, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }
        // Level top right
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_TR, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Black fringe
        memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));

    }

    ili9341_write_frame_rectangleLE(192, 5 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row for word level
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Level left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_L, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Level word
        memcpy(tileRowBuffer + pixel_y * 128 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('L'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 48, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 60, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('V'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 72, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 84, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('L'), pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // white tile
        memset(tileRowBuffer + pixel_y * 128 + 96, '\337', TILE_WIDTH * sizeof(uint16_t));

        // Level right
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_R, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 6 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row for level value // Row for word level
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Level left (same as last)
        // memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_L, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Level value (white space)
        memset(tileRowBuffer + pixel_y * 128 + 36, '\337', 5 * TILE_WIDTH * sizeof(uint16_t));

        // white tile   (same as last)
        // memset(tileRowBuffer + pixel_y * 128 + 96, '\337', TILE_WIDTH * sizeof(uint16_t));

        // Level right (same as last)
        // memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_R, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 7 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Bottom border of level
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Level bottom left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_BL, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        // Level bottom
        for(uint8_t i = 0; i < 6; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 36 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_B, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }
        // Level bottom right
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_BR, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));

    }

    ili9341_write_frame_rectangleLE(192, 8 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Top border of lines
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Lines top left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_TL, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        // Lines top
        for(uint8_t i = 0; i < 6; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 36 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_T, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }
        // Lines top right
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_TR, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Black fringe
        memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));

    }

    ili9341_write_frame_rectangleLE(192, 9 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row for word lines
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Lines left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_L, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Lines word
        memcpy(tileRowBuffer + pixel_y * 128 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('L'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 48, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('I'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 60, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('N'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 72, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixel_y), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixel_y * 128 + 84, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('S'), pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // white tile
        memset(tileRowBuffer + pixel_y * 128 + 96, '\337', TILE_WIDTH * sizeof(uint16_t));

        // Lines right
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_R, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 10 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row for lines value
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Lines left (same as last)
        // memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_L, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Lines value (white space)
        memset(tileRowBuffer + pixel_y * 128 + 36, '\337', 5 * TILE_WIDTH * sizeof(uint16_t));

        // white tile   (same as last)
        // memset(tileRowBuffer + pixel_y * 128 + 96, '\337', TILE_WIDTH * sizeof(uint16_t));

        // Lines right (same as last)
        // memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_R, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 11 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Bottom border of lines
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Lines bottom left
        memcpy(tileRowBuffer + pixel_y * 128 + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_BL, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        // Lines bottom
        for(uint8_t i = 0; i < 6; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 36 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_B, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }
        // Lines bottom right
        memcpy(tileRowBuffer + pixel_y * 128 + 108, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_NUM_BR, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        
        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));

    }

    ili9341_write_frame_rectangleLE(192, 12 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Top border of next piece
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Black tile
        memset(tileRowBuffer + pixel_y * 128 + 24, 0, TILE_WIDTH * sizeof(uint16_t));

        // Next piece border top left
        memcpy(tileRowBuffer + pixel_y * 128 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_TL, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Next piece border top
        for(uint8_t i = 0; i < 4; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 48 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_T, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }

        // Next piece border top right
        memcpy(tileRowBuffer + pixel_y * 128 + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_TR, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black tile
        memset(tileRowBuffer + pixel_y * 128 + 108, 0, TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 13 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Row 4 indentical rows for next piece window
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 24, 0, TILE_WIDTH * sizeof(uint16_t));

        // Next piece border left
        memcpy(tileRowBuffer + pixel_y * 128 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_L, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // White space
        memset(tileRowBuffer + pixel_y * 128 + 48, '\337', 4 * TILE_WIDTH * sizeof(uint16_t));

        // Next piece border right
        memcpy(tileRowBuffer + pixel_y * 128 + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_R, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 108, 0, TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 14 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);
    ili9341_write_frame_rectangleLE(192, 15 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);
    ili9341_write_frame_rectangleLE(192, 16 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);
    ili9341_write_frame_rectangleLE(192, 17 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);
    
    // Bottom border of next piece
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile (same as last row)
        // memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 12, 0, TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 24, 0, TILE_WIDTH * sizeof(uint16_t));

        // Next piece border bottom left
        memcpy(tileRowBuffer + pixel_y * 128 + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_BL, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Next piece border bottom
        for(uint8_t i = 0; i < 4; ++i)
        {
            memcpy(tileRowBuffer + pixel_y * 128 + 48 + (i * TILE_WIDTH), (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_B, pixel_y), TILE_WIDTH * sizeof(uint16_t));
        }

        // Next piece border bottom right
        memcpy(tileRowBuffer + pixel_y * 128 + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_PIECE_BR, pixel_y), TILE_WIDTH * sizeof(uint16_t));

        // Black tile (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 108, 0, TILE_WIDTH * sizeof(uint16_t));

        // Black fringe (same as last row)
        // memset(tileRowBuffer + pixel_y * 128 + 120, 0, 8 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(192, 18 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // set buffer to black
    memset(tileRowBuffer, 0, 128 * TILE_WIDTH * sizeof(uint16_t));
    // write only tile
    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
    {
        // Custom tile
        memcpy(tileRowBuffer + pixel_y * 128, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));
    }
    ili9341_write_frame_rectangleLE(192, 19 * TILE_WIDTH, 128, TILE_WIDTH, tileRowBuffer);

    // Cleanup
    free(tileRowBuffer);
    free(tileLineBuffer);
}

void drawField(uint8_t field[FIELD_HEIGHT][FIELD_WIDTH]) // Draws the game field and the current piece
{
    // Only drawing the well with this buffer
    tileRowBuffer = heap_caps_malloc(10 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawField: tileRowBuffer=%p\n", tileRowBuffer);

    for (uint8_t tile_y = 0; tile_y < 20; ++tile_y)
    {
        for (uint8_t tile_x = 0; tile_x < 10; ++tile_x)
        {
            for (uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
            {
                // O/0 will never be part of game field so we use it to mean empty for convenience
                if(field[tile_y][tile_x])
                {
                    memcpy(
                        tileRowBuffer + pixel_y * 120 + tile_x * TILE_WIDTH, 
                        (uint16_t*)&SPRITE_SHEET + getSpriteRowTileNum(field[tile_y][tile_x], pixel_y), 
                        TILE_WIDTH * sizeof(uint16_t)
                    );
                }
                else
                {
                    memset(tileRowBuffer + pixel_y * 120 + tile_x * TILE_WIDTH, '\337', TILE_WIDTH * sizeof(uint16_t));
                }
            }
        }

        // Draw buffer to the well portion of screen
        ili9341_write_frame_rectangleLE(48, tile_y * TILE_WIDTH, 10 * TILE_WIDTH, TILE_WIDTH, tileRowBuffer);
    }
    free(tileRowBuffer);
}

void drawPiece(uint8_t piece_id, uint8_t piece_x, uint8_t piece_y, uint8_t rotation)
{
    // If its an I piece, we can do it in a single draw
    if(piece_id == BLOCK_I_ID)
    {
        tileRowBuffer = heap_caps_malloc(4 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if(rotation & 1)
        {
            // portion is off screen, height to draw is either 2 or 3
            if(piece_y < 2)
            {
                for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
                {
                    memcpy(tileRowBuffer + pixel_y * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH) * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, piece_y == 0 ? 1 : 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                    if(piece_y == 1)
                        memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH * 2)  * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, 1), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                }
                ili9341_write_frame_rectangleLE(48 + piece_x * TILE_WIDTH, 0, TILE_WIDTH, (2 + piece_y) * TILE_WIDTH, tileRowBuffer);
            }
            else
            {
                for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
                {
                    memcpy(tileRowBuffer + pixel_y * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, -1), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH) * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH * 2)  * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH * 3)  * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, true, 1), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                }
                ili9341_write_frame_rectangleLE(48 + piece_x * TILE_WIDTH, (piece_y - 2) * TILE_WIDTH, TILE_WIDTH, 4 * TILE_WIDTH, tileRowBuffer);
            }
        }
        else
        {
            // Only need a width const because height is not needed for working with displays in memory as they are row ordered
            const uint8_t TETRO_WIDTH = TILE_WIDTH * 4;
            for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
            {
                memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, false, -1), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH + TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH + 2 * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH + 3 * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, false, 1), pixel_y), TILE_WIDTH * sizeof(uint16_t));
            }
            ili9341_write_frame_rectangleLE(48 + (piece_x - 2) * TILE_WIDTH, piece_y * TILE_WIDTH, TETRO_WIDTH, TILE_WIDTH, tileRowBuffer);
        }
        free(tileRowBuffer);
    }
    // Same if its an O piece, we can do it in a single draw
    else if(piece_id == BLOCK_O_ID)
    {
        tileRowBuffer = heap_caps_malloc(4 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        if(!tileRowBuffer)
        {
            abort();
        }
        printf("drawTitle: tileRowBuffer=%p\n", tileRowBuffer);
        const uint8_t TETRO_WIDTH = TILE_WIDTH * 2;
        for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
        {
            // Can get the row once for all four tiles
            const uint16_t SPRITE_ROW = getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y);
            memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
            memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH + TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
            memcpy(tileRowBuffer + (TILE_WIDTH + pixel_y) * TETRO_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
            memcpy(tileRowBuffer + (TILE_WIDTH + pixel_y) * TETRO_WIDTH + TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
        }
        ili9341_write_frame_rectangleLE(48 + (piece_x - 1) * TILE_WIDTH, piece_y * TILE_WIDTH, TETRO_WIDTH, TETRO_WIDTH, tileRowBuffer);
        free(tileRowBuffer);
    }
    // Otherwirse, need to draw piece as two rectangles to avoid overwriting the play field
    // I could always draw them tile by tile like a sane person but I prefer to "optimize" it
    else
    {
        // If its Z or S, the rectangles are 1x2 each
        if(piece_id == BLOCK_S_ID || piece_id == BLOCK_Z_ID)
        {
            tileRowBuffer = heap_caps_malloc(2 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            if(!tileRowBuffer)
            {
                abort();
            }
            printf("drawTitle: tileRowBuffer=%p\n", tileRowBuffer);

            uint8_t left1, left2, top1, top2, tetro_width, tetro_height;
            if(rotation & 1)
            {
                tetro_width = 1;
                tetro_height = 2;
                // Note: offscreen tile will always be in the first left/top
                top1 = piece_y - 1;
                top2 = piece_y;
                if(piece_id == BLOCK_Z_ID)
                {
                    left1 = piece_x + 1;
                    left2 = piece_x;
                }
                else
                {
                    left1 = piece_x;
                    left2 = piece_x + 1;
                }
            }
            else
            {
                tetro_width = 2;
                tetro_height = 1;
                top1 = piece_y;
                top2 = piece_y + 1;
                if(piece_id == BLOCK_Z_ID)
                {
                    left1 = piece_x - 1;
                    left2 = piece_x;
                }
                else
                {
                    left1 = piece_x;
                    left2 = piece_x - 1;
                }
            }

            for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
            {
                const uint16_t SPRITE_ROW = getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y);
                memcpy(tileRowBuffer + pixel_y * (tetro_width * TILE_WIDTH), (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                if(tetro_height == 2) // offset y by TILE_WIDTH
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH) * (tetro_width * TILE_WIDTH), (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                else // offset x by TILE_WIDTH
                    memcpy(tileRowBuffer + pixel_y * (tetro_width * TILE_WIDTH) + TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
            }

            // Note: offscreen tile will always be in the first draw
            bool topOffscreen = (piece_y == 0 && tetro_height == 2); // only true when both equal 2 since piece_y can never be 1
            ili9341_write_frame_rectangleLE(48 + left1 * TILE_WIDTH, topOffscreen ? 0 : (top1 * TILE_WIDTH), tetro_width * TILE_WIDTH, (topOffscreen ? 1 : tetro_height) * TILE_WIDTH, tileRowBuffer);
            ili9341_write_frame_rectangleLE(48 + left2 * TILE_WIDTH, top2 * TILE_WIDTH, tetro_width * TILE_WIDTH, tetro_height * TILE_WIDTH, tileRowBuffer);
            free(tileRowBuffer);
        }
        // Otherwise, one is 1x1 and the other is 1x3
        else
        {
            tileRowBuffer = heap_caps_malloc(3 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            if(!tileRowBuffer)
            {
                abort();
            }
            printf("drawTitle: tileRowBuffer=%p\n", tileRowBuffer);

            // Pieces in this block are composed of a 3x1 "bar" and a 1x1 "tile"
            bool tileOffscreen = false;
            uint8_t deg = rotation % 4;
            // Regardless of which piece, they all have a 3 long bar that is horizontal or vertical based on rotation
            if(rotation & 1)
            {
                // Only offscreen for vertical rotations
                bool barOffScreen = (piece_y == 0);
                // In vertical rotation, the offscreen conditions are more varied
                tileOffscreen = (barOffScreen && ((piece_id == BLOCK_L_ID && deg == 1) || (piece_id == BLOCK_J_ID && deg == 3)));
                for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
                {
                    const uint16_t SPRITE_ROW = getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y);
                    memcpy(tileRowBuffer + pixel_y * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH) * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + (pixel_y + TILE_WIDTH * 2) * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                }
                
                // Only draws 2 tiles if top one is off screen
                ili9341_write_frame_rectangleLE(48 + piece_x * TILE_WIDTH, (piece_y - (barOffScreen ? 0 : 1)) * TILE_WIDTH, TILE_WIDTH, TILE_WIDTH * (barOffScreen ? 2 : 3), tileRowBuffer);
            }
            else
            {
                // All pieces handled here will have their 1x1 offscreen at this position and degree
                tileOffscreen = (piece_y == 0 && deg == 2);
                const uint8_t TETRO_WIDTH = 3 * TILE_WIDTH;
                for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
                {
                    const uint16_t SPRITE_ROW = getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y);
                    memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH + TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                    memcpy(tileRowBuffer + pixel_y * TETRO_WIDTH + TILE_WIDTH * 2, (uint16_t*)&SPRITE_SHEET + SPRITE_ROW, TILE_WIDTH * sizeof(uint16_t));
                }
                
                // This portion will never be off screen
                ili9341_write_frame_rectangleLE(48 + (piece_x - 1) * TILE_WIDTH, piece_y * TILE_WIDTH, TETRO_WIDTH, TILE_WIDTH, tileRowBuffer);
            }
            
            if(!tileOffscreen)
            {
                // Setup buffer with single tile for 1x1 piece if not already formatted as such
                if(!(rotation & 1))
                {
                    for(uint8_t pixel_y = 0; pixel_y < TILE_WIDTH; ++pixel_y)
                    {
                        memcpy(tileRowBuffer + pixel_y * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getTetrominoCoord(piece_id, false, 0), pixel_y), TILE_WIDTH * sizeof(uint16_t));
                    }
                }

                uint8_t left, top;
                if(piece_id == BLOCK_T_ID)
                {
                    if(deg == 0)
                    {
                        left = piece_x;
                        top = (piece_y + 1);
                    }
                    else if(deg == 1)
                    {
                        left = piece_x - 1;
                        top = piece_y;
                    }
                    else if(deg == 2)
                    {
                        left = piece_x;
                        top = piece_y - 1;
                    }
                    else //(deg == 3)
                    {
                        left = piece_x + 1;
                        top = piece_y;
                    }
                }
                else
                {
                    if(deg == 0)
                    {
                        top = piece_y + 1;
                    }
                    else if(deg == 2)
                    {
                        top = piece_y - 1;
                    }
                    else
                    {
                        top = piece_y + (((piece_id == BLOCK_J_ID) == (deg == 1)) ? 1 : -1);
                    }

                    if(deg == 1)
                    {
                        left = piece_x - 1;
                    }
                    else if(deg == 3)
                    {
                        left = piece_x + 1;
                    }
                    else
                    {
                        left = piece_x + (((piece_id == BLOCK_J_ID) == (deg == 2)) ? -1 : 1);
                    }
                }

                ili9341_write_frame_rectangleLE(48 + left * TILE_WIDTH, top * TILE_WIDTH, TILE_WIDTH, TILE_WIDTH, tileRowBuffer);
            }
            free(tileRowBuffer);
        }
    }
}

void drawClearAnimation(uint8_t lines[4], uint8_t phase)
{
    
    tileRowBuffer = heap_caps_malloc(10 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawClearAnimation: tileRowBuffer=%p\n", tileRowBuffer);

    if(phase == 4)
        memset(tileRowBuffer, '\337', 10 * TILE_WIDTH * TILE_WIDTH * sizeof(uint16_t));
    else
    {
        for(int8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
        {
            for(uint8_t tileX = 0; tileX < FIELD_WIDTH; ++tileX)
            {
                // Copy current phase's tile row from sprite sheet. Animation only has 3 tiles, hence the mask
                memcpy(
                    tileRowBuffer + pixelY * 120 + tileX * TILE_WIDTH, 
                    (uint16_t*)&SPRITE_SHEET + getSpriteRow((sheet_coord){phase % 3, BLOCK_SHINE_1.y}, pixelY), 
                    TILE_WIDTH * sizeof(uint16_t)
                );
            }
        }
    }

    // For each line checked
    for(uint8_t i = 0; i < 4; ++i)
    {
        // If line was cleared
        if(lines[i] != 32)
        {
            // Draw line animation for current phase
            ili9341_write_frame_rectangleLE(48, lines[i] * TILE_WIDTH, 10 * TILE_WIDTH, TILE_WIDTH, tileRowBuffer);
        }
    }

    free(tileRowBuffer);
}

void drawNumber(uint32_t num, NumberType type)
{
    if(type == LEVEL || type == LINES)
    {
        if (num > 999999)
            num = 999999;
        
        tileRowBuffer = heap_caps_malloc(6 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    }
    else
    {
        if(num > 999999999)
            num = 999999999;
            
        tileRowBuffer = heap_caps_malloc(9 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    }

    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawNumber: tileRowBuffer=%p\n", tileRowBuffer);

    uint8_t x, y, width;
    switch(type)
    {
        case SCORE:
            width = 9 * TILE_WIDTH;
            x = 204;
            y = 36;
            break;
        case LEVEL:
            width = 6 * TILE_WIDTH;
            x = 228;
            y = 84;
            break;
        case LINES:
            width = 6 * TILE_WIDTH;
            x = 228;
            y = 132;
            break;
        default:
            printf("Unknown number type %d\n", type);
            abort();
            x = y = 0;
    }

    uint8_t pixelX = 0;
    for(uint32_t divisor = (type == SCORE ? 100000000 : 100000); divisor > 0; divisor /= 10)
    {
        uint8_t currentDigit = num / divisor;
        sheet_coord digCoord = getNumCoord(currentDigit);
        // Copy current digit to buffer
        for(uint8_t pixelY = 0; pixelY < 12; ++pixelY)
        {
            memcpy(tileRowBuffer + (pixelY * width) + pixelX, (uint16_t*)&SPRITE_SHEET + getSpriteRow(digCoord, pixelY), TILE_WIDTH * sizeof(uint16_t));
        }

        // Update num for next digit
        num %= divisor;
        pixelX += TILE_WIDTH;
    }
    // Write buffer to screen
    ili9341_write_frame_rectangleLE(x, y, width, TILE_WIDTH, tileRowBuffer);

    free(tileRowBuffer);
}

// blank the space in next piece window since the field draw doesnt take of it
void clearNextPiece()
{
    // Allocate buffer 4 tiles wide by 2 tiles tall
    tileRowBuffer = heap_caps_malloc(4 * TILE_WIDTH * 2 * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("clearNextPiece: tileRowBuffer=%p\n", tileRowBuffer);

    // set buffer to white
    memset(tileRowBuffer, '\337', 4 * TILE_WIDTH * 2 * TILE_WIDTH * sizeof(uint16_t));
    // write to screen
    ili9341_write_frame_rectangleLE(240, 180, 4 * TILE_WIDTH, 2 * TILE_WIDTH, tileRowBuffer);

    free(tileRowBuffer);
}

void drawNextPiece(uint8_t piece_id)
{
    clearNextPiece();
    // Thanks to collision detection in game logic, the display logic never checks if the piece will be out of field bounds so we can use it for next piece as well
    drawPiece(piece_id, 18, 15, 0);
}

// When the player pauses the game, blank the field so as not to give them an advantage from pausing
void drawPause()
{
    // Only drawing the well with this buffer
    tileRowBuffer = heap_caps_malloc(10 * TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawField: tileRowBuffer=%p\n", tileRowBuffer);

    memset(tileRowBuffer, '\337', 10 * TILE_WIDTH * TILE_WIDTH * sizeof(uint16_t));
    // for each row of the well
    for (uint8_t tile_y = 0; tile_y < 20; ++tile_y)
    {
        // Draw buffer to the well portion of screen
        ili9341_write_frame_rectangleLE(48, tile_y * TILE_WIDTH, 10 * TILE_WIDTH, TILE_WIDTH, tileRowBuffer);
    }

    uint8_t wordWidth = 5 * TILE_WIDTH;
    for(uint8_t pixelY = 0; pixelY < 12; ++pixelY)
    {
        memcpy(tileRowBuffer + (pixelY * wordWidth), (uint16_t*)&SPRITE_SHEET + getSpriteRow(getCharCoord('P'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + (pixelY * wordWidth) + 12, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getCharCoord('A'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + (pixelY * wordWidth) + 24, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getCharCoord('U'), pixelY), TILE_WIDTH * sizeof(uint16_t)); // close enough...
        memcpy(tileRowBuffer + (pixelY * wordWidth) + 36, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getCharCoord('S'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + (pixelY * wordWidth) + 48, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixelY), TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(78, 60, wordWidth, TILE_WIDTH, tileRowBuffer);

    free(tileRowBuffer);
}

// TODO add param for top scores
void drawMenu()
{
    tileRowBuffer = heap_caps_malloc(SCREEN_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawTitle: tileRowBuffer=%p\n", tileRowBuffer);

    // Draw top and bottom rows
    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++pixelY)
    {
        // Screen width is not evenly divisible by 12 so we have to draw 1/3rd blocks at the left and right sides
        // Set left fringe to white on left side of buffer
        memset(tileRowBuffer + pixelY * SCREEN_WIDTH, '\337', 4 * sizeof(uint16_t));

        // draw left tile
        memcpy(
            // Copy to the x offset of the current pixel row
            tileRowBuffer + pixelY * SCREEN_WIDTH + 4, 
            // Reference the current sprite row
            (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_TL, pixelY),
            TILE_WIDTH * sizeof(uint16_t)
        );
        
        // draw repeating custom tile. can define custom tile inline since its rows are all the same pixel value
        memset(tileRowBuffer + pixelY * SCREEN_WIDTH + 16, pixelY / 4 == 1 ? 0 : '\377', 288 * sizeof(uint16_t));

        // draw right tile
        memcpy(
            // Copy to the x offset of the current pixel row
            tileRowBuffer + pixelY * SCREEN_WIDTH + 304, 
            // Reference the current sprite row
            (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_TR, pixelY),
            TILE_WIDTH * sizeof(uint16_t)
        );

        // Draw right fringe
        memset(tileRowBuffer + pixelY * SCREEN_WIDTH + 316, '\337', 4 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(0, 0, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);


    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++pixelY)
    {
        // Only need to update two tiles for bottom row
        // draw left tile   
        memcpy(
            // Copy to the x offset of the current pixel row
            tileRowBuffer + pixelY * SCREEN_WIDTH + 4, 
            // Reference the current sprite row
            (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_BL, pixelY),
            TILE_WIDTH * sizeof(uint16_t)
        );

        // draw right tile
        memcpy(
            // Copy to the x offset of the current pixel row
            tileRowBuffer + pixelY * SCREEN_WIDTH + 304, 
            // Reference the current sprite row
            (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_BR, pixelY),
            TILE_WIDTH * sizeof(uint16_t)
        );
    }

    ili9341_write_frame_rectangleLE(0, 228, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);

    // Draw middle/background rows
    // Allocate a single line buffer for a simple tile that not worth putting in the sprite sprite sheet
    uint16_t *tileLineBuffer = heap_caps_malloc(TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileLineBuffer)
    {
        abort();
    }
    printf("drawMenu: tileLineBuffer=%p\n", tileLineBuffer);
    
    // set up custom tile
    memset(tileLineBuffer, '\337', 4 * sizeof(uint16_t));
    memset(tileLineBuffer + 4, 0, 4 * sizeof(uint16_t));
    memset(tileLineBuffer + 8, '\337', 4 * sizeof(uint16_t));

    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++pixelY)
    {
        // draw left white fringe
        memset(tileRowBuffer + pixelY * SCREEN_WIDTH, '\337', 4 * sizeof(uint16_t));
        // draw left tile
        memcpy(
            // Copy to the x offset of the current pixel row
            tileRowBuffer + pixelY * SCREEN_WIDTH + 4, 
            tileLineBuffer,
            TILE_WIDTH * sizeof(uint16_t)
        );
        
        // draw repeating tiles
        uint16_t spriteRow = getSpriteRow(BLOCK_I_BORDERLESS, pixelY);
        for(uint8_t tile_x = 0; tile_x < 24; ++tile_x)
        {
            memcpy(
                // Copy to the x offset of the current pixel row
                tileRowBuffer + pixelY * SCREEN_WIDTH + tile_x * TILE_WIDTH + 16, 
                // Reference the current sprite row
                (uint16_t *)&SPRITE_SHEET + spriteRow,
                TILE_WIDTH * sizeof(uint16_t)
            );
        }

        // draw right tile
        memcpy(
            // Copy to the x offset of the current pixel row
            tileRowBuffer + pixelY * SCREEN_WIDTH + 304, 
            tileLineBuffer,
            TILE_WIDTH * sizeof(uint16_t)
        );
        // draw right white fringe
        memset(tileRowBuffer + pixelY * SCREEN_WIDTH + 316, '\337', 4 * sizeof(uint16_t));
    }
    
    //for 18 times, draw
    for(uint8_t rowY = 1; rowY < 19; ++rowY)
    {
        ili9341_write_frame_rectangleLE(0, rowY * 12, SCREEN_WIDTH, TILE_WIDTH, tileRowBuffer);
    }

    // Draw UI elements over background
    memset(tileRowBuffer, 0, 16 * sizeof(uint16_t));
    // draw adjoining border elements over existing screen draw
    ili9341_write_frame_rectangleLE(116, 8, 4, 4, tileRowBuffer);
    ili9341_write_frame_rectangleLE(12, 28, 4, 4, tileRowBuffer);

    // set up draw for 9 tiles, draw top left corner
    uint8_t rowWidth = TILE_WIDTH * 9;
    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
    {
        memcpy(tileRowBuffer + pixelY * rowWidth, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getNumCoord(1), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 12, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('-'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 24, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('P'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 36, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('L'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 48, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('A'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 60, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('Y'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 72, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 84, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('R'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        // custom tile
        memcpy(tileRowBuffer + pixelY * rowWidth + 96, tileLineBuffer, TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(16, 12, rowWidth, TILE_WIDTH, tileRowBuffer);

    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
    {
        // custom tile for first 8
        memset(tileRowBuffer + pixelY * rowWidth, pixelY / 4 == 1 ? 0 : '\377', 8 * TILE_WIDTH * sizeof(uint16_t));

        memcpy(tileRowBuffer + pixelY * rowWidth + 96, (uint16_t *)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_BR, pixelY), TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(16, 24, rowWidth, TILE_WIDTH, tileRowBuffer);

    // set up draw width, draw the word LEVEL with borders above and below
    rowWidth = TILE_WIDTH * 6;
    // half tiles for this row, but we actually only have to draw 4 pixel rows instead of 6 thanks to background
    for(uint8_t pixelY = 2; pixelY < 6; ++ pixelY)
    {
        uint8_t bufferY = pixelY - 2;
        memcpy(tileRowBuffer + bufferY * rowWidth, (uint16_t*)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_HEADER_LC, pixelY), TILE_WIDTH * sizeof(uint16_t));
        uint16_t topRow = getSpriteRow(BORDER_MENU_HEADER_HS, pixelY);
        for(uint16_t tileX = 1; tileX < 5; ++ tileX)
        {
            memcpy(tileRowBuffer + bufferY * rowWidth + tileX * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + topRow, TILE_WIDTH * sizeof(uint16_t));
        }
        memcpy(tileRowBuffer + bufferY * rowWidth + 60, (uint16_t*)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_HEADER_RC, pixelY), TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(124, 44, rowWidth, 4, tileRowBuffer);

    // back to full tiles for this row (except borders, hence we shrink row width temporarily)
    rowWidth -= 4;
    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
    {
        uint16_t sideBorders = getSpriteRow(BORDER_MENU_HEADER_VS, pixelY);
        // left side half tile, again only concerned with the 4 pixels that actually change
        memcpy(tileRowBuffer + pixelY * rowWidth, (uint16_t*)&SPRITE_SHEET + sideBorders + 2, 4 * sizeof(uint16_t));

        memcpy(tileRowBuffer + pixelY * rowWidth + 4, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('L'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 16, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 28, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('V'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 40, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('E'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        memcpy(tileRowBuffer + pixelY * rowWidth + 52, (uint16_t *)&SPRITE_SHEET + getSpriteRow(getCharCoord('L'), pixelY), TILE_WIDTH * sizeof(uint16_t));
        // right side half tile
        memcpy(tileRowBuffer + pixelY * rowWidth + 64, (uint16_t*)&SPRITE_SHEET + sideBorders + 6, 4 * sizeof(uint16_t));
    }

    // write to screen skipping two pixels on first and last tiles as they already have the correct pixels
    ili9341_write_frame_rectangleLE(126, 48, rowWidth, TILE_WIDTH, tileRowBuffer);

    // then half tiles again for bottom
    rowWidth += 4;
    for(uint8_t pixelY = 6; pixelY < 10; ++ pixelY)
    {
        uint8_t bufferY = pixelY - 6;
        memcpy(tileRowBuffer + bufferY * rowWidth, (uint16_t*)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_HEADER_LC, pixelY), TILE_WIDTH * sizeof(uint16_t));
        uint16_t topRow = getSpriteRow(BORDER_MENU_HEADER_HS, pixelY);
        for(uint16_t tileX = 1; tileX < 5; ++ tileX)
        {
            memcpy(tileRowBuffer + bufferY * rowWidth + tileX * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + topRow, TILE_WIDTH * sizeof(uint16_t));
        }
        memcpy(tileRowBuffer + bufferY * rowWidth + 60, (uint16_t*)&SPRITE_SHEET + getSpriteRow(BORDER_MENU_HEADER_RC, pixelY), TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(124, 60, rowWidth, 4, tileRowBuffer);

    // write level number selection portion of menu
    rowWidth = TILE_WIDTH * 8 - 4;
    // start with black borders 2 pixels tall and rowWidth wide
    memset(tileRowBuffer, 0, 2 * rowWidth * sizeof(uint16_t));
    ili9341_write_frame_rectangleLE(114, 70, rowWidth, 2, tileRowBuffer);
    ili9341_write_frame_rectangleLE(114, 84, rowWidth, 2, tileRowBuffer);
    ili9341_write_frame_rectangleLE(114, 94, rowWidth, 2, tileRowBuffer);
    ili9341_write_frame_rectangleLE(114, 108, rowWidth, 2, tileRowBuffer);
    // side borders
    ili9341_write_frame_rectangleLE(114, 86, 2, 8, tileRowBuffer);
    ili9341_write_frame_rectangleLE(204, 86, 2, 8, tileRowBuffer);
    
    // now write numbers between the borders we just placed
    // set up custom half tile for border between numbers
    memset(tileLineBuffer, '\337', 2 * sizeof(uint16_t));
    memset(tileLineBuffer + 2, 0, 2 * sizeof(uint16_t));
    memset(tileLineBuffer + 4, '\337', 2 * sizeof(uint16_t));
    
    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
    {
        uint16_t sideBorder = getSpriteRow(BORDER_MENU_HEADER_VS, pixelY);
        // left side border
        memcpy(tileRowBuffer + pixelY * rowWidth, (uint16_t*)&SPRITE_SHEET + sideBorder + 2, 4 * sizeof(uint16_t));
        // numbers and dividers
        for(uint8_t num = 0; num < 5; ++num)
        {
            // each number + divider occupies 1 and 1/2 tiles, hence the 18 below
            memcpy(tileRowBuffer + pixelY * rowWidth + num * 18 + 4, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getNumCoord(num), pixelY), TILE_WIDTH * sizeof(uint16_t));
            if(num != 4) // last one gets border, not divider
                // custom half tile divider
                memcpy(tileRowBuffer + pixelY * rowWidth + num * 18 + 16, tileLineBuffer, 6 * sizeof(uint16_t));

        }
        // right side border
        memcpy(tileRowBuffer + pixelY * rowWidth + 88, (uint16_t*)&SPRITE_SHEET + sideBorder + 6, 4 * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(114, 72, rowWidth, TILE_WIDTH, tileRowBuffer);

    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
    {
        // just numbers this time, everything else is same as last draw
        for(uint8_t num = 5; num < 10; ++num)
        {
            // each number + divider occupies 1 and 1/2 tiles, hence the 18 below
            uint8_t offset = num - 5;
            memcpy(tileRowBuffer + pixelY * rowWidth + offset * 18 + 4, (uint16_t*)&SPRITE_SHEET + getSpriteRow(getNumCoord(num), pixelY), TILE_WIDTH * sizeof(uint16_t));
        }
    }

    ili9341_write_frame_rectangleLE(114, 96, rowWidth, TILE_WIDTH, tileRowBuffer);

    // TODO draw top scores here

    free(tileLineBuffer);
    free(tileRowBuffer);
}

void drawTile(sheet_coord tile, uint8_t left, uint8_t top)
{
    tileRowBuffer = heap_caps_malloc(TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawTile: tileRowBuffer=%p\n", tileRowBuffer);

    for(uint8_t pixelY = 0; pixelY < TILE_WIDTH; ++ pixelY)
    {
        memcpy(tileRowBuffer + pixelY * TILE_WIDTH, (uint16_t*)&SPRITE_SHEET + getSpriteRow(tile, pixelY), TILE_WIDTH * sizeof(uint16_t));
    }

    ili9341_write_frame_rectangleLE(left, top, TILE_WIDTH, TILE_WIDTH, tileRowBuffer);
    free(tileRowBuffer);
}

void blankTile(uint8_t left, uint8_t top)
{
    tileRowBuffer = heap_caps_malloc(TILE_WIDTH * TILE_WIDTH * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    if(!tileRowBuffer)
    {
        abort();
    }
    printf("drawTile: tileRowBuffer=%p\n", tileRowBuffer);
    memset(tileRowBuffer, '\337', TILE_WIDTH * TILE_WIDTH * 2);

    ili9341_write_frame_rectangleLE(left, top, TILE_WIDTH, TILE_WIDTH, tileRowBuffer);
    free(tileRowBuffer);
}

void drawLevel(uint8_t level)
{
    if(level > 9)
    {
        printf("level out of bounds = %d", level);
        abort();
    }
    uint8_t left, top;
    if(level > 4)
    {
        top = 96;
        left = 118 + (level - 5) * 18;
    }
    else
    {
        top = 72;
        left = 118 + level * 18;
    }

    drawTile(getNumCoord(level), left, top);
}

void blankLevel(uint8_t level)
{
    if(level > 9)
    {
        printf("level out of bounds = %d", level);
        abort();
    }
    uint8_t left, top;
    if(level > 4)
    {
        top = 96;
        left = 118 + (level - 5) * 18;
    }
    else
    {
        top = 72;
        left = 118 + level * 18;
    }
    blankTile(left, top);
}