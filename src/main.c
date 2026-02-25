#include "raylib.h"
#include "sim.h"
#include "cli_args.h"
#include "measurement_tools.h"
#include "hud.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    // initialize program

    const int screen_width = 1200;
    const int screen_height = 800;
    const double min_time_scale = 1.0;
    const double max_time_scale = 86400.0 * 365.0;

    SimCliOptions options = {0};
    const char *arg_error = NULL;
    if (!parse_sim_cli_args(argc, argv, &options, &arg_error))
    {
        if (arg_error)
        {
            fprintf(stderr, "Argument error: %s\n", arg_error);
        }
        fprintf(stderr, "%s\n", sim_cli_usage(argv[0]));
        return 1;
    }

    InitWindow(screen_width, screen_height, "Fizyka - Gravity Sim");
    SetTargetFPS(60);

    Arena *arena = init_arena(10 * 1024 * 1024);
    SimContext sim = {0};
    sim_init(&sim, arena);
    sim_set_integrator(&sim, options.integrator);

    i128 cam_x_mm = 0;
    i128 cam_y_mm = 0;
    double cam_zoom = 3.0e-6;
    bool paused = false;
    bool show_hud = true;

    MeasurementTools tools = {0};
    measurement_tools_init(&tools, arena);

    while (!WindowShouldClose())
    {
        {   // handle camera 
            const float wheel = GetMouseWheelMove();
            if (wheel != 0.0f)
            {
                cam_zoom *= (1.0 + wheel * 0.15);
                if (cam_zoom < 1.0e-17)
                    cam_zoom = 1.0e-17;
                if (cam_zoom > 1.0e4)
                    cam_zoom = 1.0e4;
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
            {
                Vector2 delta = GetMouseDelta();
                long double dx_mm = (long double)delta.x / (long double)cam_zoom;
                long double dy_mm = (long double)delta.y / (long double)cam_zoom;
                cam_x_mm -= (dx_mm >= 0.0L) ? (i128)(dx_mm + 0.5L) : (i128)(dx_mm - 0.5L);
                cam_y_mm -= (dy_mm >= 0.0L) ? (i128)(dy_mm + 0.5L) : (i128)(dy_mm - 0.5L);
            }
        }

        {   // measurement tools
            measurement_tools_handle_waypoint_inputs(&tools, arena,
                                                     cam_x_mm, cam_y_mm, cam_zoom,
                                                     screen_width, screen_height);

            measurement_tools_handle_runtime_controls(&tools, &paused, min_time_scale, max_time_scale);
            measurement_tools_step_simulation(&tools, &sim, paused, IsKeyPressed(KEY_N),
                                              options.max_physics_steps_per_render);
        }

        if (IsKeyPressed(KEY_F1))
        {
            show_hud = !show_hud;
        }

        {   // drawing HUD + sim
            BeginDrawing();
            ClearBackground((Color){10, 12, 20, 255});

            sim_draw(&sim, cam_x_mm, cam_y_mm, cam_zoom, screen_width, screen_height);

            measurement_tools_draw_overlays(&tools, cam_x_mm, cam_y_mm, cam_zoom, screen_width, screen_height);

            if (show_hud)
            {
                hud_draw(&sim, &tools, options.max_physics_steps_per_render, screen_width, screen_height);
            }

            EndDrawing();
        }
    }

    CloseWindow();
    free_arena(arena);
    return 0;
}
