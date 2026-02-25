#include "cli_args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* USAGE_TEMPLATE = "Usage: %s [--max-phys-steps-per-render N] [--integrator verlet|yoshida4]";

typedef struct {
    int argc;
    char** argv;
    int index;
} CliCursor;

typedef bool (*ParsePositionalFn)(const char* token, SimCliOptions* out_options, const char** out_error);
typedef bool (*ParseFlagFn)(char flag, SimCliOptions* out_options, const char** out_error);
typedef bool (*ParseNamedFn)(const char* name, const char* value, SimCliOptions* out_options, const char** out_error);

typedef enum {
    CLI_TOKEN_NONE,
    CLI_TOKEN_POSITIONAL,
    CLI_TOKEN_FLAG,
    CLI_TOKEN_NAMED,
} CliTokenKind;

static CliTokenKind classify_token(const char* token) {
    if (!token || token[0] == '\0') {
        return CLI_TOKEN_NONE;
    }
    if (token[0] != '-') {
        return CLI_TOKEN_POSITIONAL;
    }
    if (token[1] == '-') {
        return token[2] == '\0' ? CLI_TOKEN_FLAG : CLI_TOKEN_NAMED;
    }
    return token[1] == '\0' ? CLI_TOKEN_NONE : CLI_TOKEN_FLAG;
}

static bool parse_positional(CliCursor* cursor, SimCliOptions* out_options, const char** out_error, ParsePositionalFn parse_fn) {
    while (cursor->index < cursor->argc) {
        const char* token = cursor->argv[cursor->index];
        if (!token) {
            cursor->index++;
            continue;
        }

        if (token[0] == '-') {
            break;
        }

        if (parse_fn && !parse_fn(token, out_options, out_error)) {
            return false;
        }

        cursor->index++;
    }
    return true;
}

static bool parse_flag(CliCursor* cursor, SimCliOptions* out_options, const char** out_error, ParseFlagFn parse_fn) {
    const char* token = cursor->argv[cursor->index];
    if (!token || token[0] != '-' || token[1] == '-' || token[1] == '\0') {
        if (out_error) {
            *out_error = "invalid short flag";
        }
        return false;
    }

    for (int i = 1; token[i] != '\0'; i++) {
        if (!parse_fn || !parse_fn(token[i], out_options, out_error)) {
            return false;
        }
    }

    cursor->index++;
    return true;
}

static bool parse_named(CliCursor* cursor, SimCliOptions* out_options, const char** out_error, ParseNamedFn parse_fn) {
    const char* token = cursor->argv[cursor->index];
    if (!token || strncmp(token, "--", 2) != 0 || token[2] == '\0') {
        if (out_error) {
            *out_error = "invalid named argument";
        }
        return false;
    }

    const char* name = token + 2;
    if (cursor->index + 1 >= cursor->argc) {
        if (out_error) {
            *out_error = "missing value after named argument";
        }
        return false;
    }

    const char* value = cursor->argv[cursor->index + 1];
    if (!parse_fn || !parse_fn(name, value, out_options, out_error)) {
        return false;
    }

    cursor->index += 2;
    return true;
}

static bool parse_next_token(CliCursor* cursor, SimCliOptions* out_options, const char** out_error,
                             ParseFlagFn parse_flag_fn, ParseNamedFn parse_named_fn) {
    const char* token = cursor->argv[cursor->index];
    switch (classify_token(token)) {
        case CLI_TOKEN_NONE:
            cursor->index++;
            return true;
        case CLI_TOKEN_NAMED:
            return parse_named(cursor, out_options, out_error, parse_named_fn);
        case CLI_TOKEN_FLAG:
            return parse_flag(cursor, out_options, out_error, parse_flag_fn);
        case CLI_TOKEN_POSITIONAL:
        default:
            if (out_error) {
                *out_error = "unexpected positional argument after flags";
            }
            return false;
    }
}

static bool parse_positional_token(const char* token, SimCliOptions* out_options, const char** out_error) {
    (void)token;
    (void)out_options;
    if (out_error) {
        *out_error = "positional arguments are not supported";
    }
    return false;
}

static bool parse_flag_token(char flag, SimCliOptions* out_options, const char** out_error) {
    (void)flag;
    (void)out_options;
    if (out_error) {
        *out_error = "short flags are not supported";
    }
    return false;
}

static bool parse_named_token(const char* name, const char* value, SimCliOptions* out_options, const char** out_error) {
    if (strcmp(name, "max-phys-steps-per-render") == 0) {
        char* end = NULL;
        long parsed = strtol(value, &end, 10);
        if (end == value || *end != '\0' || parsed < 1 || parsed > 1000000) {
            if (out_error) {
                *out_error = "invalid --max-phys-steps-per-render value (expected 1..1000000)";
            }
            return false;
        }
        out_options->max_physics_steps_per_render = (int)parsed;
        return true;
    }

    if (strcmp(name, "integrator") == 0) {
        if (strcmp(value, "verlet") == 0) {
            out_options->integrator = SIM_INTEGRATOR_VELOCITY_VERLET;
            return true;
        }
        if (strcmp(value, "yoshida4") == 0) {
            out_options->integrator = SIM_INTEGRATOR_YOSHIDA_RUTH_4;
            return true;
        }
        if (out_error) {
            *out_error = "invalid --integrator value (expected verlet|yoshida4)";
        }
        return false;
    }

    if (out_error) {
        *out_error = "unknown named argument";
    }
    return false;
}

const char* sim_cli_usage(const char* program_name) {
    static char usage[256];
    if (!program_name) {
        program_name = "fizyka";
    }
    snprintf(usage, sizeof(usage), USAGE_TEMPLATE, program_name);
    return usage;
}

bool parse_sim_cli_args(int argc, char** argv, SimCliOptions* out_options, const char** out_error) {
    if (!out_options) {
        if (out_error) {
            *out_error = "internal error: output options pointer is null";
        }
        return false;
    }

    out_options->max_physics_steps_per_render = 1000;
    out_options->integrator = SIM_INTEGRATOR_VELOCITY_VERLET;

    CliCursor cursor = {
        .argc = argc,
        .argv = argv,
        .index = 1,
    };

    if (!parse_positional(&cursor, out_options, out_error, parse_positional_token)) {
        return false;
    }

    while (cursor.index < cursor.argc) {
        if (!parse_next_token(&cursor, out_options, out_error,
                              parse_flag_token, parse_named_token)) {
            return false;
        }
    }

    if (out_error) {
        *out_error = NULL;
    }
    return true;
}
