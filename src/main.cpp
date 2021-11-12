#include <keypadc.h>
#include <graphx.h>
#include <gfx/mypalette.h>
#include <tice.h>
#include "emu.h"

int main(void)
{
    gfx_Begin();
    gfx_SetDrawBuffer();
    gfx_SetClipRegion((LCD_WIDTH - 128) / 2,
                      (LCD_HEIGHT - 128) / 2,
                      (LCD_WIDTH + 128) / 2,
                      (LCD_HEIGHT + 128) / 2);
    gfx_SetPalette(mypalette, sizeof mypalette, 0);
    init();
    do {
        update();
    } while(!kb_IsDown(kb_KeyClear));
    gfx_End();
    return 0;
}
