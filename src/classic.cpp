#include "classic.h"

#include <TINYSTL/vector.h>
#include <debug.h>
#include <cmath>
#include <cstdlib>
#include "emu.h"

// ~celeste~
// matt thorson + noel berry

// globals //
/////////////

struct vec2i room = {0, 0};

tinystl::vector<Object*> objects = {};

int freeze=0;
int shake=0;
bool will_restart=false;
int delay_restart=0;
bool got_fruit[NUM_FRUITS];
bool has_dashed=false;
int sfx_timer=0;
bool has_key=false;
bool pause_player=false;
bool flash_bg=false;
int music_timer=0;
bool new_bg = false;

uint8_t k_left=0;
uint8_t k_right=1;
uint8_t k_up=2;
uint8_t k_down=3;
uint8_t k_jump=4;
uint8_t k_dash=5;

int frames;
int deaths;
int max_djump;
bool start_game;
int start_game_flash;
int seconds;
int minutes;

void _init() {
    title_screen();
}

void title_screen() {
    for(bool & i : got_fruit) {
        i = false;
    }
    frames=0;
    deaths=0;
    max_djump=1;
    start_game=false;
    start_game_flash=0;
    //music(40,0,7);
    
    load_room(7,3);
}

void begin_game() {
    frames=0;
    seconds=0;
    minutes=0;
    music_timer=0;
    start_game=false;
    //music(0,0,7);
    load_room(0,0);
}

int level_index() {
    return room.x%8+room.y*8;
}


bool is_title() {
    return level_index()==31;
}


// effects //
/////////////

// todo
// clouds = {}
// for i=0,16 do
//     add(clouds,{
//         x=rnd(128),
//         y=rnd(128),
//         spd=1+rnd(4),
//         w=32+rnd(32)
//     })
// }

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
    this->p_jump=false;
    this->p_dash=false;
    this->grace=0;
    this->jbuffer=0;
    this->djump=max_djump;
    this->dash_time=0;
    this->dash_effect_time=0;
    this->dash_target={.x=0,.y=0};
    this->dash_accel={.x=0,.y=0};
    this->hitbox = {.x=1,.y=3,.w=6,.h=5};
    this->spr_off=0;
    this->was_on_ground=false;
    this->type = player;
    // todo
