#include "classic.h"

#include <TINYSTL/vector.h>
#include <debug.h>
#include <cstdlib>
#include <cstring>
#include <keypadc.h>
#include "emu.h"

// ~celeste~
// matt thorson + noel berry

// globals //
/////////////

struct vec2i room = {0, 0};

tinystl::vector<Object *> objects = {};
Cloud clouds[NUM_CLOUDS];

int freeze = 0;
int shake = 0;
bool will_restart = false;
int delay_restart = 0;
bool got_fruit[NUM_FRUITS];
bool has_dashed = false;
int sfx_timer = 0;
bool has_key = false;
bool pause_player = false;
bool flash_bg = false;
int music_timer = 0;
bool new_bg = false;

uint8_t k_left = 0;
uint8_t k_right = 1;
uint8_t k_up = 2;
uint8_t k_down = 3;
uint8_t k_jump = 4;
uint8_t k_dash = 5;

int frames;
int deaths;
int max_djump;
bool start_game;
int start_game_flash;
int seconds;
int minutes;

void _init() {
    for(auto &cloud: clouds) {
        cloud.x = rnd(128);
        cloud.y = rnd(128);
        cloud.spd = 1 + rnd(4);
        cloud.w = 32 + rnd(32);
    }

    title_screen();
}

void title_screen() {
    for(bool &i: got_fruit) {
        i = false;
    }
    frames = 0;
    deaths = 0;
    max_djump = 1;
    start_game = false;
    start_game_flash = 0;
    //music(40,0,7);

    load_room(7, 3);
}

void begin_game() {
    frames = 0;
    seconds = 0;
    minutes = 0;
    music_timer = 0;
    start_game = false;
    //music(0,0,7);
    load_room(0, 0);
}

int level_index() {
    return room.x % 8 + room.y * 8;
}


bool is_title() {
    return level_index() == 31;
}


// effects //
/////////////

// todo
// particles = {}
// for i=0,24 do
//     add(particles,{
//         x=rnd(128),
//         y=rnd(128),
//         s=0+flr(rnd(5)/4),
//         spd=0.25+rnd(5),
//         off=rnd(1),
//         c=6+flr(0.5+rnd(1))
//     })
// }

// dead_particles = {}

// player entity //
///////////////////

Player::Player(int x, int y) : Object(x, y) {
    dbg_printf("Player initialized at %i %i\n", x, y);
    p_jump = false;
    p_dash = false;
    grace = 0;
    jbuffer = 0;
    djump = max_djump;
    dash_time = 0;
    dash_effect_time = 0;
    dash_target = {.x=0, .y=0};
    dash_accel = {.x=0, .y=0};
    hitbox = {.x=1, .y=3, .w=6, .h=5};
    spr_off = 0;
    was_on_ground = false;
    type = player;
    create_hair(this);
}

