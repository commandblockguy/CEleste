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

#define PROFILER_ENTRIES \
        PROFILER_ENTRY(total, 0) \
        PROFILER_ENTRY(draw, 1) \
        PROFILER_ENTRY(spr, 2) \
        PROFILER_ENTRY(map, 2) \
        PROFILER_ENTRY(tilemap_1, 3) \
        PROFILER_ENTRY(tilemap_2, 3) \
        PROFILER_ENTRY(tilemap_3, 3) \
        PROFILER_ENTRY(obj_draw, 2)  \
        PROFILER_ENTRY(player_draw, 3) \
        PROFILER_ENTRY(particles, 2) \
        PROFILER_ENTRY(clouds, 2) \
        PROFILER_ENTRY(deinterlace, 2) \
        PROFILER_ENTRY(update, 1) \
        PROFILER_ENTRY(move, 2) \
        PROFILER_ENTRY(move_x, 3) \
        PROFILER_ENTRY(move_y, 3) \
        PROFILER_ENTRY(collide_player, 2) \
        PROFILER_ENTRY(collide_other, 2) \
        PROFILER_ENTRY(obj_update, 2) \
        PROFILER_ENTRY(player_update, 3)


union profiler_set {
    struct {
#define PROFILER_ENTRY(name, depth) unsigned int name;
        PROFILER_ENTRIES
#undef PROFILER_ENTRY
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
