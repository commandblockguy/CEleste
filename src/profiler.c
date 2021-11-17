#include "profiler.h"

#include <debug.h>
#include <string.h>

#if USE_PROFILER

union profiler_set current_profiler;
union profiler_set profiler_sum;
union profiler_set profiler_frames[256];

uint8_t profiler_frame_index;

void profiler_init() {
    timer_Set(2, 0);
    timer_Enable(2, TIMER_32K, TIMER_NOINT, TIMER_UP);
    profiler_frame_index = 0;
    memset(profiler_frames, 0, sizeof(profiler_frames));
    memset(&profiler_sum, 0, sizeof(profiler_sum));
}

void profiler_tick() {
    for(uint8_t i = 0; i < NUM_PROFILER_FIELDS; i++) {
        profiler_sum.array[i] -= profiler_frames[profiler_frame_index].array[i];
        profiler_sum.array[i] += current_profiler.array[i];
        profiler_frames[profiler_frame_index].array[i] = current_profiler.array[i];
        current_profiler.array[i] = 0;
    }
    profiler_frame_index++;
    timer_Set(2, 0);
}

#define as_decimal(x) (unsigned int)(x),((unsigned int)((x)*1000) % 1000)
#define profiler_field_last(name, depth) dbg_printf("%.*s%s: %u.%03u ms\n", 2*(1+(depth)), "                    ", #name, as_decimal(profiler_frames[profiler_frame_index - 1].name / 33.0))
#define profiler_field_average(name, depth) dbg_printf("%.*s%s: %u.%03u ms\n", 2*(1+(depth)), "                    ", #name, as_decimal(profiler_sum.name / 8388.608))

void profiler_print() {
    dbg_printf("Last frame (%u): %u FPS\n", profiler_frame_index - 1, 32768 / profiler_frames[profiler_frame_index - 1].total);
    profiler_field_last(total, 0);
    profiler_field_last(update, 0);
    profiler_field_last(draw, 0);
    profiler_field_last(spr, 0);
    profiler_field_last(map, 0);
    profiler_field_last(deinterlace, 0);
    profiler_field_last(tilemap_1, 0);
    profiler_field_last(tilemap_2, 0);
    profiler_field_last(tilemap_3, 0);
    profiler_field_last(obj_update, 0);
    profiler_field_last(obj_draw, 0);
    profiler_field_last(player_update, 0);
    profiler_field_last(player_draw, 0);
    profiler_field_last(particles, 0);
    profiler_field_last(clouds, 0);
    dbg_printf("Average of last 256 frames: %u FPS\n", (unsigned int)8388608 / profiler_sum.total);
    profiler_field_average(total, 0);
    profiler_field_average(update, 0);
    profiler_field_average(draw, 0);
    profiler_field_average(spr, 0);
    profiler_field_average(map, 0);
    profiler_field_average(deinterlace, 0);
    profiler_field_average(tilemap_1, 0);
    profiler_field_average(tilemap_2, 0);
    profiler_field_average(tilemap_3, 0);
    profiler_field_average(obj_update, 0);
    profiler_field_average(obj_draw, 0);
    profiler_field_average(player_update, 0);
    profiler_field_average(player_draw, 0);
    profiler_field_average(particles, 0);
    profiler_field_average(clouds, 0);
}

#endif