//    create_hair(this);
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

    bool on_ground = this->is_solid(0, 1);
    bool on_ice = this->is_ice(0, 1);

    // smoke particles
    if(on_ground and not this->was_on_ground) {
        new Smoke(this->x, this->y + 4);
    }

    bool jump = btn(k_jump) and not this->p_jump;
    this->p_jump = btn(k_jump);
    if(jump) {
        this->jbuffer = 4;
    } else if(this->jbuffer > 0) {
        this->jbuffer -= 1;
    }

    bool dash = btn(k_dash) and not this->p_dash;

    this->p_dash = btn(k_dash);

    if(on_ground) {
        this->grace = 6;
        if(this->djump < max_djump) {
            //psfx(54);
            this->djump = max_djump;
        }
    } else if(this->grace > 0) {
        this->grace -= 1;
    }

    this->dash_effect_time -= 1;
    if(this->dash_time > 0) {
        new Smoke(this->x, this->y);
        this->dash_time -= 1;
        this->spd.x = appr(this->spd.x, this->dash_target.x, this->dash_accel.x);
        this->spd.y = appr(this->spd.y, this->dash_target.y, this->dash_accel.y);
    } else {

        // move
        float maxrun = 1;
        float accel = 0.6;
        float deccel = 0.15;

        if(not on_ground) {
            accel = 0.4;
        } else if(on_ice) {
            accel = 0.05;
            if(input == (this->flip.x ? -1 : 1)) {
                accel = 0.05;
            }
        }

        if(fabs(this->spd.x) > maxrun) {
            this->spd.x = appr(this->spd.x, sign(this->spd.x) * maxrun, deccel);
        } else {
            this->spd.x = appr(this->spd.x, input * maxrun, accel);
        }

        //facing
        if(this->spd.x != 0) {
            this->flip.x = (this->spd.x < 0);
        }

        // gravity
        float maxfall = 2;
        float gravity = 0.21;

        if(fabs(this->spd.y) <= 0.15) {
            gravity *= 0.5;
        }

        // wall slide
        if(input != 0 and this->is_solid(input, 0) and not this->is_ice(input, 0)) {
            maxfall = 0.4;
            if(rnd(10) < 2) {
                new Smoke(this->x + input * 6, this->y);
            }
        }

        if(not on_ground) {
            this->spd.y = appr(this->spd.y, maxfall, gravity);
        }

        // jump
        if(this->jbuffer > 0) {
            if(this->grace > 0) {
                // normal jump
                //psfx(1);
                this->jbuffer = 0;
                this->grace = 0;
                this->spd.y = -2;
                new Smoke(this->x, this->y + 4);
            } else {
                // wall jump
                int wall_dir = (this->is_solid(-3, 0) ? -1 : this->is_solid(3, 0) ? 1 : 0);
                if(wall_dir != 0) {
                    //psfx(2);
                    this->jbuffer = 0;
                    this->spd.y = -2;
                    this->spd.x = -wall_dir * (maxrun + 1);
                    if(not this->is_ice(wall_dir * 3, 0)) {
                        new Smoke(this->x + wall_dir * 6, this->y);
                    }
                }
            }
        }

        // dash
        float d_full = 5;
        float d_half = d_full * 0.70710678118;

        if(this->djump > 0 and dash) {
            new Smoke(this->x, this->y);
            this->djump -= 1;
            this->dash_time = 4;
            has_dashed = true;
            this->dash_effect_time = 10;
            int v_input = (btn(k_up) ? -1 : (btn(k_down) ? 1 : 0));
            if(input != 0) {
                if(v_input != 0) {
                    this->spd.x = input * d_half;
                    this->spd.y = v_input * d_half;
                } else {
                    this->spd.x = input * d_full;
                    this->spd.y = 0;
                }
            } else if(v_input != 0) {
                this->spd.x = 0;
                this->spd.y = v_input * d_full;
            } else {
                this->spd.x = (this->flip.x ? -1 : 1);
                this->spd.y = 0;
            }

            //psfx(3);
            freeze = 2;
            shake = 6;
            this->dash_target.x = 2 * sign(this->spd.x);
            this->dash_target.y = 2 * sign(this->spd.y);
            this->dash_accel.x = 1.5;
            this->dash_accel.y = 1.5;

            if(this->spd.y < 0) {
                this->dash_target.y *= .75;
            }

            if(this->spd.y != 0) {
                this->dash_accel.x *= 0.70710678118;
            }
            if(this->spd.x != 0) {
                this->dash_accel.y *= 0.70710678118;
            }
        } else if(dash and this->djump <= 0) {
            //psfx(9);
            new Smoke(this->x, this->y);
        }

    }

    // animation
    this->spr_off += 0.25;
    if(not on_ground) {
        if(this->is_solid(input, 0)) {
            this->sprite = 5;
        } else {
            this->sprite = 3;
        }
    } else if(btn(k_down)) {
        this->sprite = 6;
    } else if(btn(k_up)) {
        this->sprite = 7;
    } else if((this->spd.x == 0) or (not btn(k_left) and not btn(k_right))) {
        this->sprite = 1;
    } else {
        this->sprite = 1 + (int)this->spr_off % 4;
    }

    // next level
    if(this->y < -4 and level_index() < 30) { next_room(); }

    // was on the ground
    this->was_on_ground = on_ground;
}

void Player::draw() {
    // clamp in screen
    if (this->x<-1 or this->x>121 ) { 
        this->x=clamp(this->x,-1,121);
        this->spd.x=0;
    }

    // todo
    //set_hair_color(this->djump);
    //draw_hair(this,this->flip.x ? -1 : 1);
    spr(this->sprite,this->x,this->y,1,1,this->flip.x,this->flip.y);
    //unset_hair_color();
}

