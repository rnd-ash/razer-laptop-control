#ifndef EFFECTS_H_
#define EFFECTS_H_
#include "keyboard.hpp"
#include <stdlib.h>
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include <vector>

// DO NOT CHANGE THIS TO ANYTHING HIGHER! Doing so can make the keyboard unresponsive!
#define MAX_UPDATE_INTERVAL_MS 40 // 25 times per second we can update the keyboard


class STARLIGHT_EFFECT {
    public:
        STARLIGHT_EFFECT(keyboard *board, int noise);
        virtual void startLightTick();
    private:
        struct keyPos {
            int y;
            int x;
            colour currentColour;
            colour targetColour;
            float redShift;
            float greenShift;
            float blueShift;
            bool falling;
            bool animationDone;
        };
        std::vector<keyPos> keysToUpdate;
        keyboard *kboard;
        unsigned long tick = 0;
        void addNewKey();
        int noise;
};


class WAVE_EFFECT {
    public :
        WAVE_EFFECT(keyboard *board, int dir, int speed);
        void changeDir(int dir);
        void updateTick();
    private:

        struct colour_seq {
            bool redFalling;
            bool greenFalling;
            bool blueFalling;
            colour c;
        };
        colour_seq current_start;
        keyboard *kboard;
        unsigned long tick = 0;
        float interval;
        int direction;
        void updateRows(colour_seq start, int resolution);
};

class AMBIENT_EFFECT {
    public:
        AMBIENT_EFFECT(keyboard *board);
        void updateTick();
    private:
        keyboard *kboard;
        int width;
        int height;
        Display *display;
        Window root;
        XWindowAttributes gwa;
        XImage *image;
};

#endif