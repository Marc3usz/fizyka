#include "raylib.h"
#include "sim.h"

#include <stdio.h>

typedef struct {
    double elapsed_seconds;
    bool running;
} Timer;

void timer_reset(Timer* timer) {
    timer->elapsed_seconds = 0.0;
    timer->running = false;
}

void timer_start(Timer* timer) {
    timer->running = true;
}

void timer_pause(Timer* timer) {
    timer->running = false;
}

void timer_toggle(Timer* timer) {
    timer->running = !timer->running;
}

void timer_update(Timer* timer, double dt) {
    if (timer->running) {
        timer->elapsed_seconds += dt;
    }
}

int main(void) {
    const int screen_width = 1200;
    const int screen_height = 800;
    double time_scale = 3600.0;
    const double min_time_scale = 1.0;
    const double max_time_scale = 86400.0 * 365.0;

    InitWindow(screen_width, screen_height, "Fizyka - Gravity Sim");
    SetTargetFPS(600);

    Arena* arena = init_arena(10 * 1024 * 1024);
    SimContext sim = {0};
    sim_init(&sim, arena);

    double cam_x = 0.0;
    double cam_y = 0.0;
    double cam_zoom = 3.0e-9;

    bool paused = false;
    
    Timer timer = {0};

    while (!WindowShouldClose()) {
        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            cam_zoom *= (1.0 + wheel * 0.15);
            if (cam_zoom < 1.0e-14) cam_zoom = 1.0e-14;
            if (cam_zoom > 1.0e2) cam_zoom = 1.0e2;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 delta = GetMouseDelta();
            cam_x -= (double)delta.x / cam_zoom;
            cam_y -= (double)delta.y / cam_zoom;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }

        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            time_scale *= 2.0;
            if (time_scale > max_time_scale) {
                time_scale = max_time_scale;
            }
        }

        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            time_scale *= 0.5;
            if (time_scale < min_time_scale) {
                time_scale = min_time_scale;
            }
        }

        if (IsKeyPressed(KEY_T)) {
            timer_toggle(&timer);
        }
        if (IsKeyPressed(KEY_R)) {
            timer_reset(&timer);
        }

        const bool step_once = IsKeyPressed(KEY_N);
        double sim_dt = 0.0;
        
        if (!paused) {
            sim_dt = time_scale * GetFrameTime();
            sim_step(&sim, sim_dt);
        } else if (step_once) {
            sim_dt = time_scale;
            sim_step(&sim, sim_dt);
        }

        timer_update(&timer, sim_dt);

        BeginDrawing();
        ClearBackground((Color){10, 12, 20, 255});

        sim_draw(&sim, cam_x, cam_y, cam_zoom, screen_width, screen_height);

        DrawRectangle(12, 12, 360, 130, (Color){15, 18, 30, 220});
        DrawRectangleLines(12, 12, 360, 130, (Color){90, 100, 120, 255});

        char hud[512];
        snprintf(hud, sizeof(hud), 
                 "Bodies: %zu\nTime: %.2f days\nSpeed: %.0fx\nTimer: %.2f days %s\nSpace: pause  N: step",
                 sim.bodies.length, 
                 sim.time_seconds / 86400.0, 
                 time_scale,
                 timer.elapsed_seconds / 86400.0,
                 timer.running ? "[RUNNING]" : "[PAUSED]");
        DrawText(hud, 24, 22, 16, RAYWHITE);
        DrawText("+/-: speed  Mouse wheel: zoom  Middle drag: pan", 24, 114, 12, LIGHTGRAY);
        DrawText("T: toggle timer  R: reset timer", 24, 128, 12, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    free_arena(arena);
    return 0;
}