/*

psfx=function(num)
 if (sfx_timer<=0 ) {
  sfx(num)
 }
}

create_hair=function(obj)
    obj.hair={}
    for i=0,4 do
        add(obj.hair,{x=obj.x,y=obj.y,size=max(1,min(2,3-i))})
    }
}

set_hair_color=function(djump)
    pal(8,(djump==1 and 8 or djump==2 and (7+flr((frames/3)%2)*4) or 12))
}

draw_hair=function(obj,facing)
    local last={x=obj.x+4-facing*2,y=obj.y+(btn(k_down) and 4 or 3)}
    foreach(obj.hair,function(h)
        h.x+=(last.x-h.x)/1.5
        h.y+=(last.y+0.5-h.y)/1.5
        circfill(h.x,h.y,h.size,8)
        last=h
    })
}

unset_hair_color=function()
    pal(8,8)
}

 */

PlayerSpawn::PlayerSpawn(int x, int y) : Object (x, y) {
    //sfx(4);
    this->sprite=3;
    this->target = {.x=this->x,.y=this->y};
    this->y=128;
    this->spd.y=-4;
    this->state=0;
    this->delay=0;
    this->solids=false;
    this->type = player_spawn;
    // todo
    //create_hair(this);
}

void PlayerSpawn::update() {
    // jumping up
    if(this->state == 0) {
        if(this->y < this->target.y + 16) {
            this->state = 1;
            this->delay = 3;
        }
        // falling
    } else if(this->state == 1) {
        this->spd.y += 0.5;
        if(this->spd.y > 0 and this->delay > 0) {
            this->spd.y = 0;
            this->delay -= 1;
        }
        if(this->spd.y > 0 and this->y > this->target.y) {
            this->y = this->target.y;
            this->spd = {.x=0, .y=0};
            this->state = 2;
            this->delay = 5;
            shake = 5;
            new Smoke(this->x, this->y + 4);
            //sfx(5);
        }
        // landing
    } else if(this->state == 2) {
        this->delay -= 1;
        this->sprite = 6;
        if(this->delay < 0) {
            destroy_object(this);
            new Player(this->x, this->y);
        }
    }
}

void PlayerSpawn::draw() {
    // todo
    //set_hair_color(max_djump);
    //draw_hair(this,1);
    spr(this->sprite,this->x,this->y,1,1,this->flip.x,this->flip.y);
    //unset_hair_color();
}

