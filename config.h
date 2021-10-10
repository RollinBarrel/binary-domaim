struct SensConfig {
    unsigned int base;
    unsigned int ads;
    unsigned int sniper;
    unsigned int accel;
    bool toggleADS;
    bool invertedX;
    bool invertedY;
};

SensConfig* initConfig();