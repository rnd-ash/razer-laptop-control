#include "effects.h"
#include "colours.hpp"
#include <algorithm>


STARLIGHT_EFFECT::STARLIGHT_EFFECT(keyboard *board, int noise) {
    this->kboard = board;
    this->noise = noise;
}

void STARLIGHT_EFFECT::startLightTick() {
    for (int i = 0; i < noise;i++) {
        addNewKey();
    }
    // GC tick
    if (tick % 10 == 0) {
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

void STARLIGHT_EFFECT::addNewKey() {
    int posy = rand() % 6;
    int posx = rand() % 15;
    bool canAdd = false;
    for (keyPos i : keysToUpdate) {
        if (i.x == posx && i.y == posy) {
            return;
        }
    }
    int steps = 20-(1.5*noise);
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





WAVE_EFFECT::WAVE_EFFECT(keyboard *board, int dir, int speed) {
    this->kboard = board;
    this->current_start = {true, false, true,{512,0,0}};
    this->updateRows(current_start, 15);
    this->interval = speed;
    this->direction = dir;
}

void WAVE_EFFECT::updateTick() {
    this->updateRows(this->current_start, 15);
}

void WAVE_EFFECT::changeDir(int dir) {
    this->direction = dir;
}

void WAVE_EFFECT::updateRows(colour_seq start, int resolution) {
    bool isUpdateOutput = false;
    colour_seq current = start;
    int loop_start;
    int loop_end;
    int loop_step;
    bool horizontal = true;
    bool currentUpdate = false;
    switch (this->direction)
    {
    case 1:
        loop_start = 14;
        loop_end = -1;
        loop_step = -1;
        break;
    case 2:
        loop_start = 0;
        loop_end = 6;
        loop_step = 1;
        horizontal = false;
        break;
    case 3:
        loop_start = 5;
        loop_end = -1;
        loop_step = -1;
        horizontal = false;
        break;
    default:
        loop_start = 0;
        loop_end = 15;
        loop_step = 1;
        break;
    }
    
    while (loop_start != loop_end) {
        if (horizontal) {
            kboard->matrix->setColumnColour(loop_start, current.c);
        } else {
            kboard->matrix->setRowColour(loop_start, current.c);
        }

        if (current.redFalling) {
            current.c.red -= this->interval;
        } else {
            current.c.red += this->interval;
        }

        if (current.c.green >= 512) {
            current.greenFalling = true;
        } else if (current.c.green <= -256) {
            current.greenFalling = false;
        }

        if (current.greenFalling) {
            current.c.green -= this->interval;
        } else {
            current.c.green += this->interval;
        }

        if (current.c.blue >= 512) {
            current.blueFalling = true;
        } else if (current.c.blue <= -256) {
            current.blueFalling = false;
        }

        if (current.blueFalling) {
            current.c.blue -= this->interval;
        } else {
            current.c.blue += this->interval;
        }

        if (current.c.red >= 512) {
            current.redFalling = true;
        } else if (current.c.red <= -256) {
            current.redFalling = false;
        }

        if (!currentUpdate) {
            this->current_start = current;
            currentUpdate = true;
        }

        loop_start += loop_step;
    }
    kboard->update();
}


AMBIENT_EFFECT::AMBIENT_EFFECT(keyboard *board) {
    this->kboard = board;
    this->display = XOpenDisplay(NULL);
    Screen* s = DefaultScreenOfDisplay(display);
    width = 1920/10;
    height = 1080/10;
    root = RootWindow(display, 0);
    std::cout << width << " " << height << std::endl;
}

void AMBIENT_EFFECT::updateTick() {
    colour c = {0};
    int s_y = height / 6;
    int s_x = width / 15;
    image = XGetImage(display,root, 0,0 , width*10, height*10, AllPlanes, ZPixmap);
    unsigned long red_mask = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask = image->blue_mask;
    int samples = 0;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            unsigned long pixel = XGetPixel(image,x*10,y*10);
            unsigned int blue = pixel & blue_mask;
            unsigned int green = (pixel & green_mask) >> 8;
            unsigned int red = (pixel & red_mask) >> 16;
            c.red += red; 
            c.green += green; 
            c.blue += blue;
            samples++;

            if (x % s_x == 0 && y % s_y == 0) {
                c.red = c.red / samples;
                c.green = c.green / samples;
                c.blue = c.blue / samples;
                kboard->matrix->setKeyColour(x / s_x, y / s_y, c);
                samples = 0;
                c = {0x00};
            }
        }
    }
    XDestroyImage(image);
    kboard->update();
}