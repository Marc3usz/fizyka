#include "sim.h"

#include <math.h>
#include <string.h>

#define G 6.67430e-11L
#define TRAIL_LENGTH 2000
#define TRAIL_RECORD_INTERVAL 5
#define MM_PER_METER_L 1000.0L

typedef struct {
    long double ax_mm_s2;
    long double ay_mm_s2;
} BodyAccel;

static inline long double i128_to_ld(i128 value) {
    return (long double)value;
}

static inline i128 ld_to_i128(long double value) {
    if (value >= 0.0L) {
        return (i128)(value + 0.5L);
    }
    return (i128)(value - 0.5L);
}

static long double orbital_velocity_at_distance(double central_mass, double distance_m, double semi_major_axis_m) {
    return sqrtl(G * (long double)central_mass *
                 (2.0L / (long double)distance_m - 1.0L / (long double)semi_major_axis_m));
}

static long double circular_orbital_velocity(double central_mass, double radius_m) {
    return sqrtl(G * (long double)central_mass / (long double)radius_m);
}

static PhysicalBody create_circular_orbit(
    const PhysicalBody* parent,
    double orbit_radius_m,
    double initial_angle,
    double mass,
    double radius_m,
    Color color,
    const char* name)
{
    long double vel_m_s = circular_orbital_velocity(parent->mass, orbit_radius_m);
    i128 orbit_mm = sim_m_to_mm(orbit_radius_m);

    long double cos_angle = cosl((long double)initial_angle);
    long double sin_angle = sinl((long double)initial_angle);

    i128 x_mm = parent->x_mm + ld_to_i128(i128_to_ld(orbit_mm) * cos_angle);
    i128 y_mm = parent->y_mm + ld_to_i128(i128_to_ld(orbit_mm) * sin_angle);

    i128 vx_mm_s = parent->vx_mm_s - ld_to_i128(vel_m_s * MM_PER_METER_L * sin_angle);
    i128 vy_mm_s = parent->vy_mm_s + ld_to_i128(vel_m_s * MM_PER_METER_L * cos_angle);

    return (PhysicalBody){
        .x_mm = x_mm,
        .y_mm = y_mm,
        .vx_mm_s = vx_mm_s,
        .vy_mm_s = vy_mm_s,
        .mass = mass,
        .radius_mm = sim_m_to_mm(radius_m),
        .color = color,
        .name = name,
    };
}

static PhysicalBody create_elliptical_orbit(
    const PhysicalBody* parent,
    double periapsis_m,
    double apoapsis_m,
    double initial_angle,
    double mass,
    double radius_m,
    Color color,
    const char* name)
{
    long double semi_major_axis_m = ((long double)periapsis_m + (long double)apoapsis_m) * 0.5L;
    long double eccentricity = ((long double)apoapsis_m - (long double)periapsis_m) /
                               ((long double)apoapsis_m + (long double)periapsis_m);

    long double cos_angle = cosl((long double)initial_angle);
    long double sin_angle = sinl((long double)initial_angle);

    long double r_m = semi_major_axis_m * (1.0L - eccentricity * eccentricity) /
                      (1.0L + eccentricity * cos_angle);

    long double vel_m_s = orbital_velocity_at_distance(parent->mass, (double)r_m, (double)semi_major_axis_m);

    i128 r_mm = ld_to_i128(r_m * MM_PER_METER_L);
    i128 x_mm = parent->x_mm + ld_to_i128(i128_to_ld(r_mm) * cos_angle);
    i128 y_mm = parent->y_mm + ld_to_i128(i128_to_ld(r_mm) * sin_angle);

    long double flight_path_angle = atan2l(eccentricity * sin_angle,
                                           1.0L + eccentricity * cos_angle);

    long double vel_angle = (long double)initial_angle + (long double)M_PI / 2.0L + flight_path_angle;

    i128 vx_mm_s = parent->vx_mm_s + ld_to_i128(vel_m_s * MM_PER_METER_L * cosl(vel_angle));
    i128 vy_mm_s = parent->vy_mm_s + ld_to_i128(vel_m_s * MM_PER_METER_L * sinl(vel_angle));

    return (PhysicalBody){
        .x_mm = x_mm,
        .y_mm = y_mm,
        .vx_mm_s = vx_mm_s,
        .vy_mm_s = vy_mm_s,
        .mass = mass,
        .radius_mm = sim_m_to_mm(radius_m),
        .color = color,
        .name = name,
    };
}

