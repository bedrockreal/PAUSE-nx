#include "util.h"

Handle debughandle = 0;

HiddbgHdlsHandle controllerHandle = {0};
HiddbgHdlsDeviceInfo controllerDevice = {0};
HiddbgHdlsState controllerState = {0};


u32 frameAdvanceWaitTimeNs = 1e7;

void attach()
{
    u64 pid = 0;
    Result rc = pmdmntGetApplicationProcessId(&pid);
    if (R_FAILED(rc) || pid == 0)
        return;

    detach();
    svcDebugActiveProcess(&debughandle, pid);
}

void detach(){
    if (getIsPaused())
    {
        svcCloseHandle(debughandle);
        debughandle = 0;
    }
}

inline bool getIsPaused()
{
    return debughandle != 0;
}

void advanceOneFrame()
{
    s32 cur_priority;
    svcGetThreadPriority(&cur_priority, CUR_THREAD_HANDLE);
    svcSetThreadPriority(CUR_THREAD_HANDLE, 10);
    
    u64 pid = 0;
    Result rc = pmdmntGetApplicationProcessId(&pid);
    if(R_FAILED(rc))
        fatalThrow(rc);

    rc = eventWait(&vsyncEvent, 0xFFFFFFFFFFF);
    if(R_FAILED(rc))
        fatalThrow(rc);

    svcCloseHandle(debughandle);
    svcSleepThread(frameAdvanceWaitTimeNs);

    rc = svcDebugActiveProcess(&debughandle, pid);
    if(R_FAILED(rc))
        fatalThrow(rc);

    svcSetThreadPriority(CUR_THREAD_HANDLE, cur_priority);
}