/*

spring = {
    tile=18,
    init=function(this)
        this->hide_in=0
        this->hide_for=0
    },
    update=function(this)
        if (this->hide_for>0 ) {
            this->hide_for-=1
            if (this->hide_for<=0 ) {
                this->spr=18
                this->delay=0
            }
        } else if (this->spr==18 ) {
            local hit = this->collide(player,0,0)
            if (hit !=nullptr and hit.spd.y>=0 ) {
                this->spr=19
                hit.y=this->y-4
                hit.spd.x*=0.2
                hit.spd.y=-3
                hit.djump=max_djump
                this->delay=10
                init_object(smoke,this->x,this->y)
               
                // breakable below us
                local below=this->collide(fall_floor,0,1)
                if (below!=nullptr ) {
                    break_fall_floor(below)
                }
               
                psfx(8)
            }
        } else if (this->delay>0 ) {
            this->delay-=1
            if (this->delay<=0 ) { 
                this->spr=18 
            }
        }
        // begin hiding
        if (this->hide_in>0 ) {
            this->hide_in-=1
            if (this->hide_in<=0 ) {
                this->hide_for=60
                this->spr=0
            }
        }
    }
}
add(types,spring)

function break_spring(obj)
    obj.hide_in=15
}

balloon = {
    tile=22,
    init=function(this) 
        this->offset=rnd(1)
        this->start=this->y
        this->timer=0
        this->hitbox={x=-1,y=-1,w=10,h=10}
    },
    update=function(this) 
        if (this->spr==22 ) {
            this->offset+=0.01
            this->y=this->start+sin(this->offset)*2
            local hit = this->collide(player,0,0)
            if (hit!=nullptr and hit.djump<max_djump ) {
                psfx(6)
                init_object(smoke,this->x,this->y)
                hit.djump=max_djump
                this->spr=0
                this->timer=60
            }
        } else if (this->timer>0 ) {
            this->timer-=1
        else 
         psfx(7)
         init_object(smoke,this->x,this->y)
            this->spr=22 
        }
    },
    draw=function(this)
        if (this->spr==22 ) {
            spr(13+(this->offset*8)%3,this->x,this->y+6)
            spr(this->spr,this->x,this->y)
        }
    }
}
add(types,balloon)

fall_floor = {
    tile=23,
    init=function(this)
        this->state=0
        this->solid=true
    },
    update=function(this)
        // idling
        if (this->state == 0 ) {
            if this->check(player,0,-1) or this->check(player,-1,0) or this->check(player,1,0) {
                break_fall_floor(this)
            }
        // shaking
        } else if (this->state==1 ) {
            this->delay-=1
            if (this->delay<=0 ) {
                this->state=2
                this->delay=60//how long it hides for
                this->collideable=false
            }
        // invisible, waiting to reset
        } else if (this->state==2 ) {
            this->delay-=1
            if this->delay<=0 and not this->check(player,0,0) {
                psfx(7)
                this->state=0
                this->collideable=true
                init_object(smoke,this->x,this->y)
            }
        }
    },
    draw=function(this)
        if (this->state!=2 ) {
            if (this->state!=1 ) {
                spr(23,this->x,this->y)
            else
                spr(23+(15-this->delay)/5,this->x,this->y)
            }
        }
    }
}
add(types,fall_floor)

function break_fall_floor(obj)
 if (obj.state==0 ) {
     psfx(15)
        obj.state=1
        obj.delay=15//how long until it falls
        init_object(smoke,obj.x,obj.y)
        local hit=obj.collide(spring,0,-1)
        if (hit!=nullptr ) {
            break_spring(hit)
        }
    }
}
*/
Smoke::Smoke(int x, int y) : Object(x, y) {
    this->sprite=29;
    this->spd.y=-0.1;
    this->spd.x=0.3+rnd(0.2);
    this->x+=-1+rnd(2);
    this->y+=-1+rnd(2);
    this->flip.x=maybe();
    this->flip.y=maybe();
    this->solids=false;
    this->type = smoke;
}
void Smoke::update() {
    this->sprite+=0.2;
    if (this->sprite>=32) {
        destroy_object(this);
    }
}
/*
fruit={
    tile=26,
    if_not_fruit=true,
    init=function(this) 
        this->start=this->y
        this->off=0
    },
    update=function(this)
     local hit=this->collide(player,0,0)
        if (hit!=nullptr ) {
         hit.djump=max_djump
            sfx_timer=20
            sfx(13)
            got_fruit[level_index()] = true
            init_object(lifeup,this->x,this->y)
            destroy_object(this)
        }
        this->off+=1
        this->y=this->start+sin(this->off/40)*2.5
    }
}
add(types,fruit)

fly_fruit={
    tile=28,
    if_not_fruit=true,
    init=function(this) 
        this->start=this->y
        this->fly=false
        this->step=0.5
        this->solids=false
        this->sfx_delay=8
    },
    update=function(this)
        //fly away
        if (this->fly ) {
         if (this->sfx_delay>0 ) {
          this->sfx_delay-=1
          if (this->sfx_delay<=0 ) {
           sfx_timer=20
           sfx(14)
          }
         }
            this->spd.y=appr(this->spd.y,-3.5,0.25)
            if (this->y<-16 ) {
                destroy_object(this)
            }
        // wait
        else
            if (has_dashed ) {
                this->fly=true
            }
            this->step+=0.05
            this->spd.y=sin(this->step)*0.5
        }
        // collect
        local hit=this->collide(player,0,0)
        if (hit!=nullptr ) {
         hit.djump=max_djump
            sfx_timer=20
            sfx(13)
            got_fruit[level_index()] = true
            init_object(lifeup,this->x,this->y)
            destroy_object(this)
        }
    },
    draw=function(this)
        local off=0
        if (not this->fly ) {
            local dir=sin(this->step)
            if (dir<0 ) {
                off=1+max(0,sign(this->y-this->start))
            }
        else
            off=(off+0.25)%3
        }
        spr(45+off,this->x-6,this->y-2,1,1,true,false)
        spr(this->spr,this->x,this->y)
        spr(45+off,this->x+6,this->y-2)
    }
}
add(types,fly_fruit)

lif (eup = ) {
    init=function(this)
        this->spd.y=-0.25
        this->duration=30
        this->x-=2
        this->y-=4
        this->flash=0
        this->solids=false
    },
    update=function(this)
        this->duration-=1
        if (this->duration<= 0 ) {
            destroy_object(this)
        }
    },
    draw=function(this)
        this->flash+=0.5

        print("1000",this->x-2,this->y,7+this->flash%2)
    }
}

fake_wall = {
    tile=64,
    if_not_fruit=true,
    update=function(this)
        this->hitbox={x=-1,y=-1,w=18,h=18}
        local hit = this->collide(player,0,0)
        if (hit!=nullptr and hit.dash_effect_time>0 ) {
            hit.spd.x=-sign(hit.spd.x)*1.5
            hit.spd.y=-1.5
            hit.dash_time=-1
            sfx_timer=20
            sfx(16)
            destroy_object(this)
            init_object(smoke,this->x,this->y)
            init_object(smoke,this->x+8,this->y)
            init_object(smoke,this->x,this->y+8)
            init_object(smoke,this->x+8,this->y+8)
            init_object(fruit,this->x+4,this->y+4)
        }
        this->hitbox={x=0,y=0,w=16,h=16}
    },
    draw=function(this)
        spr(64,this->x,this->y)
        spr(65,this->x+8,this->y)
        spr(80,this->x,this->y+8)
        spr(81,this->x+8,this->y+8)
    }
}
add(types,fake_wall)

key={
    tile=8,
    if_not_fruit=true,
    update=function(this)
        local was=flr(this->spr)
        this->spr=9+(sin(frames/30)+0.5)*1
        local is=flr(this->spr)
        if (is==10 and is!=was ) {
            this->flip.x=not this->flip.x
        }
        if this->check(player,0,0) {
            sfx(23)
            sfx_timer=10
            destroy_object(this)
            has_key=true
        }
    }
}
add(types,key)

chest={
    tile=20,
    if_not_fruit=true,
    init=function(this)
        this->x-=4
        this->start=this->x
        this->timer=20
    },
    update=function(this)
        if (has_key ) {
            this->timer-=1
            this->x=this->start-1+rnd(3)
            if (this->timer<=0 ) {
             sfx_timer=20
             sfx(16)
                init_object(fruit,this->x,this->y-4)
                destroy_object(this)
            }
        }
    }
}
add(types,chest)

platform={
    init=function(this)
        this->x-=4
        this->solids=false
        this->hitbox.w=16
        this->last=this->x
    },
    update=function(this)
        this->spd.x=this->dir*0.65
        if (this->x<-16 ) { this->x=128
        } else if (this->x>128 ) { this->x=-16 }
        if not this->check(player,0,0) {
            local hit=this->collide(player,0,-1)
            if (hit!=nullptr ) {
                hit.move_x(this->x-this->last,1)
            }
        }
        this->last=this->x
    },
    draw=function(this)
        spr(11,this->x,this->y-1)
        spr(12,this->x+8,this->y-1)
    }
}

message={
    tile=86,
    last=0,
    draw=function(this)
        this->text="-- celeste mountain --#this memorial to those# perished on the climb"
        if this->check(player,4,0) {
            if (this->index<#this->text ) {
             this->index+=0.5
                if (this->index>=this->last+1 ) {
                 this->last+=1
                 sfx(35)
                }
            }
            this->off={x=8,y=96}
            for i=1,this->index do
                if sub(this->text,i,i)!="#" {
                    rectfill(this->off.x-2,this->off.y-2,this->off.x+7,this->off.y+6 ,7)
                    print(sub(this->text,i,i),this->off.x,this->off.y,0)
                    this->off.x+=5
                else
                    this->off.x=8
                    this->off.y+=7
                }
            }
        else
            this->index=0
            this->last=0
        }
    }
}
add(types,message)

big_chest={
    tile=96,
    init=function(this)
        this->state=0
        this->hitbox.w=16
    },
    draw=function(this)
        if (this->state==0 ) {
            local hit=this->collide(player,0,8)
            if hit!=nullptr and hit.is_solid(0,1) {
                music(-1,500,7)
                sfx(37)
                pause_player=true
                hit.spd.x=0
                hit.spd.y=0
                this->state=1
                init_object(smoke,this->x,this->y)
                init_object(smoke,this->x+8,this->y)
                this->timer=60
                this->particles={}
            }
            spr(96,this->x,this->y)
            spr(97,this->x+8,this->y)
        } else if (this->state==1 ) {
            this->timer-=1
         shake=5
         flash_bg=true
            if this->timer<=45 and count(this->particles)<50 {
                add(this->particles,{
                    x=1+rnd(14),
                    y=0,
                    h=32+rnd(32),
                    spd=8+rnd(8)
                })
            }
            if (this->timer<0 ) {
                this->state=2
                this->particles={}
                flash_bg=false
                new_bg=true
                init_object(orb,this->x+4,this->y+4)
                pause_player=false
            }
            foreach(this->particles,function(p)
                p.y+=p.spd
                line(this->x+p.x,this->y+8-p.y,this->x+p.x,min(this->y+8-p.y+p.h,this->y+8),7)
            })
        }
        spr(112,this->x,this->y+8)
        spr(113,this->x+8,this->y+8)
    }
}
add(types,big_chest)

orb={
    init=function(this)
        this->spd.y=-4
        this->solids=false
        this->particles={}
    },
    draw=function(this)
        this->spd.y=appr(this->spd.y,0,0.5)
        local hit=this->collide(player,0,0)
        if (this->spd.y==0 and hit!=nullptr ) {
         music_timer=45
            sfx(51)
            freeze=10
            shake=10
            destroy_object(this)
            max_djump=2
            hit.djump=2
        }
       
        spr(102,this->x,this->y)
        local off=frames/30
        for i=0,7 do
            circfill(this->x+4+cos(off+i/8)*8,this->y+4+sin(off+i/8)*8,1,7)
        }
    }
}

flag = {
    tile=118,
    init=function(this)
        this->x+=5
        this->score=0
        this->show=false
        for i=1,count(got_fruit) do
            if (got_fruit[i] ) {
                this->score+=1
            }
        }
    },
    draw=function(this)
        this->spr=118+(frames/5)%3
        spr(this->spr,this->x,this->y)
        if (this->show ) {
            rectfill(32,2,96,31,0)
            spr(26,55,6)
            print("x"..this->score,64,9,7)
            draw_time(49,16)
            print("deaths:"..deaths,48,24,7)
        } else if this->check(player,0,0) {
            sfx(55)
      sfx_timer=30
            this->show=true
        }
    }
}
add(types,flag)

 */