static void trail_init(TrailBuffer* trail, Arena* arena) {
    trail->points = (TrailPoint*)arena_alloc(arena, TRAIL_LENGTH * sizeof(TrailPoint));
    trail->capacity = trail->points ? TRAIL_LENGTH : 0;
    trail->head = 0;
    trail->count = 0;
}

static void trail_add_point(TrailBuffer* trail, i128 x_mm, i128 y_mm) {
    if (!trail->points || trail->capacity == 0) {
        return;
    }
    trail->points[trail->head].x_mm = x_mm;
    trail->points[trail->head].y_mm = y_mm;
    trail->head = (trail->head + 1) % trail->capacity;
    if (trail->count < trail->capacity) {
        trail->count++;
    }
}

static void sim_seed_solar_system(SimContext* sim) {
    const double AU = 1.496e11;

    PhysicalBody sun = {
        .x_mm = 0,
        .y_mm = 0,
        .vx_mm_s = 0,
        .vy_mm_s = 0,
        .mass = 1.9885e30,
        .radius_mm = sim_m_to_mm(6.9634e8),
        .color = YELLOW,
        .name = "Sun"
    };
    BodyId sun_id = sim_add_body(sim, sun);

    BodyId mercury = sim_add_body_elliptical_orbit(sim, sun_id,
        0.3075 * AU, 0.4667 * AU, 0.0,
        3.3011e23, 2.4397e6, (Color){169, 169, 169, 255}, "Mercury");

    BodyId venus = sim_add_body_elliptical_orbit(sim, sun_id,
        0.7184 * AU, 0.7282 * AU, 0.0,
        4.8675e24, 6.0518e6, ORANGE, "Venus");

    BodyId earth = sim_add_body_elliptical_orbit(sim, sun_id,
        0.9833 * AU, 1.0167 * AU, 0.0,
        5.97237e24, 6.371e6, BLUE, "Earth");

    BodyId mars = sim_add_body_elliptical_orbit(sim, sun_id,
        1.3814 * AU, 1.6660 * AU, 0.0,
        6.4171e23, 3.3895e6, RED, "Mars");

    BodyId jupiter = sim_add_body_elliptical_orbit(sim, sun_id,
        4.9501 * AU, 5.4588 * AU, 0.0,
        1.8982e27, 6.9911e7, (Color){194, 178, 128, 255}, "Jupiter");

    BodyId saturn = sim_add_body_elliptical_orbit(sim, sun_id,
        9.0240 * AU, 10.1238 * AU, 0.0,
        5.6834e26, 5.8232e7, (Color){238, 214, 175, 255}, "Saturn");

    BodyId uranus = sim_add_body_elliptical_orbit(sim, sun_id,
        18.286 * AU, 20.096 * AU, 0.0,
        8.6810e25, 2.5362e7, (Color){79, 208, 231, 255}, "Uranus");

    BodyId neptune = sim_add_body_elliptical_orbit(sim, sun_id,
        29.81 * AU, 30.33 * AU, 0.0,
        1.02413e26, 2.4622e7, (Color){63, 84, 186, 255}, "Neptune");

    sim_add_body_elliptical_orbit(sim, earth,
        3.633e8, 4.055e8, 0.0,
        7.342e22, 1.737e6, LIGHTGRAY, "Moon");

    sim_add_body_elliptical_orbit(sim, mars,
        9.234e6, 9.517e6, 0.0,
        1.0659e16, 1.1267e4, (Color){120, 120, 120, 255}, "Phobos");

    sim_add_body_elliptical_orbit(sim, mars,
        2.340e7, 2.346e7, 0.0,
        1.4762e15, 6.2e3, (Color){160, 160, 160, 255}, "Deimos");

    sim_add_body_elliptical_orbit(sim, jupiter,
        4.201e8, 4.233e8, 0.0,
        8.9319e22, 1.8216e6, (Color){255, 255, 150, 255}, "Io");

    sim_add_body_elliptical_orbit(sim, jupiter,
        6.644e8, 6.778e8, 0.0,
        4.7998e22, 1.5608e6, (Color){200, 200, 180, 255}, "Europa");

    sim_add_body_elliptical_orbit(sim, jupiter,
        1.069e9, 1.072e9, 0.0,
        1.4819e23, 2.6341e6, (Color){150, 150, 150, 255}, "Ganymede");

    sim_add_body_elliptical_orbit(sim, jupiter,
        1.882e9, 1.884e9, 0.0,
        1.0759e23, 2.4103e6, (Color){120, 120, 120, 255}, "Callisto");

    sim_add_body_elliptical_orbit(sim, saturn,
        1.186e9, 1.258e9, 0.0,
        1.3452e23, 2.5747e6, (Color){255, 200, 150, 255}, "Titan");

    sim_add_body_elliptical_orbit(sim, saturn,
        2.379e8, 2.381e8, 0.0,
        1.0802e20, 2.52e5, (Color){255, 255, 255, 255}, "Enceladus");

    sim_add_body_elliptical_orbit(sim, uranus,
        4.356e8, 4.360e8, 0.0,
        3.527e21, 7.88e5, (Color){200, 200, 200, 255}, "Titania");

    sim_add_body_elliptical_orbit(sim, neptune,
        3.548e8, 3.548e8, 0.0,
        2.14e22, 1.353e6, (Color){220, 220, 220, 255}, "Triton");

    (void)mercury;
    (void)venus;
}

