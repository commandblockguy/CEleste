#pragma once

#include <cstdint>
#include <cstdio>

#include "gfx/atlas.h"

void init(FILE *tas);
void update();
void save_game();

extern const uint8_t tilemap[64 * 128];

#define mget(x, y) tilemap[(x) + (y) * 128]

void print(const char *str);
void print(const char *str, int x, int y, uint8_t col);
void print(char chr, int x, int y, uint8_t col);
void print_int(int n, int x, int y, uint8_t col, int l);
void print_int(int n, int x, int y, uint8_t col);
void print_int(int n);
void print_int(int n, int l);

void rectfill(int x0, int y0, int x1, int y1, uint8_t col);
void circfill(int x, int y, int radius, int col);
void draw_plus(int x, int y, int col);
void vert_line(int x, int y0, int y1, int col);

void camera();
void camera(int x, int y);

bool fget(uint8_t tile, uint8_t flag);
void map(int cell_x, int cell_y, int sx, int sy, uint8_t layers);
void spr(uint8_t n, int x, int y);
void spr(uint8_t n, int x, int y, uint8_t w, uint8_t h, bool flip_x, bool flip_y);
void pal();
void pal(int a, int b);
void pal(int a, int b, int p);

bool btn(uint8_t index);

int rnd(int max);
int min(int a, int b);
int max(int a, int b);

void gen_lookups();

#define DEGREES_TO_ANGLE(deg) ((unsigned)((float)(deg) * (1 << (24 - 3)) / 45))

int fast_sin(unsigned angle);
#define sin(rot) (-fast_sin(rot))
#define cos(rot) fast_sin(DEGREES_TO_ANGLE(90) - (rot))

#define TRIG_SCALE 256
#define TRIG_PRECISION_BITS 8
#define TRIG_PRECISION (1 << TRIG_PRECISION_BITS)
