#include <filesystem>
#include <iostream>
#include <fstream>
#include <string.h>

class fan_control {
    public:
        fan_control(std::filesystem::path p);
        void setAutoFan();
        void setManualFan(int rpm);
    private:
        std::filesystem::path path;
};

class power_control {
    public:
        power_control(std::filesystem::path p);
        void setPowerMode(int mode);
    private:
        std::filesystem::path path;
};