#include "keyboard.hpp"

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
        void setEffect(effect e);
    private:
        key_row rows[6];
};