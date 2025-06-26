// adopted from Scriptease
#include <switch.h>

const char* const get_keys[16] =
{
    "A\0",
    "B\0",
    "X\0", 
    "Y\0",
    "LSTICK\0", 
    "RSTICK\0",
    "L\0",
    "R\0",
    "ZL\0",
    "ZR\0",
    "PLUS\0",
    "MINUS\0",
    "DLEFT\0",
    "DUP\0",
    "DRIGHT\0",
    "DDOWN\0"
};

extern Handle debughandle;

extern HiddbgHdlsHandle controllerHandle;
extern HiddbgHdlsDeviceInfo controllerDevice;
extern HiddbgHdlsState controllerState;

extern ViDisplay disp;
extern Event vsyncEvent;

void attach();
void detach();
bool getIsPaused();
void advanceOneFrame();
HidNpadButton parseStringToButton(char* arg);