void Player::update() {
    if(pause_player) return;

    int input = btn(k_right) ? 1 : (btn(k_left) ? -1 : 0);

    // spikes collide
    if(spikes_at(x + hitbox.x, y + hitbox.y, hitbox.w, hitbox.h, spd.x, spd.y)) {
        kill();
    }

    // bottom death
    if(y > 128) {
        kill();
    }

    bool on_ground = is_solid(0, 1);
    bool on_ice = is_ice(0, 1);

    // smoke particles
    if(on_ground and not was_on_ground) {
        new Smoke(x, y + 4);
    }

    bool jump = btn(k_jump) and not p_jump;
    p_jump = btn(k_jump);
    if(jump) {
        jbuffer = 4;
    } else if(jbuffer > 0) {
        jbuffer -= 1;
    }

    bool dash = btn(k_dash) and not p_dash;

    p_dash = btn(k_dash);

    if(on_ground) {
        grace = 6;
        if(djump < max_djump) {
            //psfx(54);
            djump = max_djump;
        }
    } else if(grace > 0) {
        grace -= 1;
    }

    dash_effect_time -= 1;
    if(dash_time > 0) {
        new Smoke(x, y);
        dash_time -= 1;
        spd.x = appr(spd.x, dash_target.x, dash_accel.x);
        spd.y = appr(spd.y, dash_target.y, dash_accel.y);
    } else {

        // move
        subpixel maxrun = SP(1);
        subpixel accel = SP(0.6);
        subpixel deccel = SP(0.15);

        if(not on_ground) {
            accel = SP(0.4);
        } else if(on_ice) {
            accel = SP(0.05);
            if(input == (flip.x ? -1 : 1)) {
                accel = SP(0.05);
            }
        }

        if(abs(spd.x) > maxrun) {
            spd.x = appr(spd.x, sign(spd.x) * maxrun, deccel);
        } else {
            spd.x = appr(spd.x, input * maxrun, accel);
        }

        //facing
        if(spd.x != 0) {
            flip.x = (spd.x < 0);
        }

        // gravity
        subpixel maxfall = SP(2);
        subpixel gravity = SP(0.21);

        if(abs(spd.y) <= SP(0.15)) {
            gravity = SP(0.21 * 0.5);
        }

        // wall slide
        if(input != 0 and is_solid(input, 0) and not is_ice(input, 0)) {
            maxfall = SP(0.4);
            if(rnd(10) < 2) {
                new Smoke(x + input * 6, y);
            }
        }

        if(not on_ground) {
            spd.y = appr(spd.y, maxfall, gravity);
        }

        // jump
        if(jbuffer > 0) {
            if(grace > 0) {
                // normal jump
                //psfx(1);
                jbuffer = 0;
                grace = 0;
                spd.y = SP(-2);
                new Smoke(x, y + 4);
            } else {
                // wall jump
                int wall_dir = (is_solid(-3, 0) ? -1 : is_solid(3, 0) ? 1 : 0);
                if(wall_dir != 0) {
                    //psfx(2);
                    jbuffer = 0;
                    spd.y = SP(-2);
                    spd.x = -wall_dir * (maxrun + SP(1));
                    if(not is_ice(wall_dir * 3, 0)) {
                        new Smoke(x + wall_dir * 6, y);
                    }
                }
            }
        }

        // dash
        subpixel d_full = SP(5);
        subpixel d_half = SP(5 * 0.70710678118);

        if(djump > 0 and dash) {
            new Smoke(x, y);
            djump -= 1;
            dash_time = 4;
            has_dashed = true;
            dash_effect_time = 10;
            int v_input = (btn(k_up) ? -1 : (btn(k_down) ? 1 : 0));
            if(input != 0) {
                if(v_input != 0) {
                    spd.x = input * d_half;
                    spd.y = v_input * d_half;
                } else {
                    spd.x = input * d_full;
                    spd.y = 0;
                }
            } else if(v_input != 0) {
                spd.x = 0;
                spd.y = v_input * d_full;
            } else {
                spd.x = SP(flip.x ? -1 : 1);
                spd.y = 0;
            }

            //psfx(3);
            freeze = 2;
            shake = 6;
            dash_target.x = SP(2) * sign(spd.x);
            dash_target.y = SP(2) * sign(spd.y);
            dash_accel.x = SP(1.5);
            dash_accel.y = SP(1.5);

            if(spd.y < 0) {
                dash_target.y = dash_target.y * 3 / 4;
            }

            if(spd.y != 0) {
                dash_accel.x = SP(1.5 * 0.70710678118);
            }
            if(spd.x != 0) {
                dash_accel.y = SP(15 * 0.70710678118);
            }
        } else if(dash and djump <= 0) {
            //psfx(9);
            new Smoke(x, y);
        }

    }

    // animation
    spr_off += 1;
    if(not on_ground) {
        if(is_solid(input, 0)) {
            sprite = 5;
        } else {
            sprite = 3;
        }
    } else if(btn(k_down)) {
        sprite = 6;
    } else if(btn(k_up)) {
        sprite = 7;
    } else if((spd.x == 0) or (not btn(k_left) and not btn(k_right))) {
        sprite = 1;
    } else {
        sprite = 1 + (spr_off / 4) % 4;
    }

    // was on the ground
    was_on_ground = on_ground;

    // next level
    if(y < -4 and level_index() < 30) { next_room(); }
}

void Player::draw() {
    // clamp in screen
    if(x < -1 or x > 121) {
        x = clamp(x, -1, 121);
        spd.x = 0;
    }

    set_hair_color(djump);
    draw_hair(this, flip.x ? -1 : 1);
    spr(sprite, x, y, 1, 1, flip.x, flip.y);
    unset_hair_color();
}

//void psfx(int num) {
//    if(sfx_timer <= 0) {
//        //sfx(num)
//    }
//}

void create_hair(Object *obj) {
    for(int i = 0; i <= 4; i++) {
        obj->hair[i].x = obj->x;
        obj->hair[i].y = obj->y;
        obj->hair[i].size = max(1, min(2, 3 - i));
    }
}

void set_hair_color(int djump) {
    // todo: floor?
    pal(8, (djump == 1 ? 8 : djump == 2 ? (7 + ((frames / 3) % 2) * 4) : 12));
}

