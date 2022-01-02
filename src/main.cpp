#include <keypadc.h>
#include <graphx.h>
#include <tice.h>
#include "emu.h"

int main(void)
{
    gfx_Begin();
    init();
    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    do {
        update();
    } while(!kb_IsDown(kb_KeyClear) && !kb_IsDown(kb_KeyDel));
    if(!kb_IsDown(kb_KeyDel)) {
        save_game();
    }
    gfx_End();
    return 0;
}