RoomTitle::RoomTitle(int x, int y) : Object(x, y) {
    this->delay = 5;
}

void RoomTitle::draw() {
    this->delay -= 1;
    if (this->delay < -30) {
        destroy_object(this);
    } else if (this->delay < 0) {

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
        case platform:
            // todo
            //return new Platform(x, y);
        case smoke:
            return new Smoke(x, y);
        default:
            return nullptr;
    }
}

Object::Object(int x, int y) {
    this->collideable=true;
    this->solids=true;

    this->sprite = 0; // todo
    this->flip = {.x=false,.y=false};

    this->x = x;
    this->y = y;
    this->hitbox = { .x=0,.y=0,.w=8,.h=8 };

    this->spd = {.x=0,.y=0};
    this->rem = {.x=0,.y=0};

    objects.push_back(this);
}


bool Object::is_solid(int ox, int oy) {
    if(oy > 0 and not this->check(platform, ox, 0) and this->check(platform, ox, oy)) {
        return true;
    }
    return solid_at(this->x + this->hitbox.x + ox, this->y + this->hitbox.y + oy, this->hitbox.w, this->hitbox.h)
           or this->check(fall_floor, ox, oy)
           or this->check(fake_wall, ox, oy);
}


bool Object::is_ice(int ox, int oy) {
    return ice_at(this->x+this->hitbox.x+ox,this->y+this->hitbox.y+oy,this->hitbox.w,this->hitbox.h);
}

