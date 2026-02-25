#ifndef HUD_H
#define HUD_H

#include "measurement_tools.h"
#include "sim.h"

void hud_draw(const SimContext* sim, const MeasurementTools* tools,
              int max_physics_steps_per_render, int screen_w, int screen_h);

#endif
