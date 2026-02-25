#ifndef SIM_H
#define SIM_H

#include "arena.h"
#include "dynamic_array.h"
#include "raylib.h"
#include <stdint.h>

typedef __int128 i128;

#define SIM_MM_PER_METER ((i128)1000)

static inline i128 sim_m_to_mm(double meters) {
    long double scaled = (long double)meters * (long double)SIM_MM_PER_METER;
    if (scaled >= 0.0L) {
        return (i128)(scaled + 0.5L);
    }
    return (i128)(scaled - 0.5L);
}

static inline double sim_mm_to_m(i128 millimeters) {
    return (double)((long double)millimeters / (long double)SIM_MM_PER_METER);
}

typedef struct {
    i128 x_mm, y_mm;
    i128 vx_mm_s, vy_mm_s;
    double mass;
    i128 radius_mm;
    Color color;
    const char* name;
} PhysicalBody;

DEFINE_ARRAY(PhysicalBody);

typedef struct {
    i128 x_mm, y_mm;
} TrailPoint;

typedef struct {
    TrailPoint* points;
    size_t capacity;
    size_t head;
    size_t count;
} TrailBuffer;

DEFINE_ARRAY(TrailBuffer);

typedef enum {
    SIM_INTEGRATOR_VELOCITY_VERLET = 0,
    SIM_INTEGRATOR_YOSHIDA_RUTH_4 = 1,
} SimIntegrator;

typedef struct {
    Arena* sim_arena;
    Array_PhysicalBody bodies;
    Array_TrailBuffer trails;
    double time_seconds;
    int trail_frame_counter;
    SimIntegrator integrator;
} SimContext;

typedef long BodyId;

void sim_init(SimContext* sim, Arena* arena);
void sim_reset(SimContext* sim);
BodyId sim_add_body(SimContext* sim, PhysicalBody body);
void sim_step(SimContext* sim, double dt_seconds);
void sim_set_integrator(SimContext* sim, SimIntegrator integrator);
SimIntegrator sim_get_integrator(const SimContext* sim);
void sim_draw(const SimContext* sim, i128 cam_x_mm, i128 cam_y_mm, double zoom_px_per_mm, int screen_w, int screen_h);
BodyId sim_add_body_circular_orbit(SimContext* sim, BodyId parent_id,
                                   double orbit_radius, double initial_angle,
                                                                     double mass, double radius_m, Color color, const char* name);
BodyId sim_add_body_elliptical_orbit(SimContext* sim, BodyId parent_id,
                                     double periapsis, double apoapsis, double initial_angle,
                                                                         double mass, double radius_m, Color color, const char* name);
#endif