Object *Object::collide(enum type type, int ox, int oy) {
    for(Object *other: objects) {
        if(other != nullptr and other->type == type and other != this and other->collideable and
           other->x + other->hitbox.x + other->hitbox.w > this->x + this->hitbox.x + ox and
           other->y + other->hitbox.y + other->hitbox.h > this->y + this->hitbox.y + oy and
           other->x + other->hitbox.x < this->x + this->hitbox.x + this->hitbox.w + ox and
           other->y + other->hitbox.y < this->y + this->hitbox.y + this->hitbox.h + oy) {
            return other;
        }
    }
    return nullptr;
}

bool Object::check(enum type type, int ox, int oy) {
    return this->collide(type, ox, oy) != nullptr;
}

void Object::move(float ox, float oy) {
    float amount;
    // [x] get move amount
    this->rem.x += ox;
    amount = floor(this->rem.x + 0.5);
    this->rem.x -= amount;
    this->move_x(amount, 0);

    // [y] get move amount
    this->rem.y += oy;
    amount = floor(this->rem.y + 0.5);
    this->rem.y -= amount;
    this->move_y(amount);
}

void Object::move_x(float amount, float start) {
    if(this->solids) {
        int step = sign(amount);
        for(int i = start; i < fabs(amount); i++) {
            if(not this->is_solid(step, 0)) {
                this->x += step;
            } else {
                this->spd.x = 0;
                this->rem.x = 0;
                break;
            }
        }
    } else {
        this->x += amount;
    }
}

