#include "sim.h"

#include <math.h>
#include <string.h>

#define G 6.67430e-11
#define TRAIL_LENGTH 2000
#define TRAIL_RECORD_INTERVAL 5

typedef struct {
    double ax, ay;
} BodyAccel;

static void trail_init(TrailBuffer* trail, Arena* arena) {
    trail->points = (TrailPoint*)arena_alloc(arena, TRAIL_LENGTH * sizeof(TrailPoint));
    trail->capacity = trail->points ? TRAIL_LENGTH : 0;
    trail->head = 0;
    trail->count = 0;
}

static void trail_add_point(TrailBuffer* trail, double x, double y) {
    if (!trail->points || trail->capacity == 0) {
        return;
    }
    trail->points[trail->head].x = x;
    trail->points[trail->head].y = y;
    trail->head = (trail->head + 1) % trail->capacity;
    if (trail->count < trail->capacity) {
        trail->count++;
    }
}

static void sim_seed_solar_system(SimContext* sim) {
    const double sun_mass = 1.9885e30;

    sim_add_body(sim, (PhysicalBody){
        .x = 0.0,
        .y = 0.0,
        .vx = 0.0,
        .vy = 0.0,
        .mass = sun_mass,
        .radius = 6.9634e8f,
        .color = YELLOW,
        .name = "Sun",
    });

    const double earth_r = 1.496e11;
    const double earth_v = sqrt(G * sun_mass / earth_r);
    sim_add_body(sim, (PhysicalBody){
        .x = earth_r,
        .y = 0.0,
        .vx = 0.0,
        .vy = earth_v,
        .mass = 5.972e24,
        .radius = 6.371e6f,
        .color = BLUE,
        .name = "Earth",
    });

    const double venus_r = 1.082e11;
    const double venus_v = sqrt(G * sun_mass / venus_r);
    sim_add_body(sim, (PhysicalBody){
        .x = venus_r,
        .y = 0.0,
        .vx = 0.0,
        .vy = venus_v,
        .mass = 4.867e24,
        .radius = 6.052e6f,
        .color = ORANGE,
        .name = "Venus",
    });

    const double mars_r = 2.279e11;
    const double mars_v = sqrt(G * sun_mass / mars_r);
    sim_add_body(sim, (PhysicalBody){
        .x = mars_r,
        .y = 0.0,
        .vx = 0.0,
        .vy = mars_v,
        .mass = 6.39e23,
        .radius = 3.389e6f,
        .color = RED,
        .name = "Mars",
    });
}

void sim_init(SimContext* sim, Arena* arena) {
    sim->sim_arena = arena;
    array_init(&sim->bodies, 16, arena);
    array_init(&sim->trails, 16, arena);
    sim->time_seconds = 0.0;
    sim->trail_frame_counter = 0;

    sim_seed_solar_system(sim);
}

void sim_reset(SimContext* sim) {
    array_clear(&sim->bodies);
    array_clear(&sim->trails);
    sim->time_seconds = 0.0;
    sim->trail_frame_counter = 0;

    sim_seed_solar_system(sim);
}

BodyId sim_add_body(SimContext* sim, PhysicalBody body) {
    BodyId id = (BodyId)array_push(&sim->bodies, body, sim->sim_arena);
    TrailBuffer trail = {0};
    trail_init(&trail, sim->sim_arena);
    array_push(&sim->trails, trail, sim->sim_arena);
    return id;
}

static void compute_accelerations(SimContext* sim, BodyAccel* accels) {
    const size_t count = sim->bodies.length;
    
    for (size_t i = 0; i < count; i++) {
        accels[i].ax = 0.0;
        accels[i].ay = 0.0;
    }
    
    for (size_t i = 0; i < count; i += 1) {
        PhysicalBody* body_i = &sim->bodies.data[i];
        for (size_t j = i + 1; j < count; j += 1) {
            PhysicalBody* body_j = &sim->bodies.data[j];
            const double dx = body_j->x - body_i->x;
            const double dy = body_j->y - body_i->y;
            const double dist2 = dx * dx + dy * dy;
            const double dist = sqrt(dist2);
            const double inv_dist3 = 1.0 / (dist2 * dist);

            const double accel_i = G * body_j->mass * inv_dist3;
            const double accel_j = G * body_i->mass * inv_dist3;

            accels[i].ax += accel_i * dx;
            accels[i].ay += accel_i * dy;
            accels[j].ax -= accel_j * dx;
            accels[j].ay -= accel_j * dy;
        }
    }
}