void draw_hair(Object *obj, int facing) {
    struct vec2i last = {.x=obj->x + 4 - facing * 2, .y=obj->y + (btn(k_down) ? 4 : 3)};
    for(int i = 0; i < 5; i++) {
        obj->hair[i].x += (last.x - obj->hair[i].x) / 1.5;
        obj->hair[i].y += (last.y + 0.5 - obj->hair[i].y) / 1.5;
        circfill(obj->hair[i].x, obj->hair[i].y, obj->hair[i].size, 8);
        last.x = obj->hair[i].x;
        last.y = obj->hair[i].y;
    }
}

void unset_hair_color() {
    pal(8, 8);
}

PlayerSpawn::PlayerSpawn(int x, int y) : Object(x, y) {
    //sfx(4);
    sprite = 3;
    target = {.x=x, .y=y};
    y = 128;
    spd.y = SP(-4);
    state = 0;
    delay = 0;
    solids = false;
    type = player_spawn;
    create_hair(this);
}

void PlayerSpawn::update() {
    // jumping up
    if(state == 0) {
        if(y < target.y + 16) {
            state = 1;
            delay = 3;
        }
        // falling
    } else if(state == 1) {
        spd.y += SP(0.5);
        if(spd.y > 0 and delay > 0) {
            spd.y = 0;
            delay -= 1;
        }
        if(spd.y > 0 and y > target.y) {
            y = target.y;
            spd = {.x=0, .y=0};
            state = 2;
            delay = 5;
            shake = 5;
            new Smoke(x, y + 4);
            //sfx(5);
        }
        // landing
    } else if(state == 2) {
        delay -= 1;
        sprite = 6;
        if(delay < 0) {
            new Player(x, y);
            delete this;
        }
    }
}

void PlayerSpawn::draw() {
    set_hair_color(max_djump);
    draw_hair(this, 1);
    spr(sprite, x, y, 1, 1, flip.x, flip.y);
    unset_hair_color();
}

Spring::Spring(int x, int y) : Object(x, y) {
    hide_in = 0;
    hide_for = 0;
    type = spring;
    sprite = spring;
}

void Spring::update() {
    if(hide_for > 0) {
        hide_for -= 1;
        if(hide_for <= 0) {
            sprite = 18;
            delay = 0;
        }
    } else if(sprite == 18) {
        auto hit = (Player *) collide(player, 0, 0);
        if(hit != nullptr and hit->spd.y >= 0) {
            sprite = 19;
            hit->y = y - 4;
            hit->spd.x /= 5;
            hit->spd.y = SP(-3);
            hit->djump = max_djump;
            delay = 10;
            new Smoke(x, y);

            // breakable below us
            auto below = (FallFloor *) collide(fall_floor, 0, 1);
            if(below != nullptr) {
                below->break_floor();
            }

            //psfx(8);
        }
    } else if(delay > 0) {
        delay -= 1;
        if(delay <= 0) {
            sprite = 18;
        }
    }
    // begin hiding
    if(hide_in > 0) {
        hide_in -= 1;
        if(hide_in <= 0) {
            hide_for = 60;
            sprite = 0;
        }
    }
}

void Spring::break_spring() {
    hide_in = 15;
}

Balloon::Balloon(int x, int y) : Object(x, y) {
    //offset=rnd(1);
    start = y;
    timer = 0;
    hitbox = {.x=-1, .y=-1, .w=10, .h=10};
    type = balloon;
    sprite = balloon;
}

void Balloon::update() {
    if(sprite == 22) {
        // todo
        //offset+=0.01;
        //y=start+sin(offset)*2;
        auto hit = (Player *) collide(player, 0, 0);
        if(hit != nullptr and hit->djump < max_djump) {
            //psfx(6);
            new Smoke(x, y);
            hit->djump = max_djump;
            sprite = 0;
            timer = 60;
        }
    } else if(timer > 0) {
        timer -= 1;
    } else {
        //psfx(7)
        new Smoke(x, y);
        sprite = 22;
    }
}

void Balloon::draw() {
    if(sprite == 22) {
        spr(13 + (offset * 8) % 3, x, y + 6);
        spr(sprite, x, y);
    }
}

FallFloor::FallFloor(int x, int y) : Object(x, y) {
    state = 0;
    solid = true;
    type = fall_floor;
}

void FallFloor::update() {
    // idling
    if(state == 0) {
        if(check(player, 0, -1) or check(player, -1, 0) or check(player, 1, 0)) {
            break_floor();
        }
        // shaking
    } else if(state == 1) {
        delay -= 1;
        if(delay <= 0) {
            state = 2;
            delay = 60;//how long it hides for
            collideable = false;
        }
        // invisible, waiting to reset
    } else if(state == 2) {
        delay -= 1;
        if(delay <= 0 and not check(player, 0, 0)) {
            //psfx(7);
            state = 0;
            collideable = true;
            new Smoke(x, y);
        }
    }
}

