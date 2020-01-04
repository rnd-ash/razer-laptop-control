#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include "power_fan_control.hpp"
#include "keyboard.hpp"
#include "effects.h"
#include <ctime>


#define DRIVER_DIR "/sys/module/razercontrol/drivers/hid:Razer laptop System control driver"

namespace fs = std::filesystem;

void help() {
    std::cout << "Incorrect usage!:\n\n"
        << "--fan auto        Set fan to auto\n"
        << "--fan 1000        Set fan to 1000 rpm\n"
        << "--power gaming    Enable gaming power mode\n"
        << "--power balanced  Enable balanced power mode\n"
        << "--power creator   Enable creator power mode\n";
}


fs::path sysfs_path;
int main(int argc, char *argv[]) {
    srand(time(0));
    if (!fs::exists(DRIVER_DIR)) {
        std::cout << "Razercontrol module is not loaded!\n";
        return 1;
    }
    if (getuid()) {
        std::cout << "Must be run as ROOT!\n";
        return 1;
    }
    for(auto& p: fs::directory_iterator(DRIVER_DIR)) {
        if (p.is_directory()) {
            fs::path x = p.path();
            x.append("fan_rpm");
            if (fs::exists(x)) {
                std::cout << p.path() << std::endl;
                sysfs_path = p.path();
            }
            //break;
        }
    }
    if (argc <=2) {
        help();
        return 0;
    }
    for (int i = 1; i < argc; i+=2) {
        if (std::string(argv[i]) == "--fan") {
            int rpm = std::atoi(argv[i+1]);
            if (rpm == 0) {
                fan_control(sysfs_path).setAutoFan();
            } else {
                fan_control(sysfs_path).setManualFan(rpm);
            }
        } else if (std::string(argv[i]) == "--power") {
            int power_mode = 0;
            if (std::string(argv[i+1]) == "gaming") {
                power_mode = 1;
            } else if (std::string(argv[i+1]) == "creator") {
                power_mode = 2;
            }
            power_control(sysfs_path).setPowerMode(power_mode);
        } else if (std::string(argv[i]) == "--rgb_demo") {
            if (std::string(argv[i+1]) == "starlight") {
                if (argc != i+3) {
                    std::cout << "Starlight requires noise argument (1-10)" << std::endl;
                    return 1;
                }
                int noise = atoi(argv[i+2]);
                if (noise < 1 || noise > 10) {
                    std::cout << "Starlight requires noise argument (1-10)" << std::endl;
                }
                std::cout << "Starlight mode!" << std::endl;
                keyboard k = keyboard(sysfs_path);
                STARLIGHT_EFFECT e = STARLIGHT_EFFECT(&k, noise);
                while(true) {
                    e.startLightTick();
                    usleep(MAX_UPDATE_INTERVAL_MS*1000);
                }
            }
            else if (std::string(argv[i+1]) == "wave") {
                if (argc != i+4) {
                    std::cout << "Wave requires 2 arguments (Direction 1-4), Speed(1-256)" << std::endl;
                    return 1;
                }
                int dir = atoi(argv[i+2]);
                int spd = atoi(argv[i+3]);
                if (dir > 4 || dir < 0) {
                    std::cout << "Wave direction incorrect!\n"
                              << "(0) Right to left\n"
                              << "(1) Left to Right\n"
                              << "(2) Up to down\n"
                              << "(3) Down to up" << std::endl;
                    return 1;
                }
                if (spd < 0 || spd > 256) {
                    std::cout << "Wave speed must be between 1 and 256" << std::endl;
                    return 1;
                }
                keyboard k = keyboard(sysfs_path);
                WAVE_EFFECT e = WAVE_EFFECT(&k, dir, spd);
                std::cout << "Wave mode!" << std::endl;
                while (true) {
                    e.updateTick();
                    usleep(MAX_UPDATE_INTERVAL_MS*1000);
                }
            }
            else if (std::string(argv[i+1]) == "static") {
                if (argc == i+5) {
                    int r = atoi(argv[i+2]);
                    int g = atoi(argv[i+3]);
                    int b = atoi(argv[i+4]);
                    keyboard k = keyboard(sysfs_path);
                    k.matrix->setAllColour({r,g,b});
                    k.update();
                    k.update();
                }
                else {
                    std::cout << "Static mode needs 3 numbers (R,G,B)" << std::endl;
                    return 1;
                }
            }
        }
    }
    return 0;
}