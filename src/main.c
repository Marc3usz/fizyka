#include "raylib.h"
#include "sim.h"

#include <stdio.h>
#include <math.h>

typedef struct {
    double elapsed_seconds;
    bool running;
} Timer;

typedef struct {
    double x;
    double y;
} Waypoint;

typedef struct {
    size_t from_idx;
    size_t to_idx;
} WaypointLine;

DEFINE_ARRAY(Waypoint);
DEFINE_ARRAY(WaypointLine);

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

void waypoint_array_init(Array_Waypoint* list, Arena* arena, size_t initial_capacity) {
    array_init(list, initial_capacity, arena);
}

void waypoint_array_add(Array_Waypoint* list, Arena* arena, double x, double y) {
    Waypoint wp = {x, y};
    array_push(list, wp, arena);
}

int waypoint_array_find_near(const Array_Waypoint* list, double x, double y, double cam_x, double cam_y,
                             double cam_zoom, int screen_width, int screen_height, double snap_radius_px) {
    for (size_t i = 0; i < list->length; i++) {
        double screen_x = (list->data[i].x - cam_x) * cam_zoom + screen_width / 2.0;
        double screen_y = (list->data[i].y - cam_y) * cam_zoom + screen_height / 2.0;

        double dx = screen_x - x;
        double dy = screen_y - y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist <= snap_radius_px) {
            return (int)i;
        }
    }
    return -1;
}

void waypoint_array_remove(Array_Waypoint* list, size_t index) {
    if (index >= list->length) return;
    if (index + 1 < list->length) {
        memmove(&list->data[index], &list->data[index + 1],
                (list->length - index - 1) * sizeof(Waypoint));
    }
    list->length--;
}

void waypoint_array_draw(const Array_Waypoint* list, double cam_x, double cam_y, double cam_zoom,
                         int screen_width, int screen_height) {
    for (size_t i = 0; i < list->length; i++) {
        double screen_x = (list->data[i].x - cam_x) * cam_zoom + screen_width / 2.0;
        double screen_y = (list->data[i].y - cam_y) * cam_zoom + screen_height / 2.0;

        DrawCircle((int)screen_x, (int)screen_y, 6, (Color){255, 200, 50, 255});
        DrawCircleLines((int)screen_x, (int)screen_y, 6, (Color){255, 255, 100, 255});

        DrawLine((int)screen_x - 3, (int)screen_y, (int)screen_x + 3, (int)screen_y, BLACK);
        DrawLine((int)screen_x, (int)screen_y - 3, (int)screen_x, (int)screen_y + 3, BLACK);
    }
}

void waypoint_line_array_init(Array_WaypointLine* list, Arena* arena, size_t initial_capacity) {
    array_init(list, initial_capacity, arena);
}

void waypoint_line_array_add(Array_WaypointLine* list, Arena* arena, size_t from_idx, size_t to_idx) {
    for (size_t i = 0; i < list->length; i++) {
        if ((list->data[i].from_idx == from_idx && list->data[i].to_idx == to_idx) ||
            (list->data[i].from_idx == to_idx && list->data[i].to_idx == from_idx)) {
            return;
        }
    }

    WaypointLine line = {from_idx, to_idx};
    array_push(list, line, arena);
}

