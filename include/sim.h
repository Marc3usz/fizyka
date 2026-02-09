#ifndef SIM_H
#define SIM_H

#include "arena.h"
#include "dynamic_array.h"
#include "raylib.h"

typedef struct {
    double x, y;
    double vx, vy;
    double mass;
    float radius;
    Color color;
    const char* name;
} PhysicalBody;

DEFINE_ARRAY(PhysicalBody);

typedef struct {
    Arena* sim_arena;
    Array_PhysicalBody bodies;
    double time_seconds;
} SimContext;

typedef long BodyId;

void sim_init(SimContext* sim, Arena* arena);
void sim_reset(SimContext* sim);
BodyId sim_add_body(SimContext* sim, PhysicalBody body);
void sim_step(SimContext* sim, double dt_seconds);
void sim_draw(const SimContext* sim, double cam_x, double cam_y, double zoom, int screen_w, int screen_h);

#endif
