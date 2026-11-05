#include <swilib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ipc.h"
#include "config.h"
#include "keyhook.h"

#define MSG_ID 0x9993

const int minus11 =- 11;
unsigned short maincsm_name_body[140];

typedef struct {
    CSM_RAM csm;
    IMGHDR *img;
    HObj hobj;
} MAIN_CSM;

void GetFileExt(char *ext) {
    switch (CFG.img_format) {
        case IMG_FORMAT_PNG:
            PNG:
            strcpy(ext, "png");
        break;
        case IMG_FORMAT_JPG:
            strcpy(ext, "jpg");
        break;
        default: goto PNG;
    }
}

WSHDR *GetFilePath() {
    TDate date;
    TTime time;
    char ext[8];
    char file_name[64];
    GetFileExt(ext);
    GetDateTime(&date, &time);
    sprintf(file_name, "%lu-%02d-%02d_%02d-%02d-%02d.%s",
            date.year, date.day, date.month,
            time.hour, time.min, time.sec,
            ext);
    WSHDR *ws = AllocWS(256);
    wsprintf(ws, "%s%s", CFG.dir, file_name);
    return ws;
}

int TakeScreenshot(MAIN_CSM *csm, const WSHDR *path_ws) {
    csm->img = malloc(sizeof(IMGHDR));
    zeromem(csm->img, sizeof(IMGHDR));
    csm->img->w = ScreenW();
    csm->img->h = ScreenH();
    csm->img->bpnum = IMGHDR_TYPE_RGB565;
    const size_t size = csm->img->w * csm->img->h * 2;
    csm->img->bitmap = malloc(size);
    memcpy(csm->img->bitmap, RamScreenBuffer(), size);

    uint32_t err;
    int status = 0;
    int uid = GetExtUidByFileName_ws(path_ws);
    csm->hobj = Obs_CreateObject(0x2D, uid, 2, MSG_ID, 1, 0, &err);
    if (!csm->hobj) {
        return 0;
    }
    Obs_SetUserPointer(csm->hobj, csm);
    status = Obs_SetInput_Memory(csm->hobj, 0, (void*)csm->img, (int)size);
    if (status != 0 && status < 0x8000) {
        Obs_DestroyObject(csm->hobj);
        return 0;
    }
    status = Obs_SetOutputImageSize(csm->hobj, (short)csm->img->w, (short)csm->img->h);
    if (status != 0 && status < 0x8000) {
        Obs_DestroyObject(csm->hobj);
        return 0;
    }
    status = Obs_SetScaling(csm->hobj, 0);
    if (status != 0 && status < 0x8000) {
        Obs_DestroyObject(csm->hobj);
        return 0;
    }
#ifdef NEWSGOLD
    status = Obs_SetOutput_File(csm->hobj, path_ws);
#else
    char path[256];
    ws_2str(path_ws, path, 255);
    status = Obs_SetOutput_File(csm->hobj, path);
#endif
    if (status != 0 && status < 0x8000) {
        Obs_DestroyObject(csm->hobj);
        return 0;
    }
    status = Obs_Start(csm->hobj);
    if (status != 0 && status < 0x8000) {
        Obs_DestroyObject(csm->hobj);
        return 0;
    }
    return 1;
}

void Obs_Converted_Handler(const HObj hobj) {
    ShowMSG(DIALOG_NORMAL | DIALOG_DUMMY_CSM, (int)"Screenshot saved");
    Obs_DestroyObject(hobj);
}

void Obs_Destroy_Handler(const HObj hobj, MAIN_CSM *csm) {
    if (csm->img) {
        mfree(csm->img->bitmap);
        mfree(csm->img);
        csm->img = NULL;
    }
    csm->hobj = 0;
}

OBSevent OBSeventHandlers[] = {
    {0x03, Obs_Converted_Handler},
    {OBS_EV_ConvDestroyed, Obs_Destroy_Handler},
    {OBS_EV_EndList, NULL},
};

int OnMessage(CSM_RAM *data, GBS_MSG *msg) {
    MAIN_CSM *csm = (MAIN_CSM*)data;
    if (msg->msg == MSG_RECONFIGURE_REQ) {
        if (strcmpi(CFG_PATH, msg->data0) == 0) {
            InitConfig();
            ShowMSG(1, (int)"SieShot config updated");
        }
    } else if (msg->msg == MSG_IPC) {
        const IPC_REQ *ipc = msg->data0;
        if (strcmpi(ipc->name_to, IPC_NAME) == 0) {
            if (msg->submess == IPC_SECOND_LAUNCH) {
                int ipc_csm_id = (int)ipc->data;
                if (csm->csm.id != ipc_csm_id) {
                    MAIN_CSM *new_csm = (MAIN_CSM*)FindCSMbyID(ipc_csm_id);
                    if (new_csm) {
                        new_csm->csm.state = CSM_STATE_CLOSED;
                        static IPC_REQ ipc_req;
                        IPC_SendMessage(&ipc_req, IPC_TAKE_SCREENSHOT);
                    }
                }
            } else if (msg->submess == IPC_TAKE_SCREENSHOT) {
                if (!csm->hobj) {
                    uint32_t err;
                    if (isdir(CFG.dir, &err) > 0) {
                        WSHDR *path = GetFilePath();
                        LockSched();
                        int success = TakeScreenshot(csm, path);
                        UnlockSched();
                        FreeWS(path);
                        if (!success) {
                            MsgBoxError(DIALOG_NORMAL | DIALOG_DUMMY_CSM, (int)"Can't take screenshot");
                        }
                    } else {
                        MsgBoxError(DIALOG_NORMAL | DIALOG_DUMMY_CSM, (int)"Folder not found");
                    }
                } else {
                    MsgBoxError(DIALOG_NORMAL | DIALOG_DUMMY_CSM, (int)"Screenshot in progress");
                }
            }
        }
    } else if (msg->msg == MSG_ID) {
        if (csm->hobj && csm->hobj == (HObj)msg->data0) {
            Obs_TranslateMessageGBS(msg, OBSeventHandlers);
        }
    }
    return 1;
}

void OnCreate(CSM_RAM *data) {
    MAIN_CSM *csm = (MAIN_CSM*)data;
    KeyHook_Register();
    static IPC_REQ ipc;
    ipc.data = (void*)csm->csm.id;
    IPC_SendMessage(&ipc, IPC_SECOND_LAUNCH);
}

void KillElf(MAIN_CSM *csm) {
    if (csm->hobj) {
        Obs_DestroyObject(csm->hobj);
    }
    kill_elf();
}

void OnClose(CSM_RAM *data) {
    MAIN_CSM *csm = (MAIN_CSM*)data;
    KeyHook_Unregister();
    SUBPROC(kill_elf, csm);
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
    wsprintf((WSHDR*)(&MAINCSM.maincsm_name), "SieShot");
}

int main() {
    MAIN_CSM main_csm;
    zeromem(&main_csm, sizeof(MAIN_CSM));
    UpdateCSMname();
    InitConfig();
    LockSched();
    CSM_RAM *save_cmpc = CSM_root()->csm_q->current_msg_processing_csm;
    CSM_root()->csm_q->current_msg_processing_csm = CSM_root()->csm_q->csm.first;
    CreateCSM(&MAINCSM.maincsm, &main_csm, 0);
    CSM_root()->csm_q->current_msg_processing_csm = save_cmpc;
    UnlockSched();
    return 0;
}
