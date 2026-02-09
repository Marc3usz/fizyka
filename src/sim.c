#include "sim.h"

#include <math.h>
#include <string.h>

#define G 6.67430e-11
#define TRAIL_LENGTH 2000
#define TRAIL_RECORD_INTERVAL 5

typedef struct {
    double ax, ay;
} BodyAccel;

typedef struct {
    double semi_major_axis;  // meters
    double eccentricity;     // 0 = circular, 0-1 = ellipse
    double inclination;      // radians
    double arg_periapsis;    // radians (angle from reference to periapsis)
    double longitude;        // radians (initial position angle)
} OrbitalElements;

static double orbital_velocity_at_distance(double central_mass, double distance, double semi_major_axis) {
    // Vis-viva equation: v^2 = GM(2/r - 1/a)
    return sqrt(G * central_mass * (2.0 / distance - 1.0 / semi_major_axis));
}

static double circular_orbital_velocity(double central_mass, double radius) {
    return sqrt(G * central_mass / radius);
}

static PhysicalBody create_circular_orbit(
    const PhysicalBody* parent,
    double orbit_radius,
    double initial_angle,
    double mass,
    float radius,
    Color color,
    const char* name)
{
    double vel = circular_orbital_velocity(parent->mass, orbit_radius);
    
    double x = parent->x + orbit_radius * cos(initial_angle);
    double y = parent->y + orbit_radius * sin(initial_angle);
    
    double vx = parent->vx - vel * sin(initial_angle);
    double vy = parent->vy + vel * cos(initial_angle);
    
    return (PhysicalBody){
        .x = x,
        .y = y,
        .vx = vx,
        .vy = vy,
        .mass = mass,
        .radius = radius,
        .color = color,
        .name = name,
    };
}

static PhysicalBody create_elliptical_orbit(
    const PhysicalBody* parent,
    double periapsis,      // closest distance
    double apoapsis,       // farthest distance
    double initial_angle,  // radians (0 = periapsis)
    double mass,
    float radius,
    Color color,
    const char* name)
{
    double semi_major_axis = (periapsis + apoapsis) / 2.0;
    double eccentricity = (apoapsis - periapsis) / (apoapsis + periapsis);
    
    double r = semi_major_axis * (1.0 - eccentricity * eccentricity) / 
               (1.0 + eccentricity * cos(initial_angle));
    
    double vel = orbital_velocity_at_distance(parent->mass, r, semi_major_axis);
    
    double x = parent->x + r * cos(initial_angle);
    double y = parent->y + r * sin(initial_angle);
    
    double h = sqrt(G * parent->mass * semi_major_axis * (1.0 - eccentricity * eccentricity));
    double flight_path_angle = atan2(eccentricity * sin(initial_angle), 
                                     1.0 + eccentricity * cos(initial_angle));
    
    double vel_angle = initial_angle + M_PI / 2.0 + flight_path_angle;
    
    double vx = parent->vx + vel * cos(vel_angle);
    double vy = parent->vy + vel * sin(vel_angle);
    
    return (PhysicalBody){
        .x = x,
        .y = y,
        .vx = vx,
        .vy = vy,
        .mass = mass,
        .radius = radius,
        .color = color,
        .name = name,
    };
}

static PhysicalBody create_from_orbital_elements(
    const PhysicalBody* parent,
    const OrbitalElements* elements,
    double mass,
    float radius,
    Color color,
    const char* name)
{
    double periapsis = elements->semi_major_axis * (1.0 - elements->eccentricity);
    double apoapsis = elements->semi_major_axis * (1.0 + elements->eccentricity);
    
    return create_elliptical_orbit(parent, periapsis, apoapsis, 
                                   elements->longitude, mass, radius, color, name);
}

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
 const double AU = 1.496e11;


// ================= SUN =================
PhysicalBody sun = {
.x = 0.0, .y = 0.0,
.vx = 0.0, .vy = 0.0,
.mass = 1.9885e30,
.radius = 6.9634e8f,
.color = YELLOW,
.name = "Sun"
};
BodyId sun_id = sim_add_body(sim, sun);