void FallFloor::draw() {
    if(state != 2) {
        if(state != 1) {
            spr(23, x, y);
        } else {
            spr(23 + (15 - delay) / 5, x, y);
        }
    }
}

void FallFloor::break_floor() {
    if(state == 0) {
        //psfx(15)
        state = 1;
        delay = 15;//how long until it falls
        new Smoke(x, y);
        auto hit = (Spring *) collide(spring, 0, -1);
        if(hit != nullptr) {
            hit->break_spring();
        }
    }
}

Smoke::Smoke(int x, int y) : Object(x, y) {
    sprite = 29;
    spd.y = SP(-0.1);
    spd.x = SP(0.3) + rnd(SP(0.2));
    this->x += -1 + rnd(2);
    this->y += -1 + rnd(2);
    flip.x = maybe();
    flip.y = maybe();
    solids = false;
    type = smoke;
}

void Smoke::update() {
    if(++sprite_timer == 5) {
        sprite_timer = 0;
        sprite++;
    }
    if(sprite >= 32) {
        delete this;
    }
}

Fruit::Fruit(int x, int y) : Object(x, y) {
    start = y;
    off = 0;
    sprite = 26;
    type = fruit;
}

void Fruit::update() {
    auto hit = (Player *) collide(player, 0, 0);
    if(hit != nullptr) {
        hit->djump = max_djump;
        sfx_timer = 20;
        //sfx(13);
        got_fruit[level_index()] = true;
        new LifeUp(x, y);
        delete this;
    } else {
        off += 1;
        y = start;
        // todo: y = start + sin(off / 40) * 5 / 2;
    }
}

FlyFruit::FlyFruit(int x, int y) : Object(x, y) {
    start = y;
    fly = false;
    step = 0.5;
    solids = false;
    sfx_delay = 8;
    type = fly_fruit;
    sprite = fly_fruit;
}

void FlyFruit::update() {
    //fly away
    if(fly) {
        if(sfx_delay > 0) {
            sfx_delay -= 1;
            if(sfx_delay <= 0) {
                sfx_timer = 20;
                //sfx(14);
            }
        }
        spd.y = appr(spd.y, SP(-3.5), SP(0.25));
        if(y < -16) {
            delete this;
        }
        // wait
    } else {
        if(has_dashed) {
            fly = true;
        }
        step += 0.05;
        // todo
        //spd.y=sin(step)*0.5;
    }
    // collect
    auto hit = (Player *) collide(player, 0, 0);
    if(hit != nullptr) {
        hit->djump = max_djump;
        sfx_timer = 20;
        //sfx(13)
        got_fruit[level_index()] = true;
        new LifeUp(x, y);
        delete this;
    }
}

void FlyFruit::draw() {
    int off = 0;
    if(not fly) {
        // todo
//        int dir = sin(step);
//        if(dir < 0) {
//            off = 1 + max(0, sign(y - start));
//        }
    } else {
//        off=(off+0.25)%3;
    }
    spr(45 + off, x - 6, y - 2, 1, 1, true, false);
    spr(sprite, x, y);
    spr(45 + off, x + 6, y - 2);
}

LifeUp::LifeUp(int x, int y) : Object(x, y) {
    spd.y = SP(-0.25);
    duration = 30;
    x -= 2;
    y -= 4;
    flash = 0;
    solids = false;
}

void LifeUp::update() {
    duration -= 1;
    if(duration <= 0) {
        delete this;
    }
}

void LifeUp::draw() {
    flash += 1;

    print("1000", x - 2, y, 7 + (flash / 2) % 2);
}

FakeWall::FakeWall(int x, int y) : Object(x, y) {
    type = fake_wall;
};

void FakeWall::update() {
    hitbox = {.x=-1, .y=-1, .w=18, .h=18};
    auto *hit = (Player *) collide(player, 0, 0);
    if(hit != nullptr and hit->dash_effect_time > 0) {
        hit->spd.x = -sign(hit->spd.x) * 3 / 2;
        hit->spd.y = SP(-1.5);
        hit->dash_time = -1;
        sfx_timer = 20;
        //sfx(16);
        new Smoke(x, y);
        new Smoke(x + 8, y);
        new Smoke(x, y + 8);
        new Smoke(x + 8, y + 8);
        new Fruit(x + 4, y + 4);
        delete this;
    }
    hitbox = {.x=0, .y=0, .w=16, .h=16};
}

void FakeWall::draw() {
    spr(64, x, y);
    spr(65, x + 8, y);
    spr(80, x, y + 8);
    spr(81, x + 8, y + 8);
}

Key::Key(int x, int y) : Object(x, y) {
    type = key;
    sprite = key;
}

