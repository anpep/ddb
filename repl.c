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

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "debug.h"

void repl_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    rl_bind_key('\t', rl_complete);
}

void repl_loop(struct debug_ctx *ctx)
{
    int res = 0;

    while (1) {
        debug_loop(ctx);
        char *input = readline(res == -1 ? "\x1b[31m(ddb)\x1b[0m "
                                         : "\x1b[35m(ddb)\x1b[0m ");
        if (!input)  /* EOF */
            break;
        add_history(input);
        res = debug_eval(ctx, (const char *) input);
        free(input);
    }
}