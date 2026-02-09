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
    double x, y;
} TrailPoint;

typedef struct {
    TrailPoint* points;
    size_t capacity;
    size_t head;
    size_t count;
} TrailBuffer;

DEFINE_ARRAY(TrailBuffer);

typedef struct {
    Arena* sim_arena;
    Array_PhysicalBody bodies;
    Array_TrailBuffer trails;
    double time_seconds;
    int trail_frame_counter;
} SimContext;

typedef long BodyId;

void sim_init(SimContext* sim, Arena* arena);
void sim_reset(SimContext* sim);
BodyId sim_add_body(SimContext* sim, PhysicalBody body);
void sim_step(SimContext* sim, double dt_seconds);
void sim_draw(const SimContext* sim, double cam_x, double cam_y, double zoom, int screen_w, int screen_h);
BodyId sim_add_body_circular_orbit(SimContext* sim, BodyId parent_id,
                                   double orbit_radius, double initial_angle,
                                   double mass, float radius, Color color, const char* name);
BodyId sim_add_body_elliptical_orbit(SimContext* sim, BodyId parent_id,
                                     double periapsis, double apoapsis, double initial_angle,
                                     double mass, float radius, Color color, const char* name);
#endif