void sim_init(SimContext* sim, Arena* arena) {
    sim->sim_arena = arena;
    array_init(&sim->bodies, 32, arena);
    array_init(&sim->trails, 32, arena);
    sim->time_seconds = 0.0;
    sim->trail_frame_counter = 0;
    sim->integrator = SIM_INTEGRATOR_YOSHIDA_RUTH_4;

    sim_seed_solar_system(sim);
}

void sim_reset(SimContext* sim) {
    array_clear(&sim->bodies);
    array_clear(&sim->trails);
    sim->time_seconds = 0.0;
    sim->trail_frame_counter = 0;

    sim_seed_solar_system(sim);
}

void sim_set_integrator(SimContext* sim, SimIntegrator integrator) {
    if (!sim) {
        return;
    }
    switch (integrator) {
        case SIM_INTEGRATOR_VELOCITY_VERLET:
        case SIM_INTEGRATOR_YOSHIDA_RUTH_4:
            sim->integrator = integrator;
            break;
        default:
            break;
    }
}

SimIntegrator sim_get_integrator(const SimContext* sim) {
    if (!sim) {
        return SIM_INTEGRATOR_VELOCITY_VERLET;
    }
    return sim->integrator;
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
                                   double mass, double radius_m, Color color, const char* name)
{
    if (parent_id >= sim->bodies.length) {
        return (BodyId)-1;
    }

    const PhysicalBody* parent = &sim->bodies.data[parent_id];
    PhysicalBody body = create_circular_orbit(parent, orbit_radius, initial_angle,
                                              mass, radius_m, color, name);
    return sim_add_body(sim, body);
}

BodyId sim_add_body_elliptical_orbit(SimContext* sim, BodyId parent_id,
                                     double periapsis, double apoapsis, double initial_angle,
                                     double mass, double radius_m, Color color, const char* name)
{
    if (parent_id >= sim->bodies.length) {
        return (BodyId)-1;
    }

    const PhysicalBody* parent = &sim->bodies.data[parent_id];
    PhysicalBody body = create_elliptical_orbit(parent, periapsis, apoapsis, initial_angle,
                                                mass, radius_m, color, name);
    return sim_add_body(sim, body);
}