void Key::update() {
    int was = sprite;
    // todo
    //sprite=9+(sin(frames/30)+0.5)*1;
    int is = sprite;
    if(is == 10 and is != was) {
        flip.x = not flip.x;
    }
    if(check(player, 0, 0)) {
        //sfx(23)
        sfx_timer = 10;
        has_key = true;
        delete this;
    }
}

Chest::Chest(int x, int y) : Object(x, y) {
    x -= 4;
    start = x;
    timer = 20;
    type = chest;
    sprite = chest;
}

void Chest::update() {
    if(has_key) {
        timer -= 1;
        x = start - 1 + rnd(3);
        if(timer <= 0) {
            sfx_timer = 20;
            //sfx(16);
            new Fruit(x, y - 4);
            delete this;
        }
    }
}

Platform::Platform(int x, int y, int dir) : Object(x, y) {
    x -= 4;
    solids = false;
    hitbox.w = 16;
    last = x;
    this->dir = dir;
    type = platform;
}

void Platform::update() {
    spd.x = dir * SP(0.65);
    if(x < -16) {
        x = 128;
    } else if(x > 128) {
        x = -16;
    }
    if(not check(player, 0, 0)) {
        Object *hit = collide(player, 0, -1);
        if(hit != nullptr) {
            hit->move_x(x - last, 1);
        }
    }
    last = x;
}

void Platform::draw() {
    spr(11, x, y - 1);
    spr(12, x + 8, y - 1);
}

Message::Message(int x, int y) : Object(x, y) {
    type = message;
}

void Message::draw() {
    const char *text = "-- celeste mountain --#this memorial to those# perished on the climb";
    if(check(player, 4, 0)) {
        if(index / 2 < strlen(text)) {
            index += 1;
            // sfx deleted here
        }
        vec2i off = {x = 8, y = 96};
        for(unsigned i = 0; i < index / 2; i++) {
            if(text[i] != '#') {
                rectfill(off.x - 2, off.y - 2, off.x + 7, off.y + 6, 7);
                print(text[i], off.x, off.y, 0);
                off.x += 5;
            } else {
                off.x = 8;
                off.y += 7;
            }
        }
    } else {
        index = 0;
    }
}

BigChest::BigChest(int x, int y) : Object(x, y) {
    state = 0;
    hitbox.w = 16;
    type = big_chest;
}

void BigChest::draw() {
    if(state == 0) {
        auto hit = (Player *) collide(player, 0, 8);
        if(hit != nullptr and hit->is_solid(0, 1)) {
            //music(-1,500,7);
            //sfx(37);
            pause_player = true;
            hit->spd.x = 0;
            hit->spd.y = 0;
            state = 1;
            new Smoke(x, y);
            new Smoke(x + 8, y);
            timer = 60;
            //particles={};
        }
        spr(96, x, y);
        spr(97, x + 8, y);
    } else if(state == 1) {
        timer -= 1;
        shake = 5;
        flash_bg = true;
//        if (timer<=45 and count(particles)<50) {
//            add(particles,{
//                x=1+rnd(14),
//                y=0,
//                h=32+rnd(32),
//                spd=8+rnd(8)
//            })
//        }
        if(timer < 0) {
            state = 2;
            //particles={};
            flash_bg = false;
            new_bg = true;
            new Orb(x + 4, y + 4);
            pause_player = false;
        }
//        foreach(particles,function(p)
//            p.y+=p.spd
//            line(x+p.x,y+8-p.y,x+p.x,min(y+8-p.y+p.h,y+8),7)
//        })
    }
    spr(112, x, y + 8);
    spr(113, x + 8, y + 8);
}

Orb::Orb(int x, int y) : Object(x, y) {
    spd.y = -4;
    solids = false;
    //particles={}
}

void Orb::draw() {
    spd.y = appr(spd.y, 0, SP(0.5));
    auto hit = (Player *) collide(player, 0, 0);
    if(spd.y == 0 and hit != nullptr) {
        music_timer = 45;
        //sfx(51);
        freeze = 10;
        shake = 10;
        max_djump = 2;
        hit->djump = 2;
        delete this;
    } else {
        spr(102, x, y);
        // todo
//      float off=frames/30;
//      for (int i=0; i <= 7; i++) {
//          circfill(x+4+cos(off+i/8)*8,y+4+sin(off+i/8)*8,1,7)
//      }
    }
}

Flag::Flag(int x, int y) : Object(x, y) {
    x += 5;
    score = 0;
    show = false;
    for(bool i: got_fruit) {
        if(i) {
            score += 1;
        }
    }
    type = flag;
}

