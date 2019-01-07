#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "esp_system.h"

#define FIELD_WIDTH (10)
#define FIELD_HEIGHT (20)

typedef enum NumberType {
    SCORE,
    LEVEL,
    LINES
} NumberType;

// Coordinates rendering to the odroid screen

void drawTitle(); // Draws title 'art' to the ili screen
void drawGameScreen(); // Draws the well and UI elements like frames for score and next piece
void drawMenu();

// draw single tiles, used for selecting input like start level or high score name
// void drawTile(sheet_coord tile, uint8_t left, uint8_t top);
// void blankTile(uint8_t left, uint8_t top);
// callers of draw/blankTile
void drawLevel(uint8_t level);
void blankLevel(uint8_t level);


void drawNumber(uint32_t num, NumberType type);
void drawNextPiece(uint8_t piece_id);
void clearNextPiece();
void drawPause();

void drawField(uint8_t field[FIELD_HEIGHT][FIELD_WIDTH]); // Draws the game field and the current piece
void drawPiece(uint8_t piece_id, uint8_t piece_x, uint8_t piece_y, uint8_t rotation);
void drawClearAnimation(uint8_t lines[4], uint8_t phase);

#endif