static void compute_accelerations(SimContext* sim, BodyAccel* accels) {
    const size_t count = sim->bodies.length;

    for (size_t i = 0; i < count; i++) {
        accels[i].ax_mm_s2 = 0.0L;
        accels[i].ay_mm_s2 = 0.0L;
    }

    for (size_t i = 0; i < count; i += 1) {
        const PhysicalBody* body_i = &sim->bodies.data[i];
        for (size_t j = i + 1; j < count; j += 1) {
            const PhysicalBody* body_j = &sim->bodies.data[j];

            i128 dx_mm = body_j->x_mm - body_i->x_mm;
            i128 dy_mm = body_j->y_mm - body_i->y_mm;

            if (dx_mm == 0 && dy_mm == 0) {
                continue;
            }

            long double dx_m = i128_to_ld(dx_mm) / MM_PER_METER_L;
            long double dy_m = i128_to_ld(dy_mm) / MM_PER_METER_L;
            long double dist2 = dx_m * dx_m + dy_m * dy_m;

            if (dist2 <= 0.0L) {
                continue;
            }

            long double dist = sqrtl(dist2);
            long double inv_dist3 = 1.0L / (dist2 * dist);

            long double accel_i_scalar = G * (long double)body_j->mass * inv_dist3;
            long double accel_j_scalar = G * (long double)body_i->mass * inv_dist3;

            long double ax_i_m_s2 = accel_i_scalar * dx_m;
            long double ay_i_m_s2 = accel_i_scalar * dy_m;
            long double ax_j_m_s2 = accel_j_scalar * dx_m;
            long double ay_j_m_s2 = accel_j_scalar * dy_m;

            accels[i].ax_mm_s2 += ax_i_m_s2 * MM_PER_METER_L;
            accels[i].ay_mm_s2 += ay_i_m_s2 * MM_PER_METER_L;
            accels[j].ax_mm_s2 -= ax_j_m_s2 * MM_PER_METER_L;
            accels[j].ay_mm_s2 -= ay_j_m_s2 * MM_PER_METER_L;
        }
    }
}

static void apply_drift(SimContext* sim, long double dt_seconds) {
    const size_t count = sim->bodies.length;
    for (size_t i = 0; i < count; i++) {
        PhysicalBody* body = &sim->bodies.data[i];
        long double delta_x_mm = i128_to_ld(body->vx_mm_s) * dt_seconds;
        long double delta_y_mm = i128_to_ld(body->vy_mm_s) * dt_seconds;
        body->x_mm += ld_to_i128(delta_x_mm);
        body->y_mm += ld_to_i128(delta_y_mm);
    }
}

static void apply_kick(SimContext* sim, const BodyAccel* accels, long double dt_seconds) {
    const size_t count = sim->bodies.length;
    for (size_t i = 0; i < count; i++) {
        PhysicalBody* body = &sim->bodies.data[i];
        long double delta_vx_mm_s = accels[i].ax_mm_s2 * dt_seconds;
        long double delta_vy_mm_s = accels[i].ay_mm_s2 * dt_seconds;
        body->vx_mm_s += ld_to_i128(delta_vx_mm_s);
        body->vy_mm_s += ld_to_i128(delta_vy_mm_s);
    }
}

static void finish_sim_step(SimContext* sim, double dt_seconds) {
    const size_t count = sim->bodies.length;

    sim->trail_frame_counter++;
    if (sim->trail_frame_counter >= TRAIL_RECORD_INTERVAL) {
        const size_t trail_count = sim->trails.length;
        const size_t min_count = count < trail_count ? count : trail_count;
        for (size_t i = 0; i < min_count; i++) {
            const PhysicalBody* body = &sim->bodies.data[i];
            trail_add_point(&sim->trails.data[i], body->x_mm, body->y_mm);
        }
        sim->trail_frame_counter = 0;
    }

    sim->time_seconds += dt_seconds;
}

