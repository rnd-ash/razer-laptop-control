#ifndef EFFECTS_H_
#define EFFECTS_H_
#include "keyboard.hpp"
#include <stdlib.h>
#include <vector>

// DO NOT CHANGE THIS TO ANYTHING HIGHER! Doing so can make the keyboard unresponsive!
#define MAX_UPDATE_INTERVAL_MS 40 // 25 times per second we can update the keyboard


class EFFECT {
    public:
        EFFECT(keyboard *board);
        virtual void updateTick();
        virtual void updateTickDemo1();
        virtual void updateTickDemo2();
        virtual void updateTickDemo3();
        virtual void startLightTick();
    private:
        struct keyPos {
            int y;
            int x;
            colour currentColour;
            colour targetColour;
            float redShift;
            float blueShift;
            float greenShift;
            bool falling;
            bool animationDone;
        };
        std::vector<keyPos> keysToUpdate;
        keyboard *kboard;
        unsigned long tick = 0;
        void addNewKey();
};

#endif