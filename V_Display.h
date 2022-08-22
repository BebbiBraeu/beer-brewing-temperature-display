#ifndef V_DISPLAY_H
#define V_DISPLAY_H
#include <Arduino.h>
#include "TFT_eSPI.h"

class V_Display : public TFT_eSPI
{
    public:
        V_Display();
        void printText(int x, int y, String text, uint8_t textSize = 1, uint8_t textAllign = 1, uint8_t lineLength = 239);
};

#endif