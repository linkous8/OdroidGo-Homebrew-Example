#include "esp_system.h"

#include "bootloader_random.h"

// ESP32 has TRNG which generates 32 bit numbers. We only need a value from 0-7 and would be a shame
//  to waste all that random data with a modulo. So instead we shift three bits off at a time and call random
//  only after remaining bits < 3
uint8_t randomReads = 0;
uint32_t randomValue;
// NES Tetris has a bias against sequential duplicate pieces and re-rolls once when it occurs
uint8_t lastRandom = 7;

// Initialize first random value
void initRandom()
{
    randomValue = esp_random();
}

// Gets next tetris piece
uint8_t getNextPiece()
{
    // Set return value to the 3 least significant bits of random value
    uint8_t retVal = randomValue & 7;
    if(++randomReads == 10)
    {
        randomReads = 0;
        randomValue = esp_random();
    }
    else // Shift value to set up access for next four bits
        randomValue = randomValue >> 3;
    
    // If next piece same as last or number is equal to 7, re-roll once
    if(retVal == lastRandom || retVal == 7)
    {
        retVal = randomValue & 7;
        if(++randomReads == 10)
        {
            randomReads = 0;
            randomValue = esp_random();
        }
        else
            randomValue = randomValue >> 3;
    }
    
    // Constrain 
    return retVal % 7;
}