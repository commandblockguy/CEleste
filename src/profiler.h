#ifndef PROFILER_H
#define PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <tice.h>

#ifndef NDEBUG
#define USE_PROFILER 1
#endif

#if USE_PROFILER

union profiler_set {
    struct {
        unsigned int total;
        unsigned int update;
        unsigned int draw;
        unsigned int spr;
        unsigned int map;
        unsigned int tilemap_1;
        unsigned int tilemap_2;
        unsigned int tilemap_3;
        unsigned int obj_update;
        unsigned int obj_draw;
        unsigned int player_update;
        unsigned int player_draw;
        unsigned int particles;
        unsigned int clouds;
    };
    unsigned int array[0];
};

#define NUM_PROFILER_FIELDS (sizeof(union profiler_set) / sizeof(unsigned int))

#define profiler_add(name) (current_profiler.name = (unsigned int)timer_2_Counter - current_profiler.name)
#define profiler_end(name) (current_profiler.name = (unsigned int)timer_2_Counter - current_profiler.name)

void profiler_init(void);

void profiler_tick(void);

void profiler_print(void);

extern union profiler_set current_profiler;

#else

#define profiler_add(name)
#define profiler_end(name)

#define profiler_init()
#define profiler_tick()
#define profiler_print()

#endif

#ifdef __cplusplus
};
#endif

#endif //PROFILER_H
