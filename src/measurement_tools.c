#include "measurement_tools.h"

#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static inline long double i128_to_ld(i128 value)
{
    return (long double)value;
}

static inline i128 ld_to_i128(long double value)
{
    if (value >= 0.0L)
    {
        return (i128)(value + 0.5L);
    }
    return (i128)(value - 0.5L);
}

static void timer_reset(Timer *timer)
{
    timer->elapsed_seconds = 0.0;
    timer->running = false;
}

static void timer_toggle(Timer *timer)
{
    timer->running = !timer->running;
}

static void timer_update(Timer *timer, double dt)
{
    if (timer->running)
    {
        timer->elapsed_seconds += dt;
    }
}

static int waypoint_array_find_near(const Array_Waypoint *list, double x, double y,
                                    i128 cam_x_mm, i128 cam_y_mm, double cam_zoom,
                                    int screen_width, int screen_height, double snap_radius_px)
{
    for (size_t i = 0; i < list->length; i++)
    {
        double screen_x = (double)(i128_to_ld(list->data[i].x_mm - cam_x_mm) * (long double)cam_zoom +
                                   (long double)screen_width / 2.0L);
        double screen_y = (double)(i128_to_ld(list->data[i].y_mm - cam_y_mm) * (long double)cam_zoom +
                                   (long double)screen_height / 2.0L);

        double dx = screen_x - x;
        double dy = screen_y - y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist <= snap_radius_px)
        {
            return (int)i;
        }
    }
    return -1;
}

static void waypoint_array_remove(Array_Waypoint *list, size_t index)
{
    array_remove_at(list, index);
}

static void waypoint_line_array_add(Array_WaypointLine *list, Arena *arena, size_t from_idx, size_t to_idx)
{
    for (size_t i = 0; i < list->length; i++)
    {
        if ((list->data[i].from_idx == from_idx && list->data[i].to_idx == to_idx) ||
            (list->data[i].from_idx == to_idx && list->data[i].to_idx == from_idx))
        {
            return;
        }
    }

    WaypointLine line = {from_idx, to_idx};
    array_push(list, line, arena);
}

static double point_to_segment_distance(double px, double py, double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len_sq = dx * dx + dy * dy;

    if (len_sq < 1e-10)
    {
        double dpx = px - x1;
        double dpy = py - y1;
        return sqrt(dpx * dpx + dpy * dpy);
    }

    double t = ((px - x1) * dx + (py - y1) * dy) / len_sq;
    t = fmax(0.0, fmin(1.0, t));

    double closest_x = x1 + t * dx;
    double closest_y = y1 + t * dy;

    double dist_x = px - closest_x;
    double dist_y = py - closest_y;

    return sqrt(dist_x * dist_x + dist_y * dist_y);
}

static int waypoint_line_array_find_near(const Array_WaypointLine *lines, const Array_Waypoint *waypoints,
                                         double mouse_x, double mouse_y, i128 cam_x_mm, i128 cam_y_mm,
                                         double cam_zoom, int screen_width, int screen_height,
                                         double snap_radius_px)
{
    for (size_t i = 0; i < lines->length; i++)
    {
        size_t from_idx = lines->data[i].from_idx;
        size_t to_idx = lines->data[i].to_idx;

        if (from_idx >= waypoints->length || to_idx >= waypoints->length)
            continue;

        Waypoint from = waypoints->data[from_idx];
        Waypoint to = waypoints->data[to_idx];

        double screen_x1 = (double)(i128_to_ld(from.x_mm - cam_x_mm) * (long double)cam_zoom +
                                    (long double)screen_width / 2.0L);
        double screen_y1 = (double)(i128_to_ld(from.y_mm - cam_y_mm) * (long double)cam_zoom +
                                    (long double)screen_height / 2.0L);
        double screen_x2 = (double)(i128_to_ld(to.x_mm - cam_x_mm) * (long double)cam_zoom +
                                    (long double)screen_width / 2.0L);
        double screen_y2 = (double)(i128_to_ld(to.y_mm - cam_y_mm) * (long double)cam_zoom +
                                    (long double)screen_height / 2.0L);

        double dist = point_to_segment_distance(mouse_x, mouse_y, screen_x1, screen_y1, screen_x2, screen_y2);

        if (dist <= snap_radius_px)
        {
            return (int)i;
        }
    }
    return -1;
}

static void waypoint_line_array_remove(Array_WaypointLine *list, size_t index)
{
    array_remove_at(list, index);
}

static void waypoint_line_array_remove_connected(Array_WaypointLine *list, size_t waypoint_idx)
{
    for (size_t i = 0; i < list->length;)
    {
        if (list->data[i].from_idx == waypoint_idx || list->data[i].to_idx == waypoint_idx)
        {
            waypoint_line_array_remove(list, i);
        }
        else
        {
            i++;
        }
    }

    for (size_t i = 0; i < list->length; i++)
    {
        if (list->data[i].from_idx > waypoint_idx)
        {
            list->data[i].from_idx--;
        }
        if (list->data[i].to_idx > waypoint_idx)
        {
            list->data[i].to_idx--;
        }
    }
}