void Flag::draw() {
    sprite = 118 + (frames / 5) % 3;
    spr(sprite, x, y);
    if(show) {
        rectfill(32, 2, 96, 31, 0);
        spr(26, 55, 6);
        print("x", 64, 9, 7);
        print_int(score);
        draw_time(49, 16);
        print("deaths:", 48, 24, 7);
        print_int(deaths);
    } else if(check(player, 0, 0)) {
        //sfx(55);
        sfx_timer = 30;
        show = true;
    }
}

RoomTitle::RoomTitle(int x, int y) : Object(x, y) {
    delay = 5;
}

void RoomTitle::draw() {
    delay -= 1;
    if(delay < -30) {
        delete this;
    } else if(delay < 0) {

        rectfill(24, 58, 104, 70, 0);

        if(room.x == 3 and room.y == 1) {
            print("old site", 48, 62, 7);
        } else if(level_index() == 30) {
            print("summit", 52, 62, 7);
        } else {
            int level = (1 + level_index()) * 100;
            print_int(level, ((level < 1000) ? 54 : 52), 62, 7);
            print(" m");
        }
        //print("---",86,64-2,13)

        draw_time(4, 4);
    }
}

// object functions //
///////////////////////

Object *init_object(type type, int x, int y) {
    bool has_fruit = got_fruit[level_index()];
    // todo: populate
    switch(type) {
        case player_spawn:
            return new PlayerSpawn(x, y);
        case spring:
            return new Spring(x, y);
        case balloon:
            return new Balloon(x, y);
        case fall_floor:
            return new FallFloor(x, y);
        case smoke:
            return new Smoke(x, y);
        case fruit:
            return has_fruit ? nullptr : new Fruit(x, y);
        case fly_fruit:
            return has_fruit ? nullptr : new FlyFruit(x, y);
        case fake_wall:
            return has_fruit ? nullptr : new FakeWall(x, y);
        case key:
            return has_fruit ? nullptr : new Key(x, y);
        case chest:
            return has_fruit ? nullptr : new Chest(x, y);
        case platform:
            return new Platform(x, y, -1);
        case platform_right:
            return new Platform(x, y, 1);
        case message:
            return new Message(x, y);
        case big_chest:
            return new BigChest(x, y);
        case flag:
            return new Flag(x, y);
        default:
            return nullptr;
    }
}

Object::Object(int x, int y) {
    collideable = true;
    solids = true;

    sprite = 0; // todo
    flip = {.x=false, .y=false};

    this->x = x;
    this->y = y;
    hitbox = {.x=0, .y=0, .w=8, .h=8};

    spd = {.x=0, .y=0};
    rem = {.x=0, .y=0};

    objects.push_back(this);
}


bool Object::is_solid(int ox, int oy) {
    if(oy > 0 and not check(platform, ox, 0) and check(platform, ox, oy)) {
        return true;
    }
    return solid_at(x + hitbox.x + ox, y + hitbox.y + oy, hitbox.w, hitbox.h)
           or check(fall_floor, ox, oy)
           or check(fake_wall, ox, oy);
}


bool Object::is_ice(int ox, int oy) {
    return ice_at(x + hitbox.x + ox, y + hitbox.y + oy, hitbox.w, hitbox.h);
}

Object *Object::collide(enum type type, int ox, int oy) {
    for(Object *other: objects) {
        if(other != nullptr and other->type == type and other != this and other->collideable and
           other->x + other->hitbox.x + other->hitbox.w > x + hitbox.x + ox and
           other->y + other->hitbox.y + other->hitbox.h > y + hitbox.y + oy and
           other->x + other->hitbox.x < x + hitbox.x + hitbox.w + ox and
           other->y + other->hitbox.y < y + hitbox.y + hitbox.h + oy) {
            return other;
        }
    }
    return nullptr;
}

bool Object::check(enum type type, int ox, int oy) {
    return collide(type, ox, oy) != nullptr;
}

void Object::move(subpixel ox, subpixel oy) {
    int amount;
    // [x] get move amount
    rem.x += ox;
    amount = PIX(rem.x + SP(0.5));
    rem.x -= SP(amount);
    move_x(amount, 0);

    // [y] get move amount
    rem.y += oy;
    amount = PIX(rem.y + SP(0.5));
    rem.y -= SP(amount);
    move_y(amount);
}

void Object::move_x(int amount, int start) {
    if(solids) {
        int step = sign(amount);
        for(int i = start; i <= abs(amount); i++) {
            if(not is_solid(step, 0)) {
                x += step;
            } else {
                spd.x = 0;
                rem.x = 0;
                break;
            }
        }
    } else {
        x += amount;
    }
}

void Object::move_y(int amount) {
    if(solids) {
        int step = sign(amount);
        for(int i = 0; i <= abs(amount); i++) {
            if(not is_solid(0, step)) {
                y += step;
            } else {
                spd.y = 0;
                rem.y = 0;
                break;
            }
        }
    } else {
        y += amount;
    }
}

