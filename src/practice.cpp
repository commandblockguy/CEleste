#include "practice.h"

#include <cstring>
#include <keypadc.h>
#include <graphx.h>
#include <tice.h>
#include <fontlibc.h>
#include "classic.h"
#include "emu.h"

bool practice_mode = false;
bool hud_active = true;
int current_time = 0;
int best_times[30];
uint8_t hud_erase = 0;

void practice_update() {
    if(is_title()) {
        if(kb_IsDown(kb_KeyGraph)) {
            practice_mode = true;
            load_room(0, 0);
            return;
        }
    }
    if(!practice_mode) return;
    if(player && level_index() != 30) current_time++;
    if(kb_IsDown(kb_KeyYequ)) {
//        while(kb_IsDown(kb_KeyYequ));
        prev_room();
    }
    if(kb_IsDown(kb_KeyGraph)) {
//        while(kb_IsDown(kb_KeyGraph));
        next_room();
        if(level_index() == 30) {
            hud_erase = 2;
        }
    }
    if(kb_IsDown(kb_KeyWindow)) {
        while(kb_IsDown(kb_KeyWindow));
        hud_active = !hud_active;
        if(!hud_active) {
            hud_erase = 2;
        }
    }
    if(kb_IsDown(kb_KeyZoom)) {
        if(!is_title() &&  level_index() > 21) {
            while(kb_IsDown(kb_KeyZoom));
            max_djump = max_djump == 2 ? 1 : 2;
            load_room(room.x, room.y);
        }
    }
}

void practice_on_player_spawn() {
    current_time = 0;
}

void practice_on_complete() {
    if(best_times[level_index()] == 0 || current_time < best_times[level_index()]) {
        best_times[level_index()] = current_time;
    }

    load_room(room.x, room.y);
}

void practice_draw_hud() {
    if(hud_erase) {
        gfx_SetColor(0);
        gfx_FillRectangle_NoClip(0, 0, 16, 29);
        gfx_FillRectangle_NoClip(LCD_WIDTH / 2, 0, 16, 29);
        hud_erase--;
        return;
    }
    if(!practice_mode) return;
    if(!hud_active) return;
    if(level_index() == 30) return;

    // Current run
    gfx_SetColor(0x00);
    gfx_FillRectangle_NoClip(0, 0, 26, 7);
    fontlib_SetForegroundColor(0x77);
    fontlib_SetCursorPosition(1, 1);
    if(current_time >= 100 * 30) {
        fontlib_DrawString("99.999");
    } else {
        fontlib_DrawUInt(current_time / 30, 2);
        fontlib_DrawGlyph('.');
        fontlib_DrawUInt((current_time % 30) * 1000 / 30, 3);
    }

    // Best time (frames)
    gfx_SetColor(0x00);
    gfx_FillRectangle_NoClip(0, 9, 16, 5);
    fontlib_SetForegroundColor(0xaa);
    fontlib_SetCursorPosition(1, 9);
    if(best_times[level_index()] < 1000) {
        fontlib_DrawUInt(best_times[level_index()], 3);
    } else {
        fontlib_DrawString("999");
    }

    // Keypad indicator
    const static struct {uint8_t x; uint8_t y;} locations[6] = {
            {0, 4}, // left
            {8, 4}, // right
            {4, 0}, // up
            {4, 4}, // down
            {2, 9}, // jump
            {6, 9}, // dash
    };

    for(int i = 0; i < 6; i++) {
        if(btn(i)) {
            gfx_SetColor(0x77);
        } else {
            gfx_SetColor(0x11);
        }
        gfx_FillRectangle_NoClip(1 + locations[i].x, 16 + locations[i].y, 3, 3);
    }

    for(int y = 0; y < 29; y++) {
        memcpy(&gfx_vbuffer[y][LCD_WIDTH / 2], &gfx_vbuffer[y][0], 16);
    }
}

int practice_get_total_time() {
    int total = 0;
    for(int best_time : best_times) {
        if(best_time == 0) return 0;
        total += best_time;
    }
    return total;
}
