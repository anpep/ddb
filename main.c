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
#include <string.h>
#include <limits.h>

#include "debug.h"
#include "repl.h"
#include "list.h"

static void show_usage(void)
{
    printf("usage: ddb PROGRAM [ARGS...]\n"
           "       ddb -a PID\n");
}

int main(int argc, char *argv[])
{
    struct debug_ctx ctx = {0};

    repl_init();
    repl_loop(&ctx);

#if 0
    if (!strcmp("-a", argv[1])) {
        /* attach to process */
        if (argc != 3) {
            show_usage();
            return 0;
        }
        ctx.pid = strtoul(argv[2], NULL, 10);
        if (!ctx.pid || ctx.pid == ULONG_MAX) {
            printf(ERROR "invalid process ID\n");
            exit(1);
        }
    } else {
        /* execute process */
        ctx.argc = argc - 1;
        ctx.argv = argv + 1;
    }
#endif

    return 0;
}