#include <swilib.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "config.h"

const int minus11 =- 11;
unsigned short maincsm_name_body[140];

uint32_t TAKING;

typedef struct {
    CSM_RAM csm;
} MAIN_CSM;

void GetFilePath(char *path, const char *ext) {
    TDate date;
    TTime time;
    char file_name[64];
    GetDateTime(&date, &time);
    sprintf(file_name, "%lu-%02d-%02d_%02d-%02d-%02d.%s",
            date.year, date.day, date.month,
            time.hour, time.min, time.sec,
            ext);
    strcpy(path, CFG.dir);
    strcat(path, file_name);
}

void RGB565_to_RGB888(uint16_t pixel, uint8_t *dest) {
    uint8_t r = (pixel >> 11) & 0x1F;
    uint8_t g = (pixel >> 5) & 0x3F;
    uint8_t b = pixel & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    dest[0] = r;
    dest[1] = g;
    dest[2] = b;
}

uint8_t *TakeScreenshot(int screen_w, int screen_h) {
    const size_t size = screen_w * screen_h * 3;
    uint8_t *shot = malloc(size);
    uint8_t *pixels = RamScreenBuffer();
    for (int i = 0; i < screen_w * screen_h; i++) {
        const uint16_t pixel = pixels[i * 2] | (pixels[i * 2 + 1] << 8);
        RGB565_to_RGB888(pixel, shot + i * 3);
    }
    return shot;
}

void SaveScreenshot(int img_format) {
    uint32_t err;
    if (isdir(CFG.dir, &err) <= 0) {
        MsgBoxError(1, (int)"Output folder not found");
        TAKING = 0;
        return;
    }
    char ext[8];
    switch (img_format) {
        case IMG_FORMAT_PNG:
            strcpy(ext, "png");
        break;
        case IMG_FORMAT_JPG:
            strcpy(ext, "jpg");
        break;
        default:
            MsgBoxError(1, (int)"Unknown image format");
            TAKING = 0;
        return;
    }
    char path[256];
    GetFilePath(path, ext);
    const int w = ScreenW();
    const int h = ScreenH();
    uint8_t *shot = TakeScreenshot(w, h);
    ShowMSG(1, (int)"Taking screenshot...");
    int success = 0;
    if (img_format == IMG_FORMAT_PNG) {
        success = stbi_write_png(path, w, h, 3, shot, w * 3);
    } else {
        success = stbi_write_jpg(path, w, h, 3, shot, 100);
    }
    mfree(shot);
    if (success) {
        ShowMSG(1, (int)"Screenshot saved");
    } else {
        MsgBoxError(1, (int)"Error saving screenshot");
    }
    TAKING = 0;
}

void TakeScreenshot_Proc() {
    if (!TAKING) {
        TAKING = 1;
        SUBPROC(SaveScreenshot, CFG.img_format);
    } else {
        MsgBoxError(1, (int)"Screenshot already in progress");
    }
}

int KeyHook(int submsg, int msg) {
    static int flag = 0;
    if (submsg == GREEN_BUTTON) {
        if (msg == KEY_DOWN) {
            if (flag) {
                flag = 0;
                return KEYHOOK_NEXT;
            } else {
                return KEYHOOK_BREAK;
            }
        }
        else if (msg == LONG_PRESS) {
            TakeScreenshot_Proc();
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

int OnMessage(CSM_RAM *data, GBS_MSG *msg) {
    if (msg->msg == MSG_RECONFIGURE_REQ) {
        if (strcmpi(CFG_PATH, msg->data0) == 0) {
            InitConfig();
            ShowMSG(1, (int)"SieShot config updated!");
        }
    }
    return 1;
}

void OnCreate(CSM_RAM *data) {
    AddKeybMsgHook(KeyHook);
}

void OnClose(CSM_RAM *csm) {
    RemoveKeybMsgHook(KeyHook);
    SUBPROC(kill_elf);
}

const struct {
    CSM_DESC maincsm;
    WSHDR maincsm_name;
} MAINCSM = {
        {
                OnMessage,
                OnCreate,
#ifdef NEWSGOLD
                0,
                0,
                0,
                0,
#endif
                OnClose,
                sizeof(MAIN_CSM),
                1,
                &minus11
        },
        {
                maincsm_name_body,
                NAMECSM_MAGIC1,
                NAMECSM_MAGIC2,
                0x0,
                139,
                0
        }
};

void UpdateCSMname(void) {
    wsprintf((WSHDR *)(&MAINCSM.maincsm_name), "SieShot");
}

int main() {
    char dummy[sizeof(MAIN_CSM)];
    UpdateCSMname();
    InitConfig();
    LockSched();
    CSM_RAM *save_cmpc = CSM_root()->csm_q->current_msg_processing_csm;
    CSM_root()->csm_q->current_msg_processing_csm = CSM_root()->csm_q->csm.first;
    CreateCSM(&MAINCSM.maincsm,dummy,0);
    CSM_root()->csm_q->current_msg_processing_csm = save_cmpc;
    UnlockSched();
    return 0;
}
