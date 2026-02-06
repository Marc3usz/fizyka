typedef struct {
    double x, y;
    double vx, vy;
    double mass;
} PhysicalBody;

typedef struct {
    double dvx, dvy;
} PhysicalSimStep;

void apply_sim_step(PhysicalBody* body, PhysicalSimStep* step, double dt) {
    body->vx += step->dvx;
    body->vy += step->dvy;
    body->x += body->vx * dt;
    body->y += body->vy * dt;
}
