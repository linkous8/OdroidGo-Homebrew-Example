#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

#include "tetrominoes.h"
#include "random.h"
#include "screen.h"
#include "sprite_sheet.h"

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "../odroid/odroid_input.h"

// FPS limiter vars
// FreeRTOS has been configured to tick at 120hz which, divided by two, results in an even 60 update/draws per second
#define TICKS_PER_UPDATE (2)
TickType_t xLastWakeTime;

// matrix of blocks on field. Each value is the SPRITE_SHEET offset (in units of squared TILE_WIDTH) of the tiles.
// Blank tile is represented by 0 so collissions are easily checked against non-zero values
// even though tile number 0 is occupie by a character (O/0) it will never be part of game field so we use it for convenience
uint8_t playField[FIELD_HEIGHT][FIELD_WIDTH];

uint8_t currentPiece = 0;
uint8_t currentRotation = 0;
int8_t currentX = 5;
uint8_t currentY = 0;
uint8_t nextPiece = 0;

// Used to determine how many frames between drops for the given level
const uint8_t GRAVITIES[30] = {48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 };
bool maxGrav = false; // Set this to true at lvl 29

uint16_t MULTIPLIERS[5] = {0, 40, 100, 300, 1200};
uint32_t score = 0, lines = 0, level = 0;
uint8_t softDropBonus = 0; // Incremented for every frame a soft (down held) drop occurs, reset to zero when down released. If down held during lock, value is added to score

uint8_t speed = 48;
int8_t speedCounter = -96; // First piece gets a looong delay before it starts dropping. Will be cancelled if play soft drops
bool forceDown = false;

uint8_t repeatX = 0, repeatY = 0;
bool leftHeld = false, rightHeld = false, downHeld = false, aHeld = false, bHeld = false, selHeld =false, startHeld =false;

odroid_gamepad_state joystick;

// Indicates whether a certain configuration of current piece will fit on the current playField without overlapping the locked pieces or walls/floor
// No params to save memory
bool doesCollide()
{
    // construct a bitmask of playField at currentPiece's position
    uint16_t fieldMask = 0;
    uint16_t bitMaskDec = 0x8000;   // Set highest order bit to one, shift it to the next lowest order each iteration
    // the two row above the piece y can potentially be occupied by a tile part of the piece, a
    for(int8_t y = currentY - 2; y <  currentY + 2; ++y)
    {
        // Special cases ensure we dont go out of playfield bounds on y axis. if() short-circuiting handles the x axis
        if(y > 19)
        {
            // floor collision bits added to mask
            for(uint8_t i = 0; i < 4; ++i)
            {
                fieldMask |= bitMaskDec;
                bitMaskDec >>= 1;
            }
            continue;
        }

        for(int8_t x = currentX - 2; x < currentX + 2; ++x)
        {
            // Note we still want to collide with walls even if they are in virtual rows
            if(x < 0 || x > 9 || (y > -1 && playField[y][x] > 0)) // If field is a wall or non-zero at position
            {
                // Set current position in mask to 1
                fieldMask |= bitMaskDec;
            }
            bitMaskDec >>= 1;
        }
    }

    // Compare fieldMask to mask for current rotation of piece
    if(fieldMask & getRotation(currentPiece, currentRotation))
    {
        // debug
        // printf("Collision!\nfieldMask=%x\npieceMask=%x\n", fieldMask, getRotation(currentPiece, currentRotation));
        return true;    // One or more bits overlapped between the numbers, collision has occurred
    }

    return false;
}

void debounce(int key, TickType_t xLastWakeTime)
{
  while (joystick.values[key])
  {
    odroid_input_gamepad_read(&joystick);
    vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );
  }
}

