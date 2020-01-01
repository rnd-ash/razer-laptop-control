#include "keyboard.hpp"

keyboard::keyboard(std::filesystem::path p) {
    path_all = p.append("rgb_map");
    struct colour c;
    c.blue = 255;
    c.green = 255;
    c.red = 255;
    matrix = new key_matrix();
}

void keyboard::update() {
    std::ofstream file;
    file.open(path_all);
    file.write(this->matrix->toRGBData(), 360);
    file.close();
}

void key_matrix::setKeyColour(__uint8_t keyX, __uint8_t keyY, struct colour c) {
    map[keyY][keyX] = c;
}

void key_matrix::setRowColour(__uint8_t row, struct colour c) {
    for (int i = 0; i < 15; i++) {
        map[row][i] = c;
    }
}
void key_matrix::setColumnColour(__uint8_t col, struct colour c) {
    for (int i = 0; i < 6; i++) {
        map[i][col] = c;
    }
}

void key_matrix::setAllColour(struct colour c){
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 15; x++) {
            map[y][x] = c;
        }
    }
}

void key_matrix::clearColours() {
    memset(map, 0x00, sizeof(map));
}

char * key_matrix::toRGBData() {
    int pos = 0;
    for (int y = 0; y < 6; y++) {
        for (int x = 0; x < 15; x++) {
            buffer[pos  ] = map[y][x].red;
            buffer[pos+1] = map[y][x].green;
            buffer[pos+2] = map[y][x].blue;
            buffer[pos+3] = 0xFF;
            pos +=4;
        }
    }
    return buffer; 
}