// ================= PLANETS (ELLIPTICAL ORBITS) =================


// Mercury (high eccentricity)
BodyId mercury = sim_add_body_elliptical_orbit(sim, sun_id,
0.3075*AU, 0.4667*AU, 0.0,
3.3011e23, 2.4397e6f, (Color){169,169,169,255}, "Mercury");


// Venus
BodyId venus = sim_add_body_elliptical_orbit(sim, sun_id,
0.7184*AU, 0.7282*AU, 0.0,
4.8675e24, 6.0518e6f, ORANGE, "Venus");


// Earth
BodyId earth = sim_add_body_elliptical_orbit(sim, sun_id,
0.9833*AU, 1.0167*AU, 0.0,
5.97237e24, 6.371e6f, BLUE, "Earth");


// Mars
BodyId mars = sim_add_body_elliptical_orbit(sim, sun_id,
1.3814*AU, 1.6660*AU, 0.0,
6.4171e23, 3.3895e6f, RED, "Mars");


// Jupiter
BodyId jupiter = sim_add_body_elliptical_orbit(sim, sun_id,
4.9501*AU, 5.4588*AU, 0.0,
1.8982e27, 6.9911e7f, (Color){194,178,128,255}, "Jupiter");


// Saturn
BodyId saturn = sim_add_body_elliptical_orbit(sim, sun_id,
9.0240*AU, 10.1238*AU, 0.0,
5.6834e26, 5.8232e7f, (Color){238,214,175,255}, "Saturn");


// Uranus
BodyId uranus = sim_add_body_elliptical_orbit(sim, sun_id,
18.286*AU, 20.096*AU, 0.0,
8.6810e25, 2.5362e7f, (Color){79,208,231,255}, "Uranus");


// Neptune
BodyId neptune = sim_add_body_elliptical_orbit(sim, sun_id,
29.81*AU, 30.33*AU, 0.0,
1.02413e26, 2.4622e7f, (Color){63,84,186,255}, "Neptune");


// ================= MOON SYSTEMS =================


// Earth
sim_add_body_elliptical_orbit(sim, earth,
3.633e8, 4.055e8, 0.0,
7.342e22, 1.737e6f, LIGHTGRAY, "Moon");


// Mars
sim_add_body_elliptical_orbit(sim, mars,
9.234e6, 9.517e6, 0.0,
1.0659e16, 1.1267e4f, (Color){120,120,120,255}, "Phobos");


sim_add_body_elliptical_orbit(sim, mars,
2.340e7, 2.346e7, 0.0,
1.4762e15, 6.2e3f, (Color){160,160,160,255}, "Deimos");


// Jupiter (Galilean moons)
sim_add_body_elliptical_orbit(sim, jupiter,
4.201e8, 4.233e8, 0.0,
8.9319e22, 1.8216e6f, (Color){255,255,150,255}, "Io");


sim_add_body_elliptical_orbit(sim, jupiter,
6.644e8, 6.778e8, 0.0,
4.7998e22, 1.5608e6f, (Color){200,200,180,255}, "Europa");


sim_add_body_elliptical_orbit(sim, jupiter,
1.069e9, 1.072e9, 0.0,
1.4819e23, 2.6341e6f, (Color){150,150,150,255}, "Ganymede");


sim_add_body_elliptical_orbit(sim, jupiter,
1.882e9, 1.884e9, 0.0,
1.0759e23, 2.4103e6f, (Color){120,120,120,255}, "Callisto");


// Saturn
sim_add_body_elliptical_orbit(sim, saturn,
1.186e9, 1.258e9, 0.0,
1.3452e23, 2.5747e6f, (Color){255,200,150,255}, "Titan");


sim_add_body_elliptical_orbit(sim, saturn,
2.379e8, 2.381e8, 0.0,
1.0802e20, 2.52e5f, (Color){255,255,255,255}, "Enceladus");


// Uranus
sim_add_body_elliptical_orbit(sim, uranus,
4.356e8, 4.360e8, 0.0,
3.527e21, 7.88e5f, (Color){200,200,200,255}, "Titania");