double point_to_segment_distance(double px, double py, double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    double len_sq = dx * dx + dy * dy;
    
    if (len_sq < 1e-10) {
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

int waypoint_line_array_find_near(const Array_WaypointLine* lines, const Array_Waypoint* waypoints,
                                  double mouse_x, double mouse_y, double cam_x, double cam_y,
                                  double cam_zoom, int screen_width, int screen_height,
                                  double snap_radius_px) {
    for (size_t i = 0; i < lines->length; i++) {
        size_t from_idx = lines->data[i].from_idx;
        size_t to_idx = lines->data[i].to_idx;
        
        if (from_idx >= waypoints->length || to_idx >= waypoints->length) continue;
        
        Waypoint from = waypoints->data[from_idx];
        Waypoint to = waypoints->data[to_idx];
        
        double screen_x1 = (from.x - cam_x) * cam_zoom + screen_width / 2.0;
        double screen_y1 = (from.y - cam_y) * cam_zoom + screen_height / 2.0;
        double screen_x2 = (to.x - cam_x) * cam_zoom + screen_width / 2.0;
        double screen_y2 = (to.y - cam_y) * cam_zoom + screen_height / 2.0;
        
        double dist = point_to_segment_distance(mouse_x, mouse_y, screen_x1, screen_y1, screen_x2, screen_y2);
        
        if (dist <= snap_radius_px) {
            return (int)i;
        }
    }
    return -1;
}

void waypoint_line_array_remove(Array_WaypointLine* list, size_t index) {
    if (index >= list->length) return;
    if (index + 1 < list->length) {
        memmove(&list->data[index], &list->data[index + 1],
                (list->length - index - 1) * sizeof(WaypointLine));
    }
    list->length--;
}

void waypoint_line_array_remove_connected(Array_WaypointLine* list, size_t waypoint_idx) {
    for (size_t i = 0; i < list->length; ) {
        if (list->data[i].from_idx == waypoint_idx || list->data[i].to_idx == waypoint_idx) {
            waypoint_line_array_remove(list, i);
        } else {
            i++;
        }
    }
    
    for (size_t i = 0; i < list->length; i++) {
        if (list->data[i].from_idx > waypoint_idx) {
            list->data[i].from_idx--;
        }
        if (list->data[i].to_idx > waypoint_idx) {
            list->data[i].to_idx--;
        }
    }
}

void waypoint_line_array_draw(const Array_WaypointLine* lines, const Array_Waypoint* waypoints,
                              double cam_x, double cam_y, double cam_zoom,
                              int screen_width, int screen_height) {
    for (size_t i = 0; i < lines->length; i++) {
        size_t from_idx = lines->data[i].from_idx;
        size_t to_idx = lines->data[i].to_idx;
        
        if (from_idx >= waypoints->length || to_idx >= waypoints->length) continue;
        
        Waypoint from = waypoints->data[from_idx];
        Waypoint to = waypoints->data[to_idx];
        
        double screen_x1 = (from.x - cam_x) * cam_zoom + screen_width / 2.0;
        double screen_y1 = (from.y - cam_y) * cam_zoom + screen_height / 2.0;
        double screen_x2 = (to.x - cam_x) * cam_zoom + screen_width / 2.0;
        double screen_y2 = (to.y - cam_y) * cam_zoom + screen_height / 2.0;
        
        DrawLineEx((Vector2){(float)screen_x1, (float)screen_y1},
                   (Vector2){(float)screen_x2, (float)screen_y2},
                   2.0f, (Color){100, 200, 255, 255});
        
        double dx = to.x - from.x;
        double dy = to.y - from.y;
        double distance = sqrt(dx * dx + dy * dy);
        
        double mid_x = (screen_x1 + screen_x2) / 2.0;
        double mid_y = (screen_y1 + screen_y2) / 2.0;
        
        char distance_text[64];
        if (distance > 1e9) {
            snprintf(distance_text, sizeof(distance_text), "%.2e m", distance);
        } else if (distance > 1e6) {
            snprintf(distance_text, sizeof(distance_text), "%.2f Mm", distance / 1e6);
        } else if (distance > 1e3) {
            snprintf(distance_text, sizeof(distance_text), "%.2f km", distance / 1e3);
        } else {
            snprintf(distance_text, sizeof(distance_text), "%.2f m", distance);
        }
        
        int text_width = MeasureText(distance_text, 14);
        
        DrawRectangle((int)mid_x - text_width / 2 - 4, (int)mid_y - 18,
                      text_width + 8, 20,
                      (Color){15, 18, 30, 200});
        
        DrawText(distance_text, (int)mid_x - text_width / 2, (int)mid_y - 15,
                 14, (Color){150, 220, 255, 255});
    }
}

int main(void) {
    const int screen_width = 1200;
    const int screen_height = 800;
    double time_scale = 3600.0;
    const double min_time_scale = 1.0;
    const double max_time_scale = 86400.0 * 365.0;

    InitWindow(screen_width, screen_height, "Fizyka - Gravity Sim");
    SetTargetFPS(60);

    Arena* arena = init_arena(10 * 1024 * 1024);
    SimContext sim = {0};
    sim_init(&sim, arena);

    double cam_x = 0.0;
    double cam_y = 0.0;
    double cam_zoom = 3.0e-9;

    bool paused = false;
    
    Timer timer = {0};
    Array_Waypoint waypoints;
    waypoint_array_init(&waypoints, arena, 16);
    
    Array_WaypointLine waypoint_lines;
    waypoint_line_array_init(&waypoint_lines, arena, 16);
    
    int selected_waypoint = -1;

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

        if (IsKeyPressed(KEY_W)) {
            Vector2 mouse_pos = GetMousePosition();
            
            double world_x = (mouse_pos.x - screen_width / 2.0) / cam_zoom + cam_x;
            double world_y = (mouse_pos.y - screen_height / 2.0) / cam_zoom + cam_y;
            waypoint_array_add(&waypoints, arena, world_x, world_y);
        }

        if (IsKeyPressed(KEY_E)) {
            Vector2 mouse_pos = GetMousePosition();
            int near_idx = waypoint_array_find_near(&waypoints, mouse_pos.x, mouse_pos.y,
                                                   cam_x, cam_y, cam_zoom, 
                                                   screen_width, screen_height, 10.0);
            if (near_idx >= 0) {
                waypoint_line_array_remove_connected(&waypoint_lines, near_idx);
                waypoint_array_remove(&waypoints, near_idx);
                selected_waypoint = -1;
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse_pos = GetMousePosition();
            int near_idx = waypoint_array_find_near(&waypoints, mouse_pos.x, mouse_pos.y,
                                                   cam_x, cam_y, cam_zoom,
                                                   screen_width, screen_height, 10.0);
            
            if (near_idx >= 0) {
                if (selected_waypoint == -1) {
                    selected_waypoint = near_idx;
                } else if (selected_waypoint != near_idx) {
                    waypoint_line_array_add(&waypoint_lines, arena, selected_waypoint, near_idx);
                    selected_waypoint = -1;
                } else {
                    selected_waypoint = -1;
                }
            } else {
                selected_waypoint = -1;
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouse_pos = GetMousePosition();
            int near_line = waypoint_line_array_find_near(&waypoint_lines, &waypoints,
                                                         mouse_pos.x, mouse_pos.y,
                                                         cam_x, cam_y, cam_zoom,
                                                         screen_width, screen_height, 10.0);
            if (near_line >= 0) {
                waypoint_line_array_remove(&waypoint_lines, near_line);
            }
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
        
        waypoint_line_array_draw(&waypoint_lines, &waypoints, cam_x, cam_y, cam_zoom, screen_width, screen_height);
        waypoint_array_draw(&waypoints, cam_x, cam_y, cam_zoom, screen_width, screen_height);
        
        if (selected_waypoint >= 0 && selected_waypoint < (int)waypoints.length) {
            Waypoint wp = waypoints.data[selected_waypoint];
            double screen_x = (wp.x - cam_x) * cam_zoom + screen_width / 2.0;
            double screen_y = (wp.y - cam_y) * cam_zoom + screen_height / 2.0;
            DrawCircleLines((int)screen_x, (int)screen_y, 10, (Color){0, 255, 0, 255});
        }

        int panel_x = 12;
        int panel_y = 12;
        int panel_width = 380;
        int panel_height = 220;
        
        DrawRectangle(panel_x, panel_y, panel_width, panel_height, (Color){15, 18, 30, 230});
        DrawRectangleLines(panel_x, panel_y, panel_width, panel_height, (Color){90, 100, 120, 255});

        int text_x = panel_x + 14;
        int text_y = panel_y + 12;
        int line_height = 20;
        
        DrawText(TextFormat("Bodies: %zu", sim.bodies.length), text_x, text_y, 16, RAYWHITE);
        text_y += line_height;
        
        DrawText(TextFormat("Time: %.2f days", sim.time_seconds / 86400.0), text_x, text_y, 16, RAYWHITE);
        text_y += line_height;
        
        DrawText(TextFormat("Speed: %.0fx", time_scale), text_x, text_y, 16, RAYWHITE);
        text_y += line_height;
        
        const char *timer_status = timer.running ? "RUNNING" : "PAUSED";
        Color timer_color = timer.running ? GREEN : ORANGE;
        DrawText(TextFormat("Timer: %.2f days", timer.elapsed_seconds / 86400.0), text_x, text_y, 16, RAYWHITE);
        DrawText(TextFormat("[%s]", timer_status), text_x + 180, text_y, 16, timer_color);
        text_y += line_height;
        
        DrawText(TextFormat("Waypoints: %zu  Lines: %zu", waypoints.length, waypoint_lines.length), text_x, text_y, 16, RAYWHITE);
        text_y += line_height + 8;

        DrawLineEx((Vector2){panel_x + 10, text_y}, 
                   (Vector2){panel_x + panel_width - 10, text_y}, 
                   1.0f, (Color){60, 70, 90, 255});
        text_y += 10;

        DrawText("SPACE: pause  N: step  +/-: speed", text_x, text_y, 13, LIGHTGRAY);
        text_y += 16;
        
        DrawText("Mouse wheel: zoom  Middle drag: pan", text_x, text_y, 13, LIGHTGRAY);
        text_y += 16;
        
        DrawText("T: toggle timer  R: reset timer", text_x, text_y, 13, LIGHTGRAY);
        text_y += 16;
        
        DrawText("W: place waypoint  E: remove waypoint", text_x, text_y, 13, LIGHTGRAY);
        text_y += 16;
        
        DrawText("Left click: draw line  Right click: delete line", text_x, text_y, 13, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    free_arena(arena);
    return 0;
}