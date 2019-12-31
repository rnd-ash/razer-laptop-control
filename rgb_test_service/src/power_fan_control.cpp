#include "power_fan_control.hpp"

fan_control::fan_control(std::filesystem::path p) {
    this->path =  p.append("fan_rpm");
}

void fan_control::setAutoFan() {
    std::ofstream file;
    file.open(path);
    file << "0";
    file.close();
}

void fan_control::setManualFan(int rpm) {
    std::ofstream file;
    file.open(path);
    file << std::to_string(rpm);
    file.close();
}



power_control::power_control(std::filesystem::path p) {
    this->path = p.append("power_mode");
}

void power_control::setPowerMode(int mode) {
    std::ofstream file;
    file.open(path);
    std::cout<< path << std::endl;
    file << std::to_string(mode);
    file.close();
}