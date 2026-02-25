#include "hud.h"

#include "raylib.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char text[192];
    int font_size;
    Color color;
    int gap_after;
} HudPanelLine;

typedef struct {
    int x;
    int y;
    int padding;
    Color bg_color;
    Color border_color;
    HudPanelLine lines[24];
    int line_count;
} HudPanel;

static void hud_panel_begin(HudPanel* panel, int x, int y, int padding, Color bg_color, Color border_color) {
    panel->x = x;
    panel->y = y;
    panel->padding = padding;
    panel->bg_color = bg_color;
    panel->border_color = border_color;
    panel->line_count = 0;
}

static void hud_panel_add_line(HudPanel* panel, const char* text, int font_size, Color color, int gap_after) {
    if (panel->line_count >= (int)(sizeof(panel->lines) / sizeof(panel->lines[0])) || !text) {
        return;
    }

    HudPanelLine* line = &panel->lines[panel->line_count++];
    snprintf(line->text, sizeof(line->text), "%s", text);
    line->font_size = font_size;
    line->color = color;
    line->gap_after = gap_after;
}

static void hud_panel_add_linef(HudPanel* panel, int font_size, Color color, int gap_after, const char* fmt, ...) {
    if (panel->line_count >= (int)(sizeof(panel->lines) / sizeof(panel->lines[0])) || !fmt) {
        return;
    }

    HudPanelLine* line = &panel->lines[panel->line_count++];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line->text, sizeof(line->text), fmt, args);
    va_end(args);
    line->font_size = font_size;
    line->color = color;
    line->gap_after = gap_after;
}

static void hud_panel_render(const HudPanel* panel) {
    int content_width = 0;
    int content_height = 0;

    for (int i = 0; i < panel->line_count; i++) {
        const HudPanelLine* line = &panel->lines[i];
        int line_width = MeasureText(line->text, line->font_size);
        if (line_width > content_width) {
            content_width = line_width;
        }
        content_height += line->font_size + line->gap_after;
    }

    if (panel->line_count > 0) {
        content_height -= panel->lines[panel->line_count - 1].gap_after;
    }

    int panel_width = panel->padding * 2 + content_width;
    int panel_height = panel->padding * 2 + content_height;

    DrawRectangle(panel->x, panel->y, panel_width, panel_height, panel->bg_color);
    DrawRectangleLines(panel->x, panel->y, panel_width, panel_height, panel->border_color);

    int text_x = panel->x + panel->padding;
    int text_y = panel->y + panel->padding;
    for (int i = 0; i < panel->line_count; i++) {
        const HudPanelLine* line = &panel->lines[i];
        DrawText(line->text, text_x, text_y, line->font_size, line->color);
        text_y += line->font_size + line->gap_after;
    }
}

void hud_draw(const SimContext* sim, const MeasurementTools* tools,
              int max_physics_steps_per_render, int screen_w, int screen_h) {
    HudPanel sim_panel;
    hud_panel_begin(&sim_panel, 12, 12, 12, (Color){12, 16, 28, 230}, (Color){95, 110, 140, 255});
    hud_panel_add_line(&sim_panel, "Simulation", 18, (Color){190, 220, 255, 255}, 6);
    hud_panel_add_linef(&sim_panel, 15, RAYWHITE, 3, "Bodies: %zu", sim->bodies.length);
    hud_panel_add_linef(&sim_panel, 15, RAYWHITE, 3, "Sim Time: %.3f days", sim->time_seconds / 86400.0);
    hud_panel_add_linef(&sim_panel, 15, RAYWHITE, 3, "Speed: %.0fx", tools->time_scale);
    hud_panel_add_linef(&sim_panel, 15, RAYWHITE, 3, "Physics/Render: %d/%d",
                        tools->physics_steps_this_frame, max_physics_steps_per_render);
    hud_panel_add_linef(&sim_panel, 15, RAYWHITE, 6, "FPS: %d", tools->fps);
    hud_panel_add_linef(&sim_panel, 15, tools->timer.running ? GREEN : ORANGE, 3,
                        "Timer: %.3f days [%s]",
                        tools->timer.elapsed_seconds / 86400.0,
                        tools->timer.running ? "RUNNING" : "PAUSED");
    hud_panel_add_linef(&sim_panel, 15, RAYWHITE, 0,
                        "Waypoints: %zu  Lines: %zu",
                        tools->waypoints.length, tools->waypoint_lines.length);
    hud_panel_render(&sim_panel);

    int controls_y = 12 + 12 + 12;
    for (int i = 0; i < sim_panel.line_count; i++) {
        controls_y += sim_panel.lines[i].font_size + sim_panel.lines[i].gap_after;
    }
    controls_y += 10;

    HudPanel controls_panel;
    hud_panel_begin(&controls_panel, 12, controls_y, 12, (Color){12, 16, 28, 210}, (Color){70, 85, 110, 255});
    hud_panel_add_line(&controls_panel, "Controls", 16, (Color){170, 210, 255, 255}, 6);
    hud_panel_add_line(&controls_panel, "SPACE pause  N single-step  +/- speed", 13, LIGHTGRAY, 3);
    hud_panel_add_line(&controls_panel, "Wheel zoom  Middle-drag pan", 13, LIGHTGRAY, 3);
    hud_panel_add_line(&controls_panel, "W add waypoint  E remove waypoint", 13, LIGHTGRAY, 3);
    hud_panel_add_line(&controls_panel, "LMB connect  RMB remove line  T/R timer", 13, LIGHTGRAY, 0);
    hud_panel_render(&controls_panel);

    const SimIntegrator active_integrator = sim_get_integrator(sim);
    const char* integrator_name = (active_integrator == SIM_INTEGRATOR_VELOCITY_VERLET) ? "verlet" : "yoshida4";
    DrawText(TextFormat("Integrator: %s", integrator_name), 12, screen_h - 24, 16, (Color){180, 220, 255, 255});

    (void)screen_w;
}
