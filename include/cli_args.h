#ifndef CLI_ARGS_H
#define CLI_ARGS_H

#include <stdbool.h>
#include "sim.h"

typedef struct {
    int max_physics_steps_per_render;
    SimIntegrator integrator;
} SimCliOptions;

bool parse_sim_cli_args(int argc, char** argv, SimCliOptions* out_options, const char** out_error);
const char* sim_cli_usage(const char* program_name);

#endif
