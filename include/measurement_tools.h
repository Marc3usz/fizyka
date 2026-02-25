#ifndef MEASUREMENT_TOOLS_H
#define MEASUREMENT_TOOLS_H

#include "arena.h"
#include "dynamic_array.h"
#include "sim.h"
#include <stdbool.h>

typedef struct {
    double elapsed_seconds;
    bool running;
} Timer;

typedef struct {
    i128 x_mm;
    i128 y_mm;
} Waypoint;

typedef struct {
    size_t from_idx;
    size_t to_idx;
} WaypointLine;

DEFINE_ARRAY(Waypoint);
DEFINE_ARRAY(WaypointLine);

typedef struct {
    Timer timer;
    Array_Waypoint waypoints;
    Array_WaypointLine waypoint_lines;
    int selected_waypoint;
    double time_scale;
    int physics_steps_this_frame;
    int fps;
} MeasurementTools;

void measurement_tools_init(MeasurementTools* tools, Arena* arena);
void measurement_tools_handle_runtime_controls(MeasurementTools* tools, bool* paused,
                                               double min_time_scale, double max_time_scale);
void measurement_tools_handle_waypoint_inputs(MeasurementTools* tools, Arena* arena,
                                              i128 cam_x_mm, i128 cam_y_mm, double cam_zoom,
                                              int screen_width, int screen_height);
void measurement_tools_step_simulation(MeasurementTools* tools, SimContext* sim,
                                       bool paused, bool step_once,
                                       int max_physics_steps_per_render);
void measurement_tools_draw_overlays(const MeasurementTools* tools,
                                     i128 cam_x_mm, i128 cam_y_mm, double cam_zoom,
                                     int screen_width, int screen_height);

#endif