Object::~Object() {
    for(auto i = objects.begin(); i != objects.end(); i++) {
        if(*i == this) {
            objects.erase(i);
            return;
        }
    }
}

void Player::kill() {
    sfx_timer = 12;
    //sfx(0);
    deaths += 1;
    shake = 10;
    delete this;
//    dead_particles={};
//    for(int dir=0; dir <= 7; dir++) {
//        float angle=(dir/8.0);
//        add(dead_particles,{
//            x=obj->x+4,
//            y=obj->y+4,
//            t=10,
//            spd={
//                x=sin(angle)*3,
//                y=cos(angle)*3
//            }
//        })
//    }
    restart_room();
}

// room functions //
////////////////////


void restart_room() {
    will_restart = true;
    delay_restart = 15;
}


void next_room() {
    if(room.x == 2 and room.y == 1) {
        //music(30,500,7)
    } else if(room.x == 3 and room.y == 1) {
        //music(20,500,7)
    } else if(room.x == 4 and room.y == 2) {
        //music(30,500,7)
    } else if(room.x == 5 and room.y == 3) {
        //music(30,500,7)
    }

    if(room.x == 7) {
        load_room(0, room.y + 1);
    } else {
        load_room(room.x + 1, room.y);
    }
}

void load_room(uint8_t x, uint8_t y) {
    dbg_printf("loading room %u, %u\n", x, y);
    has_dashed = false;
    has_key = false;

    //remove existing objects
    while(!objects.empty()) {
        delete objects[0];
    }

    //current room
    room.x = x;
    room.y = y;

    // entities
    for(uint8_t tx = 0; tx <= 15; tx++) {
        for(uint8_t ty = 0; ty <= 15; ty++) {
            uint8_t tile = mget(room.x * 16 + tx, room.y * 16 + ty);
            init_object((type) (tile), tx * 8, ty * 8);
        }
    }

    if(!is_title()) {
        new RoomTitle(0, 0);
    }
}

// update function //
///////////////////////

void _update() {
    frames = ((frames + 1) % 30);
    if(frames == 0 and level_index() < 30) {
        seconds = ((seconds + 1) % 60);
        if(seconds == 0) {
            minutes += 1;
        }
    }

    if(music_timer > 0) {
        music_timer -= 1;
        if(music_timer <= 0) {
            //music(10, 0, 7);
        }
    }

    if(sfx_timer > 0) {
        sfx_timer -= 1;
    }

    // cancel if freeze
    if(freeze > 0) {
        freeze -= 1;
        return;
    }

    // screenshake
    if(shake > 0) {
        shake -= 1;
        camera();
        if(shake > 0) {
            camera(-2 + rnd(5), -2 + rnd(5));
        }
    }

    // restart (soon)
    if(will_restart and delay_restart > 0) {
        delay_restart -= 1;
        if(delay_restart <= 0) {
            will_restart = false;
            load_room(room.x, room.y);
        }
    }

    if(kb_IsDown(kb_KeyYequ)) {
        next_room();
    }

    // update each object
    for(size_t i = 0; i < objects.size();) {
        Object *obj = objects[i];
        obj->move(obj->spd.x, obj->spd.y);
        obj->update();
        if(i < objects.size() && objects[i] == obj) i++;
    }

    // start game
    if(is_title()) {
        if(not start_game and (btn(k_jump) or btn(k_dash))) {
            //music(-1);
            start_game_flash = 50;
            start_game = true;
            //sfx(38);
        }
        if(start_game) {
            start_game_flash -= 1;
            if(start_game_flash <= -30) {
                begin_game();
            }
        }
    }
}

