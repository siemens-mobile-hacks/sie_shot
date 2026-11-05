#pragma once

#include <swilib.h>

enum {
    IPC_TAKE_SCREENSHOT,
    IPC_SECOND_LAUNCH,
};

void IPC_SendMessage(IPC_REQ *ipc, int submess);

extern const char *IPC_NAME;