void Object::move_y(float amount) {
    if(this->solids) {
        int step = sign(amount);
        for(int i = 0; i < fabs(amount); i++) {
            if(not this->is_solid(0, step)) {
                this->y += step;
            } else {
                this->spd.y = 0;
                this->rem.y = 0;
                break;
            }
        }
    } else {
        this->y += amount;
    }
}

void destroy_object(Object *obj) {
    for(auto i = objects.begin(); i != objects.end(); i++) {
        if(*i == obj) {
            objects.erase(i);
            return;
        }
    }
}

void Player::kill() {
    sfx_timer=12;
    //sfx(0);
    deaths+=1;
    shake=10;
    destroy_object(this);
//    dead_particles={};
//    for(int dir=0; dir <= 7; dir++) {
//        float angle=(dir/8.0);
//        add(dead_particles,{
//            x=obj.x+4,
//            y=obj.y+4,
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
    will_restart=true;
    delay_restart=15;
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
    has_dashed=false;
    has_key=false;

    //remove existing objects
    for(Object *object : objects) {
        // todo: this probably doesn't work
        destroy_object(object);
    }

    //current room
    room.x = x;
    room.y = y;

    // entities
    for(uint8_t tx = 0; tx <= 15; tx++) {
        for(uint8_t ty = 0 ; ty <= 15; ty++) {
            uint8_t tile = mget(room.x*16+tx,room.y*16+ty);
            if(tile==11) {
                init_object(platform,tx*8,ty*8)->dir=-1;
            }
            else if(tile==12) {
                init_object(platform,tx*8,ty*8)->dir=1;
            }
            else {
                init_object((type)(tile), tx * 8, ty * 8);
            }
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

    if (music_timer > 0) {
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

    // update each object
    for(auto obj : objects) {
        obj->move(obj->spd.x, obj->spd.y);
        obj->update();
    }

    // start game
    if(is_title()) {
        if (not start_game and (btn(k_jump) or btn(k_dash)))
        {
            //music(-1);
            start_game_flash = 50;
            start_game = true;
            //sfx(38);
        }
        if (start_game) {
            start_game_flash -= 1;
            if(start_game_flash <= -30)
            {
                begin_game();
            }
        }
    }
}

// drawing functions //
///////////////////////
void _draw() {
    if(freeze>0) return;
   
    // reset all palette values
    pal();
   
    // start game flash
    if (start_game) {
        int c=10;
        if (start_game_flash>10) {
            if (frames%10<5) {
                c=7;
            }
        } else if (start_game_flash>5) {
            c=2;
        } else if (start_game_flash>0) {
            c = 1;
        } else {
            c=0;
        }
        if (c<10) {
            pal(6,c);
            pal(12,c);
            pal(13,c);
            pal(5,c);
            pal(1,c);
            pal(7,c);
        }
    }

    // clear screen
    int bg_col = 0;
    if (flash_bg) {
        bg_col = frames/5;
    } else if (new_bg) {
        bg_col=2;
    }
    rectfill(0,0,128,128,bg_col);

    // clouds
    // todo
    /*
    if (not is_title()) {
        foreach(clouds, function(c)
            c.x += c.spd
            rectfill(c.x,c.y,c.x+c.w,c.y+4+(1-c.w/64)*12,new_bg and 14 or 1)
            if (c.x > 128 ) {
                c.x = -c.w
                c.y=rnd(128-8)
            }
        })
    }
     */

    // draw bg terrain
    map(room.x * 16,room.y * 16,0,0,16,16,2);

    // platforms/big chest
    for(auto o : objects) {
        if (o->type==platform or o->type==big_chest) {
            o->draw();
        }
    }

    // draw terrain
    int off=is_title() ? -4 : 0;
    map(room.x*16,room.y * 16,off,0,16,16,1);
   
    // draw objects
    for(auto o : objects) {
        if (o->type!=platform and o->type!=big_chest) {
            o->draw();
        }
    };

    // draw fg terrain
    map(room.x * 16,room.y * 16,0,0,16,16,3);
   
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
   
    // draw outside of the screen for screenshake
    // todo: undo
//    rectfill(-5,-5,-1,133,0);
//    rectfill(-5,-5,133,-1,0);
//    rectfill(-5,128,133,133,0);
//    rectfill(128,-5,133,133,0);
   
    // credits
    if (is_title()) {
        // todo: keys
        print("x+c",58,80,5);
        print("matt thorson",42,96,5);
        print("noel berry",46,102,5);
        // todo: porting credits
    }
   
    if (level_index()==30) {
        Object *p = nullptr;
        for (auto o : objects) {
            if (o->type==player) {
                p = o;
                break;
            }
        }
        if (p!=nullptr) {
            int diff=min(24,40-fabs(p->x+4-64));
            rectfill(0,0,diff,128,0);
            rectfill(128-diff,0,128,128,0);
        }
    }
}

void Object::draw() {
    spr(this->sprite, this->x, this->y, 1, 1, this->flip.x, this->flip.y);
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

float clamp(float val, float a, float b) {
    return max(a, min(b, val));
}

float appr(float val, float target, float amount) {
 return val > target 
     ? max(val - amount, target)
     : min(val + amount, target);
}

int sign(float v) {
    return v>0 ? 1 : v<0 ? -1 : 0;
}

bool maybe() {
    return rand() & 1;
}

bool solid_at(int x, int y, int w, int h) {
 return tile_flag_at(x,y,w,h,0);
}

bool ice_at(int x, int y, int w, int h) {
 return tile_flag_at(x,y,w,h,4);
}

bool tile_flag_at(int x, int y, int w, int h, uint8_t flag) {
    for(int i = max(0, x / 8); i < min(15, (x + w - 1) / 8.0); i++) {
        for(int j = max(0, y / 8); j < min(15, (y + h - 1) / 8.0); j++) {
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

bool spikes_at(int x, int y, int w, int h, float xspd, float yspd) {
    for(int i = max(0, x / 8); i < min(15, (x + w - 1) / 8.0); i++) {
        for(int j = max(0, y / 8); j < min(15, (y + h - 1) / 8.0); j++) {
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