// drawing functions //
///////////////////////
void _draw() {
    if(freeze > 0) return;

    // reset all palette values
    pal();

    // start game flash
    if(start_game) {
        int c = 10;
        if(start_game_flash > 10) {
            if(frames % 10 < 5) {
                c = 7;
            }
        } else if(start_game_flash > 5) {
            c = 2;
        } else if(start_game_flash > 0) {
            c = 1;
        } else {
            c = 0;
        }
        if(c < 10) {
            pal(6, c);
            pal(12, c);
            pal(13, c);
            pal(5, c);
            pal(1, c);
            pal(7, c);
        }
    }

    // clear screen
    int bg_col = 0;
    if(flash_bg) {
        bg_col = frames / 5;
    } else if(new_bg) {
        bg_col = 2;
    }
    rectfill(0, 0, 128, 128, bg_col);

    // clouds
    if(not is_title()) {
        for(Cloud &c: clouds) {
            c.x += c.spd;
            rectfill(c.x, c.y, c.x + c.w, c.y + 4 + (1 - c.w / 64) * 12, new_bg ? 14 : 1);
            if(c.x > 128) {
                c.x = -c.w;
                c.y = rnd(128 - 8);
            }
        }
    }

    // draw bg terrain
    map(room.x * 16, room.y * 16, 0, 0, 16, 16, 2);

    // platforms/big chest
    for(auto o: objects) {
        if(o->type == platform or o->type == big_chest) {
            o->draw();
        }
    }

    // draw terrain
    int off = is_title() ? -4 : 0;
    map(room.x * 16, room.y * 16, off, 0, 16, 16, 1);

    // draw objects
    for(auto o: objects) {
        if(o->type != platform and o->type != big_chest) {
            o->draw();
        }
    };

    // draw fg terrain
    map(room.x * 16, room.y * 16, 0, 0, 16, 16, 3);

    // particles
    // todo
/*    foreach(particles, function(p)
        p.x += p.spd
        p.y += sin(p.off)
        p.off+= min(0.05,p.spd/32)
        rectfill(p.x,p.y,p.x+p.s,p.y+p.s,p.c)
        if (p.x>128+4 ) {
            p.x=-4
            p.y=rnd(128)
        }
    })*/

    // dead particles
/*    foreach(dead_particles, function(p)
        p.x += p.spd.x
        p.y += p.spd.y
        p.t -=1
        if (p.t <= 0 ) { del(dead_particles,p) }
        rectfill(p.x-p.t/5,p.y-p.t/5,p.x+p.t/5,p.y+p.t/5,14+p.t%2)
    })*/

    // credits
    if(is_title()) {
        // todo: keys
        print("x+c", 58, 80, 5);
        print("matt thorson", 42, 96, 5);
        print("noel berry", 46, 102, 5);
        // todo: porting credits
    }

    if(level_index() == 30) {
        Object *p = nullptr;
        for(auto o: objects) {
            if(o->type == player) {
                p = o;
                break;
            }
        }
        if(p != nullptr) {
            int diff = min(24, 40 - abs(p->x + 4 - 64));
            rectfill(0, 0, diff, 128, 0);
            rectfill(128 - diff, 0, 128, 128, 0);
        }
    }
}

void Object::draw() {
    spr(sprite, x, y, 1, 1, flip.x, flip.y);
}

void draw_time(int x, int y) {
    int s = seconds;
    int m = minutes % 60;
    int h = minutes / 60;

    rectfill(x, y, x + 32, y + 6, 0);
    print_int(h, x + 1, y + 1, 7);
    print(":");
    print_int(m, 2);
    print(":");
    print_int(s, 2);
}

// helper functions //
//////////////////////

int clamp(int val, int a, int b) {
    return max(a, min(b, val));
}

int appr(int val, int target, int amount) {
    return val > target
           ? max(val - amount, target)
           : min(val + amount, target);
}

int sign(int v) {
    return v > 0 ? 1 : v < 0 ? -1 : 0;
}

bool maybe() {
    return rand() & 1;
}

bool solid_at(int x, int y, int w, int h) {
    return tile_flag_at(x, y, w, h, 0);
}

bool ice_at(int x, int y, int w, int h) {
    return tile_flag_at(x, y, w, h, 4);
}

bool tile_flag_at(int x, int y, int w, int h, uint8_t flag) {
    for(int i = max(0, x / 8); i <= min(15, (x + w - 1) / 8.0); i++) {
        for(int j = max(0, y / 8); j <= min(15, (y + h - 1) / 8.0); j++) {
            if(fget(tile_at(i, j), flag)) {
                return true;
            }
        }
    }
    return false;
}

uint8_t tile_at(int x, int y) {
    return mget(room.x * 16 + x, room.y * 16 + y);
}

bool spikes_at(int x, int y, int w, int h, subpixel xspd, subpixel yspd) {
    for(int i = max(0, x / 8); i <= min(15, (x + w - 1) / 8); i++) {
        for(int j = max(0, y / 8); j <= min(15, (y + h - 1) / 8); j++) {
            uint8_t tile = tile_at(i, j);
            if(tile == 17 and ((y + h - 1) % 8 >= 6 or y + h == j * 8 + 8) and yspd >= 0) {
                return true;
            } else if(tile == 27 and y % 8 <= 2 and yspd <= 0) {
                return true;
            } else if(tile == 43 and x % 8 <= 2 and xspd <= 0) {
                return true;
            } else if(tile == 59 and ((x + w - 1) % 8 >= 6 or x + w == i * 8 + 8) and xspd >= 0) {
                return true;
            }
        }
    }
    return false;
}
