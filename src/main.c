#include "raylib.h"
#include "sim.h"

#include <stdio.h>

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

        const bool step_once = IsKeyPressed(KEY_N);
        if (!paused) {
            sim_step(&sim, time_scale * GetFrameTime());
        } else if (step_once) {
            sim_step(&sim, time_scale);
        }

        BeginDrawing();
        ClearBackground((Color){10, 12, 20, 255});

        sim_draw(&sim, cam_x, cam_y, cam_zoom, screen_width, screen_height);

        DrawRectangle(12, 12, 360, 102, (Color){15, 18, 30, 220});
        DrawRectangleLines(12, 12, 360, 102, (Color){90, 100, 120, 255});

        char hud[256];
        snprintf(hud, sizeof(hud), "Bodies: %zu\nTime: %.2f days\nSpeed: %.0fx\nSpace: pause  N: step", sim.bodies.length, sim.time_seconds / 86400.0, time_scale);
        DrawText(hud, 24, 22, 16, RAYWHITE);
        DrawText("+/-: speed  Mouse wheel: zoom  Middle drag: pan", 24, 100, 12, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    free_arena(arena);
    return 0;
}