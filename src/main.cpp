#include <keypadc.h>
#include <graphx.h>
#include <gfx/mypalette.h>
#include "emu.h"

int main(void)
{
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetPalette(mypalette, sizeof mypalette, 0);
    init();
    do {
        update();
    } while(!kb_IsDown(kb_KeyClear));
    gfx_End();
    return 0;
}
