#include <filesystem>
#include <iostream>
#include <fstream>
#include <string.h>

class key {
    
};

class key_row {
    private:
        key keys[15];
};

class effect {

};


class keyboard {
    public:
        keyboard(std::filesystem::path p);
        void setEffect(effect e);
        void setColour(__uint8_t r, __uint8_t g, __uint8_t b, __uint8_t brightness);
    private:
        std::filesystem::path path;
        key_row rows[6];
};