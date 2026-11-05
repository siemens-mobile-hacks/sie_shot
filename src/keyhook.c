#include <swilib.h>
#include "ipc.h"
#include "config.h"

static int KeyHook(int submsg, int msg) {
    static int flag = 0;
    if (submsg == CFG.hotkey) {
        if (msg == KEY_DOWN) {
            if (flag) {
                flag = 0;
                return KEYHOOK_NEXT;
            } else {
                return KEYHOOK_BREAK;
            }
        }
        else if (msg == LONG_PRESS) {
            static IPC_REQ ipc;
            IPC_SendMessage(&ipc, IPC_TAKE_SCREENSHOT);
            return KEYHOOK_BREAK;
        }
        else if (msg == KEY_UP) {
            flag = 1;
            GBS_SendMessage(MMI_CEPID, KEY_DOWN, GREEN_BUTTON);
            return KEYHOOK_NEXT;
        }
    }
    return KEYHOOK_NEXT;
}

void KeyHook_Register() {
    AddKeybMsgHook(KeyHook);
}

void KeyHook_Unregister() {
    RemoveKeybMsgHook(KeyHook);
}