static void sim_step_velocity_verlet(SimContext* sim, double dt_seconds) {
    const size_t count = sim->bodies.length;
    if (count == 0 || dt_seconds <= 0.0) {
        return;
    }

    long double dt = (long double)dt_seconds;
    const size_t accel_bytes = count * sizeof(BodyAccel);

    BodyAccel* accels = (BodyAccel*)arena_alloc(sim->sim_arena, accel_bytes);
    BodyAccel* new_accels = (BodyAccel*)arena_alloc(sim->sim_arena, accel_bytes);

    if (!accels || !new_accels) {
        if (new_accels) {
            arena_dealloc(sim->sim_arena, accel_bytes);
        }
        if (accels) {
            arena_dealloc(sim->sim_arena, accel_bytes);
        }
        return;
    }

    memset(accels, 0, accel_bytes);
    memset(new_accels, 0, accel_bytes);

    compute_accelerations(sim, accels);

    for (size_t i = 0; i < count; i += 1) {
        PhysicalBody* body = &sim->bodies.data[i];

        long double delta_x_mm = i128_to_ld(body->vx_mm_s) * dt +
                                 0.5L * accels[i].ax_mm_s2 * dt * dt;
        long double delta_y_mm = i128_to_ld(body->vy_mm_s) * dt +
                                 0.5L * accels[i].ay_mm_s2 * dt * dt;

        body->x_mm += ld_to_i128(delta_x_mm);
        body->y_mm += ld_to_i128(delta_y_mm);
    }

    compute_accelerations(sim, new_accels);

    for (size_t i = 0; i < count; i += 1) {
        PhysicalBody* body = &sim->bodies.data[i];

        long double delta_vx_mm_s = 0.5L * (accels[i].ax_mm_s2 + new_accels[i].ax_mm_s2) * dt;
        long double delta_vy_mm_s = 0.5L * (accels[i].ay_mm_s2 + new_accels[i].ay_mm_s2) * dt;

        body->vx_mm_s += ld_to_i128(delta_vx_mm_s);
        body->vy_mm_s += ld_to_i128(delta_vy_mm_s);
    }

    finish_sim_step(sim, dt_seconds);

    arena_dealloc(sim->sim_arena, accel_bytes);
    arena_dealloc(sim->sim_arena, accel_bytes);
}

static void sim_step_yoshida_ruth_4(SimContext* sim, double dt_seconds) {
    const size_t count = sim->bodies.length;
    if (count == 0 || dt_seconds <= 0.0) {
        return;
    }

    const long double dt = (long double)dt_seconds;
    const long double cbrt2 = cbrtl(2.0L);
    const long double w1 = 1.0L / (2.0L - cbrt2);
    const long double w0 = -cbrt2 / (2.0L - cbrt2);

    const long double c1 = 0.5L * w1;
    const long double c2 = 0.5L * (w0 + w1);
    const long double c3 = c2;
    const long double c4 = c1;

    const long double d1 = w1;
    const long double d2 = w0;
    const long double d3 = w1;

    const size_t accel_bytes = count * sizeof(BodyAccel);
    BodyAccel* accels = (BodyAccel*)arena_alloc(sim->sim_arena, accel_bytes);
    if (!accels) {
        return;
    }

    compute_accelerations(sim, accels);
    apply_kick(sim, accels, d1 * dt);
    apply_drift(sim, c1 * dt);

    compute_accelerations(sim, accels);
    apply_kick(sim, accels, d2 * dt);
    apply_drift(sim, c2 * dt);

    compute_accelerations(sim, accels);
    apply_kick(sim, accels, d3 * dt);
    apply_drift(sim, c3 * dt);
    apply_drift(sim, c4 * dt);

    finish_sim_step(sim, dt_seconds);

    arena_dealloc(sim->sim_arena, accel_bytes);
}

void sim_step(SimContext* sim, double dt_seconds) {
    switch (sim_get_integrator(sim)) {
        case SIM_INTEGRATOR_VELOCITY_VERLET:
            sim_step_velocity_verlet(sim, dt_seconds);
            break;
        case SIM_INTEGRATOR_YOSHIDA_RUTH_4:
        default:
            sim_step_yoshida_ruth_4(sim, dt_seconds);
            break;
    }
}

