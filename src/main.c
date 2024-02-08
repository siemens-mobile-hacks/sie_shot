#include <swilib.h>
#include <stdlib.h>
#include <sie/sie.h>

const int minus11=-11;
unsigned short maincsm_name_body[140];

unsigned int TAKING;
char DIR[] = "0:\\Pictures\\Screenshots\\";

typedef struct {
    CSM_RAM csm;
} MAIN_CSM;

unsigned char *GetScreenBuffer() {
    size_t size = CalcBitmapSize((short)ScreenW(), (short)ScreenH(), IMGHDR_TYPE_RGB565);
    unsigned char *buffer = malloc(size);
    memcpy(buffer, RamScreenBuffer(), size);
    return buffer;
}

png_bytepp ScreenBuffer2BytePP(unsigned const char *bitmap) {
    png_bytepp row_pointers = malloc(sizeof(png_bytepp) * ScreenH());
    for (int i = 0; i < ScreenH(); i++) {
        row_pointers[i] = malloc(sizeof(png_bytep) * ScreenW() * 3);
        for (int j = 0; j < ScreenW(); j++) {
            unsigned short rgb565 = *(unsigned short*)&bitmap[j * 2 + ScreenW() * i * 2];
            unsigned char r = (rgb565 & 0xF800) >> 11;
            unsigned char g = (rgb565 & 0x07E0) >> 5;
            unsigned char b = rgb565 & 0x001F;
            r = (r * 255) / 31;
            g = (g * 255) / 63;
            b = (b * 255) / 31;
            row_pointers[i][j * 3] = r;
            row_pointers[i][j * 3 + 1] = g;
            row_pointers[i][j * 3 + 2] = b;
        }
    }
    return row_pointers;
}

char *GetPath(const char *ext) {
    TDate date;
    TTime time;
    char *path = NULL;
    char file_name[64];
    GetDateTime(&date, &time);
    sprintf(file_name, "%02d_%02d_%02d-%02d_%02d_%02d.%s",
            date.day, date.month, (unsigned int)(date.year - 2000),
            time.hour, time.min, time.sec,
            ext);
    path = malloc(strlen(DIR) + strlen(file_name) + 1);
    strcpy(path, DIR);
    strcat(path, file_name);
    return path;
}

void TakeScreenShot_PNG(void *data) {
    png_structp png = NULL;
    png_infop info = NULL;
    png_bytepp row_pointers = data;

    char *path = GetPath("png");
    FILE *fp = fopen(path, "wb");
    mfree(path);

    unsigned int err = 1;
    if (fp) {
        png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                  NULL, NULL, NULL);
        if (png) {
            info = png_create_info_struct(png);
            if (info) {
                png_set_IHDR(png, info,
                             ScreenW(), ScreenH(),
                             8, PNG_COLOR_TYPE_RGB,
                             PNG_INTERLACE_NONE,
                             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
                png_init_io(png, fp);
                png_set_rows(png, info, row_pointers);
                png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
                err = 0;
            }
        }
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        Sie_GUI_MsgBoxOk("Screenshot saved", NULL);
    }
    for (int i = 0; i < ScreenH(); i++) {
        mfree(row_pointers[i]);
    }
    mfree(row_pointers);
    if (err) {
        Sie_GUI_MsgBoxError("Error saving screenshot", NULL);
    }
    TAKING = 0;
}

void TakeScreenShot() {
    if (!TAKING) {
        TAKING = 1;
        Sie_GUI_MsgBox("Taking screenshot...", NULL);
        unsigned char *bitmap = GetScreenBuffer();
        png_bytepp row_pointers = ScreenBuffer2BytePP(bitmap);
        mfree(bitmap);
        Sie_SubProc_Run(TakeScreenShot_PNG, row_pointers);
    } else {
        Sie_GUI_MsgBoxError("Screenshot is taking...", NULL);
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
            TakeScreenShot();
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

int maincsm_onmessage(CSM_RAM *data, GBS_MSG *msg) {
    return 1;
}

void maincsm_oncreate(CSM_RAM *data) {
    unsigned int err = 0;
    AddKeybMsgHook(KeyHook);
    if (Sie_FS_MMCardExists()) {
        DIR[0] = '4';
    }
    Sie_FS_CreateDirs(DIR, &err);
}

void KillElf() {
    extern void *__ex;
    elfclose(&__ex);
}

void maincsm_onclose(CSM_RAM *csm) {
    RemoveKeybMsgHook(KeyHook);
    SUBPROC((void *)KillElf);
}

const struct {
    CSM_DESC maincsm;
    WSHDR maincsm_name;
} MAINCSM = {
        {
                maincsm_onmessage,
                maincsm_oncreate,
#ifdef NEWSGOLD
                0,
                0,
                0,
                0,
#endif
                maincsm_onclose,
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
    wsprintf((WSHDR *)(&MAINCSM.maincsm_name),"SieShot");
}

int main() {
    CSM_RAM *save_cmpc;
    char dummy[sizeof(MAIN_CSM)];
    UpdateCSMname();
    LockSched();
    save_cmpc = CSM_root()->csm_q->current_msg_processing_csm;
    CSM_root()->csm_q->current_msg_processing_csm = CSM_root()->csm_q->csm.first;
    CreateCSM(&MAINCSM.maincsm,dummy,0);
    CSM_root()->csm_q->current_msg_processing_csm = save_cmpc;
    UnlockSched();
    return 0;
}
