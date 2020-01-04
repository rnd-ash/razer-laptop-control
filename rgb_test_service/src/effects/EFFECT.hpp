#include "../keyboard.hpp"

class EFFECT_KEY {
    
};

class EFFECT {
    public:
        EFFECT(keyboard *board);
        virtual void updateTick() = 0;
    private:
        keyboard kboard;
        unsigned long tick_number;
};