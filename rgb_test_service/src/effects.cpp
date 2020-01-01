#include "effects.h"
#include "colours.hpp"




EFFECT::EFFECT(keyboard *board) {
    this->kboard = board;
}

void EFFECT::updateTick() {
    
}

void EFFECT::updateTickDemo1() {
    struct colour c = {(__uint8_t)rand() % 255, (__uint8_t)rand() % 255, (__uint8_t)rand() % 255};
    kboard->matrix->setColumnColour(tick, c);
    if (tick >= 14) {
        tick = 0;
    } else {
        tick++;
    }
    kboard->update();
}

void EFFECT::updateTickDemo2() {
    struct colour c = {(__uint8_t)rand() % 255, (__uint8_t)rand() % 255, (__uint8_t)rand() % 255};
    kboard->matrix->setRowColour(tick, c);
    if (tick >= 5) {
        tick = 0;
    } else {
        tick++;
    }
    kboard->update();
}


void EFFECT::updateTickDemo3() {
    struct colour c = {(__uint8_t)rand() % 255, (__uint8_t)rand() % 255, (__uint8_t)rand() % 255};
    kboard->matrix->clearColours();
    kboard->matrix->setKeyColour(tick, tick2, c);
    if (tick >= 14) {
        tick = 0;
        tick2++;
    } else {
        tick++;
    }
    if (tick2 >= 5) {
        tick = 0;
        tick2 = 0;
    }
    kboard->update();
}