void measurement_tools_init(MeasurementTools *tools, Arena *arena)
{
    tools->timer.elapsed_seconds = 0.0;
    tools->timer.running = false;
    array_init(&tools->waypoints, 16, arena);
    array_init(&tools->waypoint_lines, 16, arena);
    tools->selected_waypoint = -1;
    tools->time_scale = 3600.0;
    tools->physics_steps_this_frame = 0;
    tools->fps = 0;
}

void measurement_tools_handle_runtime_controls(MeasurementTools *tools, bool *paused,
                                               double min_time_scale, double max_time_scale)
{
    if (IsKeyPressed(KEY_SPACE))
    {
        *paused = !*paused;
    }

    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD))
    {
        tools->time_scale *= 2.0;
        if (tools->time_scale > max_time_scale)
        {
            tools->time_scale = max_time_scale;
        }
    }

    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT))
    {
        tools->time_scale *= 0.5;
        if (tools->time_scale < min_time_scale)
        {
            tools->time_scale = min_time_scale;
        }
    }

    if (IsKeyPressed(KEY_T))
    {
        timer_toggle(&tools->timer);
    }
    if (IsKeyPressed(KEY_R))
    {
        timer_reset(&tools->timer);
    }
}

void measurement_tools_handle_waypoint_inputs(MeasurementTools *tools, Arena *arena,
                                              i128 cam_x_mm, i128 cam_y_mm, double cam_zoom,
                                              int screen_width, int screen_height)
{
    if (IsKeyPressed(KEY_W))
    {
        Vector2 mouse_pos = GetMousePosition();

        i128 world_x_mm = cam_x_mm + ld_to_i128(((long double)mouse_pos.x - (long double)screen_width / 2.0L) /
                                                (long double)cam_zoom);
        i128 world_y_mm = cam_y_mm + ld_to_i128(((long double)mouse_pos.y - (long double)screen_height / 2.0L) /
                                                (long double)cam_zoom);

        Waypoint wp = {world_x_mm, world_y_mm};
        array_push(&tools->waypoints, wp, arena);
    }

    if (IsKeyPressed(KEY_E))
    {
        Vector2 mouse_pos = GetMousePosition();
        int near_idx = waypoint_array_find_near(&tools->waypoints, mouse_pos.x, mouse_pos.y,
                                                cam_x_mm, cam_y_mm, cam_zoom,
                                                screen_width, screen_height, 10.0);
        if (near_idx >= 0)
        {
            waypoint_line_array_remove_connected(&tools->waypoint_lines, (size_t)near_idx);
            waypoint_array_remove(&tools->waypoints, (size_t)near_idx);
            tools->selected_waypoint = -1;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        Vector2 mouse_pos = GetMousePosition();
        int near_idx = waypoint_array_find_near(&tools->waypoints, mouse_pos.x, mouse_pos.y,
                                                cam_x_mm, cam_y_mm, cam_zoom,
                                                screen_width, screen_height, 10.0);

        if (near_idx >= 0)
        {
            if (tools->selected_waypoint == -1)
            {
                tools->selected_waypoint = near_idx;
            }
            else if (tools->selected_waypoint != near_idx)
            {
                waypoint_line_array_add(&tools->waypoint_lines, arena,
                                        (size_t)tools->selected_waypoint, (size_t)near_idx);
                tools->selected_waypoint = -1;
            }
            else
            {
                tools->selected_waypoint = -1;
            }
        }
        else
        {
            tools->selected_waypoint = -1;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
    {
        Vector2 mouse_pos = GetMousePosition();
        int near_line = waypoint_line_array_find_near(&tools->waypoint_lines, &tools->waypoints,
                                                      mouse_pos.x, mouse_pos.y,
                                                      cam_x_mm, cam_y_mm, cam_zoom,
                                                      screen_width, screen_height, 10.0);
        if (near_line >= 0)
        {
            waypoint_line_array_remove(&tools->waypoint_lines, (size_t)near_line);
        }
    }
}

void measurement_tools_step_simulation(MeasurementTools *tools, SimContext *sim,
                                       bool paused, bool step_once,
                                       int max_physics_steps_per_render)
{
    double sim_dt = 0.0;
    tools->physics_steps_this_frame = 0;

    if (!paused || step_once)
    {
        const double frame_dt = paused ? (1.0 / 60.0) : GetFrameTime();

        int physics_steps = (int)ceil(tools->time_scale);
        if (physics_steps < 1)
        {
            physics_steps = 1;
        }
        if (physics_steps > max_physics_steps_per_render)
        {
            physics_steps = max_physics_steps_per_render;
        }

        const double substep_dt = frame_dt * (tools->time_scale / (double)physics_steps);
        for (int i = 0; i < physics_steps; i++)
        {
            sim_step(sim, substep_dt);
        }

        sim_dt = substep_dt * (double)physics_steps;
        tools->physics_steps_this_frame = physics_steps;
    }

    timer_update(&tools->timer, sim_dt);
    tools->fps = GetFPS();
}

void measurement_tools_draw_overlays(const MeasurementTools *tools,
                                     i128 cam_x_mm, i128 cam_y_mm, double cam_zoom,
                                     int screen_width, int screen_height)
{
    for (size_t i = 0; i < tools->waypoint_lines.length; i++)
    {
        size_t from_idx = tools->waypoint_lines.data[i].from_idx;
        size_t to_idx = tools->waypoint_lines.data[i].to_idx;

        if (from_idx >= tools->waypoints.length || to_idx >= tools->waypoints.length)
            continue;

        Waypoint from = tools->waypoints.data[from_idx];
        Waypoint to = tools->waypoints.data[to_idx];

        double screen_x1 = (double)(i128_to_ld(from.x_mm - cam_x_mm) * (long double)cam_zoom +
                                    (long double)screen_width / 2.0L);
        double screen_y1 = (double)(i128_to_ld(from.y_mm - cam_y_mm) * (long double)cam_zoom +
                                    (long double)screen_height / 2.0L);
        double screen_x2 = (double)(i128_to_ld(to.x_mm - cam_x_mm) * (long double)cam_zoom +
                                    (long double)screen_width / 2.0L);
        double screen_y2 = (double)(i128_to_ld(to.y_mm - cam_y_mm) * (long double)cam_zoom +
                                    (long double)screen_height / 2.0L);

        DrawLineEx((Vector2){(float)screen_x1, (float)screen_y1},
                   (Vector2){(float)screen_x2, (float)screen_y2},
                   2.0f, (Color){100, 200, 255, 255});

        i128 dx_mm = to.x_mm - from.x_mm;
        i128 dy_mm = to.y_mm - from.y_mm;
        long double distance_mm = sqrtl(i128_to_ld(dx_mm) * i128_to_ld(dx_mm) + i128_to_ld(dy_mm) * i128_to_ld(dy_mm));
        double distance_m = (double)(distance_mm / 1000.0L);

        double mid_x = (screen_x1 + screen_x2) / 2.0;
        double mid_y = (screen_y1 + screen_y2) / 2.0;

        char distance_text[64];
        if (distance_m > 1e9)
        {
            snprintf(distance_text, sizeof(distance_text), "%.2e m", distance_m);
        }
        else if (distance_m > 1e6)
        {
            snprintf(distance_text, sizeof(distance_text), "%.2f Mm", distance_m / 1e6);
        }
        else if (distance_m > 1e3)
        {
            snprintf(distance_text, sizeof(distance_text), "%.2f km", distance_m / 1e3);
        }
        else
        {
            snprintf(distance_text, sizeof(distance_text), "%.2f m", distance_m);
        }

        int text_width = MeasureText(distance_text, 14);

        DrawRectangle((int)mid_x - text_width / 2 - 4, (int)mid_y - 18,
                      text_width + 8, 20,
                      (Color){15, 18, 30, 200});

        DrawText(distance_text, (int)mid_x - text_width / 2, (int)mid_y - 15,
                 14, (Color){150, 220, 255, 255});
    }

    for (size_t i = 0; i < tools->waypoints.length; i++)
    {
        double screen_x = (double)(i128_to_ld(tools->waypoints.data[i].x_mm - cam_x_mm) * (long double)cam_zoom +
                                   (long double)screen_width / 2.0L);
        double screen_y = (double)(i128_to_ld(tools->waypoints.data[i].y_mm - cam_y_mm) * (long double)cam_zoom +
                                   (long double)screen_height / 2.0L);

        DrawCircle((int)screen_x, (int)screen_y, 6, (Color){255, 200, 50, 255});
        DrawCircleLines((int)screen_x, (int)screen_y, 6, (Color){255, 255, 100, 255});

        DrawLine((int)screen_x - 3, (int)screen_y, (int)screen_x + 3, (int)screen_y, BLACK);
        DrawLine((int)screen_x, (int)screen_y - 3, (int)screen_x, (int)screen_y + 3, BLACK);
    }

    if (tools->selected_waypoint >= 0 && tools->selected_waypoint < (int)tools->waypoints.length)
    {
        Waypoint wp = tools->waypoints.data[tools->selected_waypoint];
        double screen_x = (double)(i128_to_ld(wp.x_mm - cam_x_mm) * (long double)cam_zoom +
                                   (long double)screen_width / 2.0L);
        double screen_y = (double)(i128_to_ld(wp.y_mm - cam_y_mm) * (long double)cam_zoom +
                                   (long double)screen_height / 2.0L);
        DrawCircleLines((int)screen_x, (int)screen_y, 10, (Color){0, 255, 0, 255});
    }
}
