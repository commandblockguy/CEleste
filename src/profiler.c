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

void profiler_print() {
    dbg_printf("Last frame (%u): %u FPS\n", profiler_frame_index - 1, 32768 / profiler_frames[profiler_frame_index - 1].total);
#define PROFILER_ENTRY(name, depth) dbg_printf("%.*s%s: %u.%03u ms\n", 2*(1+(depth)), "                    ", #name, as_decimal(profiler_frames[profiler_frame_index - 1].name / 33.0));
    PROFILER_ENTRIES
#undef PROFILER_ENTRY
    dbg_printf("Average of last 256 frames: %u FPS\n", (unsigned int)8388608 / profiler_sum.total);
#define PROFILER_ENTRY(name, depth) dbg_printf("%.*s%s: %u.%03u ms\n", 2*(1+(depth)), "                    ", #name, as_decimal(profiler_sum.name / 8388.608));
    PROFILER_ENTRIES
#undef PROFILER_ENTRY
}

#endif