void game()
{
    // initRandom(); - considering moving this here instead of app_main

    // TODO load top scores and player nick name

    // Display title
    drawTitle();

    // Wait for input
    bool bGameOver = true;
    xLastWakeTime = xTaskGetTickCount();

    while(true)
    {
        
        // Wait for player to press start before exiting title
        while(true)
        {
            odroid_input_gamepad_read(&joystick);
            if(joystick.values[ODROID_INPUT_START]) {
                debounce(ODROID_INPUT_START, xLastWakeTime);
                break;
            }
            vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );
        }

        // Display menu
        // TODO top score
        drawMenu();
        
        int8_t levelSelection = 0, flashTimer = 0;
        while(true)
        {
            odroid_input_gamepad_read(&joystick);
            if(joystick.values[ODROID_INPUT_RIGHT]) {
                debounce(ODROID_INPUT_RIGHT, xLastWakeTime);
                drawLevel(levelSelection);
                ++levelSelection;
                if(levelSelection > 9)
                    levelSelection = 0;
            }
            else if(joystick.values[ODROID_INPUT_LEFT]) {
                debounce(ODROID_INPUT_LEFT, xLastWakeTime);
                drawLevel(levelSelection);
                --levelSelection;
                if(levelSelection < 0)
                    levelSelection = 9;
            }
            else if(joystick.values[ODROID_INPUT_UP]) {
                debounce(ODROID_INPUT_UP, xLastWakeTime);
                drawLevel(levelSelection);
                levelSelection -= 5;
                if(levelSelection < 0)
                    levelSelection += 10;
            }
            else if(joystick.values[ODROID_INPUT_DOWN]) {
                debounce(ODROID_INPUT_DOWN, xLastWakeTime);
                drawLevel(levelSelection);
                levelSelection += 5;
                if(levelSelection > 9)
                    levelSelection -= 10;
            }
            else if(joystick.values[ODROID_INPUT_A]) {
                debounce(ODROID_INPUT_A, xLastWakeTime);
                break;
            }
            else if(joystick.values[ODROID_INPUT_B]) {
                debounce(ODROID_INPUT_B, xLastWakeTime);
                break;
            }
            else if(joystick.values[ODROID_INPUT_START]) {
                debounce(ODROID_INPUT_START, xLastWakeTime);
                break;
            }

            // flash the current selection. 8 frames on, 8 frames off
            if(flashTimer & 8)
                blankLevel(levelSelection);
            else
                drawLevel(levelSelection);

            ++flashTimer;
            if(flashTimer > 15)
                flashTimer -= 16;
            vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );
        }


        // Broke out of menu loop, begin game loop
        bGameOver = false;
        level = levelSelection;
        score = 0;
        lines = 0;
        speed = GRAVITIES[levelSelection];
        speedCounter = -96; // drop delay for first piece

        // Initiate game loop
        drawGameScreen();
        memset(playField, 0, FIELD_HEIGHT * FIELD_WIDTH * sizeof(playField[0][0]));

        // Initial piece draw. Redraw does not happen every frame. 
        //  Instead draw updates occur only as neccessary for the portrayal of the game state
        drawPiece(currentPiece, currentX, currentY, currentRotation);

        nextPiece = getNextPiece();
        drawNextPiece(nextPiece);

        drawNumber(lines, LINES);
        drawNumber(level, LEVEL);
        drawNumber(score, SCORE);

        while(!bGameOver)
        {
            // Handle input
            odroid_input_gamepad_read(&joystick);
            // Lower held bools and other onRelease events
            if(startHeld && !joystick.values[ODROID_INPUT_START])
                startHeld = false;
            if(selHeld && !joystick.values[ODROID_INPUT_SELECT])
                selHeld = false;
            if(bHeld && !joystick.values[ODROID_INPUT_B])
                bHeld = false;
            if(aHeld && !joystick.values[ODROID_INPUT_A])
                aHeld = false;
            if(rightHeld && !joystick.values[ODROID_INPUT_RIGHT])
                rightHeld = false;
            if(leftHeld && !joystick.values[ODROID_INPUT_LEFT])
                leftHeld = false;
            if(downHeld && !joystick.values[ODROID_INPUT_DOWN])
            {
                downHeld = false;
                repeatY = 0;
                softDropBonus = 0;
            }

            if(joystick.values[ODROID_INPUT_START])
            {
                if(!startHeld)
                {
                    startHeld = true;
                    clearNextPiece();
                    drawPause();
                    vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );

                    while(1)
                    {
                        // TODO make it so select will exit to menu once we have one
                        odroid_input_gamepad_read(&joystick);
                        if(startHeld && !joystick.values[ODROID_INPUT_START])
                            startHeld = false;
                        
                        if(joystick.values[ODROID_INPUT_START] && !startHeld)
                        {
                            startHeld = true;
                            break;
                        }
                        
                        vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );
                    }

                    drawField(playField);
                    drawPiece(currentPiece, currentX, currentY, currentRotation);
                    drawNextPiece(nextPiece);
                    vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );
                    // when unpaused, skip to next game loop iteration
                    continue;
                }
            }

            // Do we force piece down this frame?
            forceDown = (++speedCounter == speed);

            // Shift logic
            if(joystick.values[ODROID_INPUT_DOWN]) {
                if(!downHeld) { // Just pressed down this frame
                    downHeld = true;
                    repeatY = 1;
                }
                else
                {
                    ++repeatY;

                    if(repeatY >= 3)
                    {
                        ++softDropBonus;
                        repeatY = 1;
                        forceDown = true;
                    }
                }
            }
            else if(joystick.values[ODROID_INPUT_LEFT]) {
                if(!leftHeld)   // if left was just pressed
                {
                    leftHeld = true;

                    repeatX = 0;
                    --currentX;
                    if(currentX < 0 || doesCollide())
                    {
                        ++currentX;
                        repeatX = 16;
                    }
                    else
                    {
                        drawField(playField);
                        drawPiece(currentPiece, currentX, currentY, currentRotation);
                    }
                }
                // else if/else left was pressed prior to this frame
                else if (repeatX == 16) 
                {
                    repeatX = 10;
                    --currentX;
                    if(currentX < 0 || doesCollide())
                    {
                        ++currentX;
                        repeatX = 16;
                    }
                    else
                    {
                        drawField(playField);
                        drawPiece(currentPiece, currentX, currentY, currentRotation);
                    }
                }
                else
                    ++repeatX;
            }
            else if(joystick.values[ODROID_INPUT_RIGHT]) {
                if(!rightHeld)   // if right was just pressed
                {
                    rightHeld = true;

                    repeatX = 0;
                    ++currentX;
                    if(currentX > 9 || doesCollide())
                    {
                        --currentX;
                        repeatX = 16;
                    }
                    else
                    {
                        drawField(playField);
                        drawPiece(currentPiece, currentX, currentY, currentRotation);
                    }
                }
                else if (repeatX == 16) 
                {
                    repeatX = 10;
                    ++currentX;
                    if(currentX > 9 || doesCollide())
                    {
                        --currentX;
                        repeatX = 16;
                    }
                    else
                    {
                        drawField(playField);
                        drawPiece(currentPiece, currentX, currentY, currentRotation);
                    }
                }
                else
                    ++repeatX;
            }

            // Rotate logic
            // TODO feature: allow optional rotate repeat like done for shifting
            if(joystick.values[ODROID_INPUT_A]) {
                if(!aHeld)
                {
                    aHeld = true;
                    ++currentRotation;
                    if(doesCollide())
                        --currentRotation;
                    else
                    {
                        drawField(playField);
                        drawPiece(currentPiece, currentX, currentY, currentRotation);
                    }
                }
            }
            if(joystick.values[ODROID_INPUT_B]) {
                if(!bHeld)
                {
                    bHeld = true;
                    --currentRotation;
                    if(doesCollide())
                        ++currentRotation;
                    else
                    {
                        drawField(playField);
                        drawPiece(currentPiece, currentX, currentY, currentRotation);
                    }
                }
            }

            // Drop logic
            if(forceDown) {
                forceDown = false;
                speedCounter = 0;
                ++currentY;
                if(doesCollide() || currentY > 19)
                {
                    // Cant drop, lock piece into last valid position
                    --currentY;
                    if(currentPiece == BLOCK_I_ID)
                    {
                        // I block has more than one tile and require addtl processing
                        for(int8_t i = -2; i < 2; ++i)
                        {
                            if(currentRotation & 1)
                                playField[currentY + i][currentX] = getTetrominoTileNum(currentPiece, true, (i < 0 ? i + 1 : i));
                            else
                                playField[currentY][currentX + i] = getTetrominoTileNum(currentPiece, false, (i < 0 ? i + 1 : i));
                        }
                    }
                    else
                    {
                        uint16_t bitMaskDec = 0x8000;   // Set highest order bit to one, shift it to the next lowest order each iteration
                        uint16_t pieceMask = getRotation(currentPiece, currentRotation);
                        uint8_t tileNum = getTetrominoTileNum(currentPiece, false, 0);
                        for(int8_t y = currentY - 2; y < currentY + 2; ++y)
                        {
                            for(int8_t x = currentX - 2; x < currentX + 2; ++x)
                            {
                                if(x > -1 && x < 10 && y > -1 && y < 20 && pieceMask & bitMaskDec)
                                    playField[y][x] = tileNum;
                                bitMaskDec >>=1;
                            }
                        }
                    }
                    // No need to redraw for lock

                    // Add soft drop bonus to score
                    score += softDropBonus;
                

                    uint8_t clearedRows[4] = { 32, 32, 32, 32 }; // Using 32 because its out of playfield bounds and a single bit number
                    uint8_t linesCleared = 0; // For scoring and multiplayer
                    // Count number of cleared lines, updating playField as needed by shifting the rows down and clearing the top
                    for(int8_t curRow = -2; curRow < 2; ++curRow)
                    {
                        int8_t y = currentY + curRow;
                        // While parts of piece can be locked offscreen, they are never actually stored. Also dont want to count below field
                        if(y < 0 || y > 19)
                            continue;

                        bool rowComplete = true;
                        for(uint8_t x = 0; x < FIELD_WIDTH; ++x)
                        {
                            if(playField[y][x] == 0)
                            {
                                rowComplete = false;
                                break;
                            }
                        }

                        if(rowComplete)
                        {
                            ++linesCleared;
                            clearedRows[curRow + 2] = y;
                            // If top row, nothing to shift
                            for(; y > 0; --y )
                            {
                                memcpy(playField[y], playField[y - 1], 10 * sizeof(playField[0][0]));
                            }
                            // Clear top row
                            memset(playField[y], 0, 10 * sizeof(playField[0][0]));
                        }
                    }

                    // Line clearing animation
                    if(linesCleared != 0)
                    {
                        // Lines 
                        // For each phase in the animation
                        for(uint8_t p = 0; p < 5; ++p)
                        {
                            drawClearAnimation(clearedRows, p);
                            // delay for four frames
                            vTaskDelayUntil(&xLastWakeTime, TICKS_PER_UPDATE * 4);
                        }

                        lines += linesCleared;
                        if(lines / 10 > level)
                        {
                            ++ level;
                            if(!maxGrav)
                            {
                                speed = GRAVITIES[level];
                                if(level == 29)
                                    maxGrav = true;
                            }
                            drawNumber(level, LEVEL);
                        }
                        drawNumber(lines, LINES);
                        
                        score += MULTIPLIERS[linesCleared] * (level + 1);
                    }

                    if(softDropBonus > 0 || linesCleared > 0)
                    {
                        drawNumber(score, SCORE);
                    }

                    // Set next piece
                    currentPiece = nextPiece;
                    nextPiece = getNextPiece();
                    currentX = 5;
                    currentY = 0;
                    currentRotation = 0;
                    softDropBonus = 0;

                    if(doesCollide())
                    {
                        printf("Game Over\n");
                        bGameOver = true;
                    }

                    // Calculate entry delay
                    uint8_t entryDelay = 18 - 2 * ((currentY - 2) / 4);
                    // reduce delay by one frame to account for delay at end of loop
                    vTaskDelayUntil(&xLastWakeTime, TICKS_PER_UPDATE * (entryDelay - 1));
                    
                    drawNextPiece(nextPiece);
                }

                drawField(playField);
                drawPiece(currentPiece, currentX, currentY, currentRotation);
            }

            // Draw to screen (causes tearing so we do it selectively in the update logic)
            // drawField(playField);
            // drawPiece(currentPiece, currentX, currentY, currentRotation);
            
            // Delay if neccessary to cap at 60fps
            vTaskDelayUntil( &xLastWakeTime, TICKS_PER_UPDATE );
        }
        // Game over, redraw the title for now
        drawTitle();
    }
}