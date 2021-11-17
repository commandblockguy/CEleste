#pragma once

#include <cstdint>

typedef int subpixel;
#define SUBPIXEL_SCALE 256
#define SP(x) ((int)((x) * SUBPIXEL_SCALE))
#define PIX(x) ((int)((x) / SUBPIXEL_SCALE))

struct vec2i {
    int x;
    int y;
};

struct vec2s {
    subpixel x;
    subpixel y;
};

#define NUM_FRUITS 29
#define NUM_CLOUDS 16

struct Cloud {
    int x;
    int y;
    int spd;
    int w;
};

struct Particle {
    subpixel x;
    subpixel y;
    int s;
    subpixel spd;
    int off;
    int c;
};

struct DeadParticle {
    subpixel x;
    subpixel y;
    vec2s spd;
};

enum type {
    PLAYER_SPAWN = 1,
    SPRING = 18,
    BALLOON = 22,
    FALL_FLOOR = 23,
    SMOKE = 29,
    FRUIT = 26,
    FLY_FRUIT = 28,
    FAKE_WALL = 64,
    KEY = 8,
    CHEST = 20,
    PLATFORM = 11,
    PLATFORM_RIGHT = 12,
    MESSAGE = 86,
    BIG_CHEST = 96,
    FLAG = 118,
    PLAYER
};

class Player;

class Object {
public:
    Object(int x, int y);
    virtual ~Object();

    bool collideable;
    bool solids;
    uint8_t sprite;
    struct {bool x; bool y;} flip;
    int x;
    int y;
    struct {int x; int y; int w; int h;} hitbox;
    uint8_t type;

    struct {int x; int y; int size; } hair[5];

    struct vec2s spd;
    struct vec2s rem;

    virtual void update() {}
    virtual void move(subpixel ox, subpixel oy);
    virtual void draw();

    bool is_solid(int ox, int oy);
    bool is_ice(int ox, int oy);
    Object *collide(enum type type, int ox, int oy);
    Player *collide_player(int ox, int oy);
    bool check(enum type type, int ox,int oy);
    bool check_player(int ox,int oy);
    void move_x(int amount, int start);
    void move_y(int amount);
};

class Player : public Object {
public:
    bool p_jump;
    bool p_dash;
    int grace;
    int jbuffer;
    int djump;
    int dash_time;
    int dash_effect_time;
    struct vec2s dash_target;
    struct vec2s dash_accel;
    int spr_off;
    bool was_on_ground;

    Player(int x, int y);
    ~Player() override;
    void update() override;
    void draw() override;

    void kill();
};

class PlayerSpawn : public Object {
public:
    struct vec2i target;
    int state;
    int delay;

    PlayerSpawn(int x, int y);
    void update() override;
    void draw() override;
};

class Spring : public Object {
public:
    Spring(int x, int y);
    int hide_in;
    int hide_for;
    int delay;
    void update() override;
    void break_spring();
};

class Balloon : public Object {
public:
    Balloon(int x, int y);
    unsigned offset;
    uint8_t sprite_tmr;
    int timer;
    int start;
    void update() override;
    void draw() override;
};

class FallFloor : public Object {
public:
    FallFloor(int x, int y);
    int state;
    int delay;
    bool solid;
    void update() override;
    void draw() override;
    void break_floor();
};

class Smoke : public Object {
public:
    Smoke(int x, int y);
    int sprite_timer{0};
    void update() override;
};

class Fruit : public Object {
public:
    Fruit(int x, int y);
    int start;
    int off;
    void update() override;
};

class FlyFruit : public Object {
public:
    FlyFruit(int x, int y);
    bool fly;
    int start;
    int step;
    int off;
    int sfx_delay;
    void update() override;
    void draw() override;
};

class LifeUp : public Object {
public:
    LifeUp(int x, int y);
    int duration;
    int flash;
    void update() override;
    void draw() override;
};

class FakeWall : public Object {
public:
    FakeWall(int x, int y);
    void update() override;
    void draw() override;
};

class Key : public Object {
public:
    Key(int x, int y);
    void update() override;
};

class Chest : public Object {
public:
    Chest(int x, int y);
    int start;
    int timer;
    void update() override;
};

class Platform : public Object {
public:
    Platform(int x, int y, int dir);
    int dir;
    int last;
    void update() override;
    void draw() override;
};

class Message : public Object {
public:
    Message(int x, int y);
    unsigned index;
    void draw() override;
};

class BigChest : public Object {
    // heh heh
public:
    BigChest(int x, int y);
    int state;
    int timer;
    void draw() override;
};

class Orb : public Object {
public:
    Orb(int x, int y);
    void draw() override;
};

class Flag : public Object {
public:
    Flag(int x, int y);
    int score;
    bool show;
    void draw() override;
};

class RoomTitle : public Object {
public:
    RoomTitle(int x, int y);
    int delay;

    void draw() override;
};

void _init();
void _update();
void _draw();

void title_screen();
void begin_game();
int level_index();
bool is_title();
void load_room(uint8_t x, uint8_t y);
void restart_room();
void next_room();
void create_hair(Object *obj);
void set_hair_color(int djump);
void draw_hair(Object *obj, int facing);
void unset_hair_color();
Object *init_object(enum type type, int x, int y);
void destroy_object(Object *obj);
void draw_time(int x, int y);
bool solid_at(int x, int y, int w, int h);
bool ice_at(int x, int y, int w, int h);
bool tile_flag_at(int x, int y, int w, int h, uint8_t flag);
uint8_t tile_at(int x, int y);
bool spikes_at(int x, int y, int w, int h, subpixel xspd, subpixel yspd);
int clamp(int val, int a, int b);
int appr(int val, int target, int amount);
int sign(int v);
bool maybe();

extern int freeze;