void sim_draw(const SimContext* sim, i128 cam_x_mm, i128 cam_y_mm, double zoom_px_per_mm, int screen_w, int screen_h) {
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

            double x1 = (double)(i128_to_ld(trail->points[idx].x_mm - cam_x_mm) * (long double)zoom_px_per_mm + (long double)half_w);
            double y1 = (double)(i128_to_ld(trail->points[idx].y_mm - cam_y_mm) * (long double)zoom_px_per_mm + (long double)half_h);
            double x2 = (double)(i128_to_ld(trail->points[next_idx].x_mm - cam_x_mm) * (long double)zoom_px_per_mm + (long double)half_w);
            double y2 = (double)(i128_to_ld(trail->points[next_idx].y_mm - cam_y_mm) * (long double)zoom_px_per_mm + (long double)half_h);

            float alpha_ratio = (float)j / (float)trail->count;
            unsigned char alpha = (unsigned char)(alpha_ratio * 180.0f + 20.0f);

            Color trail_color = body->color;
            trail_color.a = alpha;

            // Cull trail segments entirely off-screen
            if ((x1 < 0 && x2 < 0) || (x1 > screen_w && x2 > screen_w) ||
                (y1 < 0 && y2 < 0) || (y1 > screen_h && y2 > screen_h)) continue;

            DrawLineEx((Vector2){(float)x1, (float)y1},
                      (Vector2){(float)x2, (float)y2},
                      1.0f,
                      trail_color);
        }
    }

    for (size_t i = 0; i < sim->bodies.length; i += 1) {
        const PhysicalBody* body = &sim->bodies.data[i];

        i128 dx_mm = body->x_mm - cam_x_mm;
        i128 dy_mm = body->y_mm - cam_y_mm;

        long double sx_ld = i128_to_ld(dx_mm) * (long double)zoom_px_per_mm + (long double)half_w;
        long double sy_ld = i128_to_ld(dy_mm) * (long double)zoom_px_per_mm + (long double)half_h;
        long double sr_ld = i128_to_ld(body->radius_mm) * (long double)zoom_px_per_mm;

        if (sr_ld < 2.0L) sr_ld = 2.0L;

        if (sx_ld + sr_ld < 0 || sx_ld - sr_ld > screen_w ||
            sy_ld + sr_ld < 0 || sy_ld - sr_ld > screen_h) continue;

        double sx = (double)sx_ld;
        double sy = (double)sy_ld;
        double sr = (double)sr_ld;

        if (sr < 4000.0) {
            DrawCircle((int)sx, (int)sy, (float)sr, body->color);
        } else {
            int y_start = (int)fmax(0.0, sy - sr);
            int y_end   = (int)fmin((double)screen_h, sy + sr + 1.0);
            if (y_start >= screen_h || y_end <= 0) continue;

            i128 radius_mm = body->radius_mm;

            for (int py = y_start; py < y_end; py++) {
                long double row_offset_px = (long double)py + 0.5L - sy_ld;
                long double row_dy_mm = row_offset_px / (long double)zoom_px_per_mm;
                long double abs_dy_mm = fabsl(row_dy_mm);

                long double r_mm_ld = i128_to_ld(radius_mm);
                long double sum  = r_mm_ld + abs_dy_mm;
                long double diff = r_mm_ld - abs_dy_mm;
                if (diff < 0.0L) continue;

                long double dx_mm_ld = sqrtl(sum * diff);
                long double half_width_px = dx_mm_ld * (long double)zoom_px_per_mm;

                int x_left  = (int)fmax(0.0, (double)(sx_ld - half_width_px));
                int x_right = (int)fmin((double)screen_w, (double)(sx_ld + half_width_px + 1.0L));

                if (x_left < x_right) {
                    DrawRectangle(x_left, py, x_right - x_left, 1, body->color);
                }
            }
        }
    }

    typedef struct {
        int x;
        int y;
        int w;
        int h;
        double size_score;
        const char* name;
    } LabelCandidate;

    const size_t candidate_bytes = sim->bodies.length * sizeof(LabelCandidate);
    LabelCandidate* candidates = (LabelCandidate*)arena_alloc(sim->sim_arena,
        candidate_bytes);
    if (!candidates) {
        return;
    }

    size_t candidate_count = 0;
    for (size_t i = 0; i < sim->bodies.length; i += 1) {
        const PhysicalBody* body = &sim->bodies.data[i];
        if (!body->name || !body->name[0]) {
            continue;
        }

        i128 ldx_mm = body->x_mm - cam_x_mm;
        i128 ldy_mm = body->y_mm - cam_y_mm;
        double sx = (double)(i128_to_ld(ldx_mm) * (long double)zoom_px_per_mm + (long double)half_w);
        double sy = (double)(i128_to_ld(ldy_mm) * (long double)zoom_px_per_mm + (long double)half_h);
        double sr = (double)(i128_to_ld(body->radius_mm) * (long double)zoom_px_per_mm);
        if (sr < 2.0) sr = 2.0;

        // Skip labels for bodies that engulf the entire viewport
        if (sr > screen_w * 2.0 && sr > screen_h * 2.0) continue;

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

    arena_dealloc(sim->sim_arena, candidate_bytes);
}