void sim_step(SimContext* sim, double dt_seconds) {
    const size_t count = sim->bodies.length;
    if (count == 0 || dt_seconds <= 0.0) {
        return;
    }
    
    size_t arena_start = sim->sim_arena->offset;
    
    BodyAccel* accels = (BodyAccel*)arena_alloc(sim->sim_arena, count * sizeof(BodyAccel));
    BodyAccel* new_accels = (BodyAccel*)arena_alloc(sim->sim_arena, count * sizeof(BodyAccel));
    
    if (!accels || !new_accels) {
        sim->sim_arena->offset = arena_start;
        return;
    }

    memset(accels, 0, count * sizeof(BodyAccel));
    memset(new_accels, 0, count * sizeof(BodyAccel));

    compute_accelerations(sim, accels);

    for (size_t i = 0; i < count; i += 1) {
        PhysicalBody* body = &sim->bodies.data[i];
        body->x += body->vx * dt_seconds + 0.5 * accels[i].ax * dt_seconds * dt_seconds;
        body->y += body->vy * dt_seconds + 0.5 * accels[i].ay * dt_seconds * dt_seconds;
    }

    compute_accelerations(sim, new_accels);

    for (size_t i = 0; i < count; i += 1) {
        PhysicalBody* body = &sim->bodies.data[i];
        body->vx += 0.5 * (accels[i].ax + new_accels[i].ax) * dt_seconds;
        body->vy += 0.5 * (accels[i].ay + new_accels[i].ay) * dt_seconds;
    }

    sim->trail_frame_counter++;
    if (sim->trail_frame_counter >= TRAIL_RECORD_INTERVAL) {
        const size_t trail_count = sim->trails.length;
        const size_t min_count = count < trail_count ? count : trail_count;
        for (size_t i = 0; i < min_count; i++) {
            const PhysicalBody* body = &sim->bodies.data[i];
            trail_add_point(&sim->trails.data[i], body->x, body->y);
        }
        sim->trail_frame_counter = 0;
    }

    sim->time_seconds += dt_seconds;
    
    sim->sim_arena->offset = arena_start;
}

void sim_draw(const SimContext* sim, double cam_x, double cam_y, double zoom, int screen_w, int screen_h) {
    const double half_w = screen_w * 0.5;
    const double half_h = screen_h * 0.5;

    const size_t trail_count = sim->trails.length;
    const size_t body_count = sim->bodies.length;
    const size_t min_count = trail_count < body_count ? trail_count : body_count;

    for (size_t i = 0; i < min_count; i += 1) {
        const TrailBuffer* trail = &sim->trails.data[i];
        const PhysicalBody* body = &sim->bodies.data[i];

        if (trail->count < 2 || trail->capacity == 0) continue;

        for (size_t j = 0; j < trail->count - 1; j++) {
            size_t idx = (trail->head + trail->capacity - trail->count + j) % trail->capacity;
            size_t next_idx = (idx + 1) % trail->capacity;

            double x1 = (trail->points[idx].x - cam_x) * zoom + half_w;
            double y1 = (trail->points[idx].y - cam_y) * zoom + half_h;
            double x2 = (trail->points[next_idx].x - cam_x) * zoom + half_w;
            double y2 = (trail->points[next_idx].y - cam_y) * zoom + half_h;

            float alpha_ratio = (float)j / (float)trail->count;
            unsigned char alpha = (unsigned char)(alpha_ratio * 180.0f + 20.0f);

            Color trail_color = body->color;
            trail_color.a = alpha;

            DrawLineEx((Vector2){(float)x1, (float)y1},
                      (Vector2){(float)x2, (float)y2},
                      1.0f,
                      trail_color);
        }
    }

    for (size_t i = 0; i < sim->bodies.length; i += 1) {
        const PhysicalBody* body = &sim->bodies.data[i];

        double sx = (body->x - cam_x) * zoom + half_w;
        double sy = (body->y - cam_y) * zoom + half_h;
        double sr = (double)body->radius * zoom;

        if (sr < 2.0) sr = 2.0;

        DrawCircle((int)sx, (int)sy, (float)sr, body->color);

        if (body->name && body->name[0]) {
            DrawText(body->name, (int)(sx + sr + 4.0), (int)(sy - 6.0), 10, LIGHTGRAY);
        }
    }
}