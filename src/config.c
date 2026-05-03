#include <swilib.h>
#include "config.h"

char CFG_PATH[] = "?:\\zbin\\etc\\SieShot.bcfg";

CONFIG CFG = {
    {CFG_STR_UTF8, "Output folder", 3, 127},
    "4:\\Pictures\\Screenshots\\",
    {CFG_CBOX, "Image format", 0, 2},
    IMG_FORMAT_PNG,
    {{"PNG"}, {"JPEG"}},
};

void InitConfig() {
    CFG_PATH[0] = BCFG_GetDefaultDisk();
    if (BCFG_LoadConfig(CFG_PATH, &CFG, sizeof(CONFIG)) == -1) {
        BCFG_SaveConfig(CFG_PATH, &CFG, sizeof(CONFIG));
    }
}
