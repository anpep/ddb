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
#include <getopt.h>

#include "debug.h"
#include "repl.h"

static void __attribute__((noreturn)) show_usage(void)
{
    fprintf(stderr, "usage: ddb PROGRAM [ARGS...]\n"
                    "       ddb -a PID\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    struct debug_ctx ctx = {0};
    struct list args = {0};
    enum {
        ATTACH, EXEC, NONE
    } mode = NONE;
    int opt;

    while ((opt = getopt(argc, argv, ":a:"))) {
        if (opt == 'a') {
            if (mode == ATTACH) {
                /* duplicate option */
                fprintf(stderr, "error: multiple PIDs specified\n");
                exit(EXIT_FAILURE);
            }
            mode = ATTACH;
            list_create(&args, 1);
            list_insert(&args, optarg);
        } else if (opt == -1) {
            break;
        } else {
            show_usage();
        }
    }
    if (optind < argc) {
        if (mode == NONE) {
            mode = EXEC;
            list_create(&args, argc - 1);
            if (!strcmp("--", *(argv + 1)))
                argv++;
            while (*++argv)
                list_insert(&args, *argv);
        } else if (mode == ATTACH) {
            fprintf(stderr, "unexpected argument after options\n");
            exit(EXIT_FAILURE);
        }
    }

    repl_init();
    if (mode == EXEC) {
        debug_cmd_exec(&ctx, &args);
    } else if (mode == ATTACH) {
        debug_cmd_attach(&ctx, &args);
    }
    if (args.len)
        list_free(&args);
    repl_loop(&ctx);
    return 0;
}
