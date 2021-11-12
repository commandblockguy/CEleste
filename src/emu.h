#pragma once

#include <cstdint>

#include "gfx/atlas.h"

void init();
void update();
void render();

extern const uint8_t tilemap[64 * 128];

#define mget(x, y) tilemap[(x) + (y) * 128]

#define NUM_SPRITES (sizeof atlas_tiles_data / sizeof atlas_tiles_data[0])

void print(const char *str);
void print(const char *str, int x, int y, uint8_t col);
void print_int(int n, int x, int y, uint8_t col);
void print_int(int n);
void print_int(int n, int l);

void rectfill(int x0, int y0, int x1, int y1, uint8_t col);

void camera();
void camera(int x, int y);

bool fget(uint8_t tile, uint8_t flag);
void map(int cell_x, int cell_y, int sx, int sy, int cell_w, int cell_h, uint8_t layers);
void spr(uint8_t n, int x, int y, uint8_t w, uint8_t h, bool flip_x, bool flip_y);
void pal();
void pal(int a, int b);

bool btn(uint8_t index);

float rnd(float max);
float min(float a, float b);
float max(float a, float b);
