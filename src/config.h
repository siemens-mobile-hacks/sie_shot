#pragma once

#include <cfg_items.h>

enum {
    IMG_FORMAT_PNG,
    IMG_FORMAT_JPG,
};

#pragma pack(push, 1)
typedef struct {
    const CFG_HDR cfghdr_0;
    char dir[128];
    const CFG_HDR cfghdr_1;
    unsigned int img_format;
    const CFG_CBOX_ITEM cfgcbox_1[2];
    const CFG_HDR cfghdr_2;
    unsigned int hotkey;
} CONFIG;
#pragma pack(pop)

extern CONFIG CFG;
extern char CFG_PATH[];

void InitConfig();
