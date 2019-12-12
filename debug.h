/*
 * ddb(1) -- A Dumb Debugger
 * Copyright (c) 2019 Angel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <sys/types.h>

#include "list.h"

/* holds the debugger context for a specific process */
struct debug_ctx {
    /* argv as a list */
    struct list argvl;
    /* process ID */
    pid_t pid;
    /* image path */
    char *ipath;

    /* debug status */
    enum { PAUSED, RUNNING } status;
};

/* holds metadata about a debugger command */
struct debug_cmd {
    /* one-letter command name */
    char name;
    /* help text shown in command list */
    char help[128];
    /* command function pointer */
    int (*fn)(struct debug_ctx *, const struct list *);
    /* user-specific value */
    int user;
};

/* evaluates a debugger expression */
int debug_eval(struct debug_ctx *ctx, const char *expr);

/* enables inferior process debugging */
int debug_loop(struct debug_ctx *ctx);

/* forks the current process and breaks before execution */
int debug_cmd_exec(struct debug_ctx *ctx, const struct list *args);

/* attach to a running process */
int debug_cmd_attach(struct debug_ctx *ctx, const struct list *args);

/* continues execution on the inferior process */
int debug_cmd_continue(struct debug_ctx *ctx, const struct list *args);

/* detaches from the current inferior process */
int debug_cmd_detach(struct debug_ctx *ctx, const struct list *args);

/* quits the debugging session */
int debug_cmd_quit(struct debug_ctx *ctx, const struct list *args);

/* displays process info */
int debug_cmd_info(struct debug_ctx *ctx, const struct list *args);

/* displays a list of the available commands */
int debug_cmd_help(struct debug_ctx *ctx, const struct list *args);