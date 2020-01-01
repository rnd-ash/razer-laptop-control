#ifndef EFFECTS_H_
#define EFFECTS_H_
#include "keyboard.hpp"

// DO NOT CHANGE THIS TO ANYTHING HIGHER! Doing so can make the keyboard unresponsive!
#define MAX_UPDATE_INTERVAL_MS 40 // 25 times per second we can update the keyboard


class EFFECT {
    public:
        EFFECT(keyboard *board);
        virtual void updateTick();
        virtual void updateTickDemo1();
        virtual void updateTickDemo2();
        virtual void updateTickDemo3();
    private:
        keyboard *kboard;
        __uint8_t tick = 0;
        __uint8_t tick2 = 0;
};

#endif