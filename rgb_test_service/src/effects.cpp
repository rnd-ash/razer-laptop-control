#include "effects.h"
#include "colours.hpp"
#include <algorithm>



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
    
}
#define MAX_TICKS 50
void EFFECT::startLightTick() {
    if (tick % 2 == 0) {
        addNewKey();
    }
    // GC tick
    if (tick % MAX_TICKS * 2 == 0) {
        keysToUpdate.erase(
            std::remove_if(
                keysToUpdate.begin(), keysToUpdate.end(),
                [](keyPos i){ return i.animationDone == true;}),
        keysToUpdate.end());
    }
    tick++;
    for (int i = 0; i < keysToUpdate.size(); i++) {
        keyPos k = keysToUpdate[i];
        if ((k.falling == true) && (k.currentColour.red <= 0x0a && k.currentColour.green <= 0x0a && k.currentColour.blue <= 0x0a)) {
            k.currentColour = {0x00, 0x00, 0x00};
            k.animationDone = true;
        }
        if (!k.animationDone) {
            if(k.falling == false && (k.currentColour.red >= k.targetColour.red
                        || k.currentColour.green >= k.targetColour.green
                        || k.currentColour.blue >= k.targetColour.blue)
                ) {
                k.currentColour = k.targetColour;
                k.falling = true;
            } else {
                if (k.falling) {
                    k.currentColour.blue -= k.blueShift;
                    k.currentColour.green -= k.greenShift;
                    k.currentColour.red -= k.redShift;
                } else {
                    k.currentColour.red += k.redShift;
                    k.currentColour.blue += k.blueShift;
                    k.currentColour.green += k.greenShift;
                }
            }
        }
        keysToUpdate[i] = k;
        kboard->matrix->setKeyColour(k.x, k.y, k.currentColour);
    }
    kboard->update();
}

void EFFECT::addNewKey() {
    int posy = rand() % 6;
    int posx = rand() % 15;
    bool canAdd = false;
    for (keyPos i : keysToUpdate) {
        if (i.x == posx && i.y == posy) {
            return;
        }
    }
    int steps = 50;
    colour c = {rand() % 256, rand() % 256, rand() % 256};
    struct keyPos p;
    p.x = posx;
    p.y = posy;
    p.currentColour = {0x00,0x00,0x00};
    p.targetColour = c;
    p.redShift = (float) c.red / steps;
    p.blueShift = (float) c.blue / steps;
    p.greenShift = (float) c.green / steps;
    p.falling = false;
    p.animationDone = false;

    keysToUpdate.push_back(p);
    kboard->matrix->setKeyColour(posx, posy, c);
}