// Neptune
sim_add_body_elliptical_orbit(sim, neptune,
3.548e8, 3.548e8, 0.0,
2.14e22, 1.353e6f, (Color){220,220,220,255}, "Triton");
}

void sim_init(SimContext* sim, Arena* arena) {
    sim->sim_arena = arena;
    array_init(&sim->bodies, 32, arena);
    array_init(&sim->trails, 32, arena);
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

BodyId sim_add_body_circular_orbit(SimContext* sim, BodyId parent_id,
                                   double orbit_radius, double initial_angle,
                                   double mass, float radius, Color color, const char* name)
{
    if (parent_id >= sim->bodies.length) {
        return (BodyId)-1;
    }
    
    const PhysicalBody* parent = &sim->bodies.data[parent_id];
    PhysicalBody body = create_circular_orbit(parent, orbit_radius, initial_angle,
                                              mass, radius, color, name);
    return sim_add_body(sim, body);
}

BodyId sim_add_body_elliptical_orbit(SimContext* sim, BodyId parent_id,
                                     double periapsis, double apoapsis, double initial_angle,
                                     double mass, float radius, Color color, const char* name)
{
    if (parent_id >= sim->bodies.length) {
        return (BodyId)-1;
    }
    
    const PhysicalBody* parent = &sim->bodies.data[parent_id];
    PhysicalBody body = create_elliptical_orbit(parent, periapsis, apoapsis, initial_angle,
                                                mass, radius, color, name);
    return sim_add_body(sim, body);
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
    const int label_font_size = 12;

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

    }

    typedef struct {
        int x;
        int y;
        int w;
        int h;
        double size_score;
        const char* name;
    } LabelCandidate;

    size_t arena_start = sim->sim_arena->offset;
    LabelCandidate* candidates = (LabelCandidate*)arena_alloc(sim->sim_arena,
        sim->bodies.length * sizeof(LabelCandidate));
    if (!candidates) {
        sim->sim_arena->offset = arena_start;
        return;
    }

    size_t candidate_count = 0;
    for (size_t i = 0; i < sim->bodies.length; i += 1) {
        const PhysicalBody* body = &sim->bodies.data[i];
        if (!body->name || !body->name[0]) {
            continue;
        }

        double sx = (body->x - cam_x) * zoom + half_w;
        double sy = (body->y - cam_y) * zoom + half_h;
        double sr = (double)body->radius * zoom;
        if (sr < 2.0) sr = 2.0;

        int text_w = MeasureText(body->name, label_font_size);
        int text_h = label_font_size;
        int tx = (int)(sx + sr + 4.0);
        int ty = (int)(sy - text_h / 2);

        candidates[candidate_count++] = (LabelCandidate){
            .x = tx,
            .y = ty,
            .w = text_w,
            .h = text_h,
            .size_score = sr,
            .name = body->name,
        };
    }

    for (size_t i = 0; i < candidate_count; i++) {
        for (size_t j = i + 1; j < candidate_count; j++) {
            if (candidates[j].size_score > candidates[i].size_score) {
                LabelCandidate tmp = candidates[i];
                candidates[i] = candidates[j];
                candidates[j] = tmp;
            }
        }
    }

    for (size_t i = 0; i < candidate_count; i++) {
        bool overlaps = false;
        for (size_t j = 0; j < i; j++) {
            int ax1 = candidates[i].x;
            int ay1 = candidates[i].y;
            int ax2 = ax1 + candidates[i].w;
            int ay2 = ay1 + candidates[i].h;

            int bx1 = candidates[j].x;
            int by1 = candidates[j].y;
            int bx2 = bx1 + candidates[j].w;
            int by2 = by1 + candidates[j].h;

            if (ax1 < bx2 && ax2 > bx1 && ay1 < by2 && ay2 > by1) {
                overlaps = true;
                break;
            }
        }

        if (!overlaps) {
            DrawText(candidates[i].name, candidates[i].x, candidates[i].y, label_font_size, LIGHTGRAY);
        }
    }

    sim->sim_arena->offset = arena_start;
}