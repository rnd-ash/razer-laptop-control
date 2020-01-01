#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string.h>

#define ANIMATION_FPS 25 // LOCKED. Any more and keyboard will struggle to update!

#define MATRIX_HEIGHT 6
#define MATRIX_WIDTH 15

/**
 * Key colour data
 */
struct colour {
    int red;
    int green;
    int blue;
};

struct colour_clamped {
    __uint8_t red;
    __uint8_t green;
    __uint8_t blue;
};

class key_matrix {
    public:
        void setKeyColour(__uint8_t keyX, __uint8_t keyY, struct colour c);
        void setRowColour(__uint8_t row, struct colour c);
        void setColumnColour(__uint8_t col, struct colour c);
        void setAllColour(struct colour c);
        void clearColours();
        char * toRGBData();
    private:
        colour_clamped getClamped(colour c);
        char buffer[360]; // To write to driver
        struct colour_clamped map[MATRIX_HEIGHT][MATRIX_WIDTH]; // 6 rows, 15 columns
};

class keyboard {
    public:
        keyboard(std::filesystem::path p);
        void update();
        key_matrix *matrix;
    private:
        std::filesystem::path path_all;
};
#endif