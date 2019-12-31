#include "keyboard.hpp"
keyboard::keyboard(std::filesystem::path p) {
    path = p.append("rgb_map");
    std::cout << path << std::endl;
}
void keyboard::setEffect(effect e) {

}
void keyboard::setColour(__uint8_t r, __uint8_t g, __uint8_t b, __uint8_t brightness) {
    std::ofstream file;
    char buffer[360];
    for (int i = 0; i < 360; i+=4) {
        buffer[i] = r;
        buffer[i+1] = g;
        buffer[i+2] = b;
        buffer[i+3] = brightness;
    }
    file.open(path);
    file.write(buffer, 360);
    file.close();
}
