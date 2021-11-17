#include <keypadc.h>
#include <graphx.h>
#include <gfx/mypalette.h>
#include <tice.h>
#include "emu.h"

int main(void)
{
    gfx_Begin();
    init();
    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    do {
        update();
    } while(!kb_IsDown(kb_KeyClear));
    gfx_End();
    return 0;
}
