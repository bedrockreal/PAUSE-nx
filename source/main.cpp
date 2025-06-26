// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <sstream>

// Include the main libnx system header, for Switch development
// #include <switch.h>

extern "C"
{
    #include "util.h"
    // Sysmodules should not use applet*.
    u32 __nx_applet_type = AppletType_None;

    // Adjust size as needed.
    #define INNER_HEAP_SIZE 0x8000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_init_time(void);
    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    ViDisplay disp;
    Event vsyncEvent;
}

HidMouseState mouse_state;
const u64 pause_toggle = HidMouseButton_Left;
const u64 frame_advance_button = HidMouseButton_Right;
u32 prev_mouse_buttons = 0;

void __libnx_initheap(void)
{
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	// Newlib
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

// Init/exit services, update as needed.
void __attribute__((weak)) __appInit(void)
{
    Result rc;

    // Initialize default services.

    rc = smInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_SM));

    rc = fsInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_FS));

    fsdevMountSdmc();

    // Enable this if you want to use HID.
    rc = hidInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));

    //Enable this if you want to use time.
    rc = timeInitialize();
    if (R_FAILED(rc))
        fatalThrow(MAKERESULT(Module_Libnx, LibnxError_InitFail_Time));

    __libnx_init_time();

    // Initialize system for pmdmnt
    rc = setsysInitialize();
    if (R_SUCCEEDED(rc)) {
        SetSysFirmwareVersion fw;
        rc = setsysGetFirmwareVersion(&fw);
        if (R_SUCCEEDED(rc))
            hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();
    }

    rc = viInitialize(ViServiceType_System);
    if(R_FAILED(rc))
        fatalThrow(rc);

    pmdmntInitialize();
}

void __attribute__((weak)) userAppExit(void);

void __attribute__((weak)) __appExit(void)
{
    // Cleanup default services.
    fsdevUnmountAll();
    fsExit();
    timeExit();//Enable this if you want to use time.
    hidExit();// Enable this if you want to use HID.
    
    viCloseDisplay(&disp);
    viExit();
    smExit();
}

std::string translateKeys(u64 keys)
{
    std::string returnString;
    for (int i = 0; i < 16; ++i)
    {
        if ((keys & (1 << i)))
        {
            if (!returnString.empty()) returnString += ";";
            returnString += "KEY_" + std::string(get_keys[i]);
        }
    }
    return returnString.empty() ? "NONE" : returnString;
}

u32 getMouseDown()
{
    hidGetMouseStates(&mouse_state, 1);
    u32 cur_mouse_buttons = mouse_state.buttons;
    u32 ret = (~prev_mouse_buttons) & cur_mouse_buttons;
    prev_mouse_buttons = cur_mouse_buttons;
    return ret;
}

// Main program entrypoint
int main(int argc, char* argv[])
{
    // init
    Result rc = viOpenDefaultDisplay(&disp);
    if(R_FAILED(rc))
        fatalThrow(rc);
    rc = viGetDisplayVsyncEvent(&disp, &vsyncEvent);
    if(R_FAILED(rc))
        fatalThrow(rc);

    // Initialization code can go here.
    int frameCount = 0;
    FILE* f = NULL;

    // Configure our supported input layout: a single player with standard controller styles
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);


    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    PadState pad;
    padInitializeDefault(&pad);

    // Initialize the mouse
    hidInitializeMouse();

    // Your code / main loop goes here.
    // If you need threads, you can use threadCreate etc.
    while(true)
    {
        // Scan the gamepad. This should be done once for each frame
        padUpdate(&pad);
        u32 mouseDown = getMouseDown();
        if (mouseDown & pause_toggle)
        {
            if (getIsPaused())
            {
                detach();
                if (f != NULL)
                {
                    fclose(f);
                    f = NULL;
                }
            }
            else
            {
                attach();
                frameCount = 0;

                time_t unixTime = time(NULL);
                struct tm* timeStruct = localtime((const time_t *)&unixTime);
                std::stringstream ss;
                std::string filename;
                ss << "/scripts/pausenx/recording-" << timeStruct->tm_mon + 1 << "_" << timeStruct->tm_mday << "_" << timeStruct->tm_year + 1900 << "-" << timeStruct->tm_hour << "_" << timeStruct->tm_min << "_" << timeStruct->tm_sec << ".txt";
                ss >> filename;
                f = fopen(filename.c_str(), "w");
            }
        }
        else if((mouseDown & frame_advance_button) && getIsPaused())
        {
            // log controller state
            u64 kHeld = padGetButtons(&pad);
            std::string keyString = translateKeys(kHeld);

            HidAnalogStickState posLeft = padGetStickPos(&pad, 0);
            HidAnalogStickState posRight = padGetStickPos(&pad, 1);

            if(!(keyString == "NONE" && posLeft.x == 0 && posLeft.y == 0 && posRight.x == 0 && posRight.y == 0))
            {
                if (f != NULL)
                {
                    fprintf(f, "%i %s %d;%d %d;%d\n", frameCount, keyString.c_str(), posLeft.x, posLeft.y, posRight.x, posRight.y);
                }
            }

            advanceOneFrame();
            frameCount++;
        }
        
        rc = eventWait(&vsyncEvent, 0xFFFFFFFFFFF);
        if(R_FAILED(rc))
            fatalThrow(rc);
    }
    // Deinitialization and resources clean up code can go here.
    return 0;
}