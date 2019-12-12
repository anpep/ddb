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

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stddef.h>
#include <assert.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"
#include "debug.h"

static struct debug_cmd _commands[] = {{'e', "(exec) forks the current process and breaks before execution", debug_cmd_exec,     0},
                                       {'a', "(attach) attach to a running process",                         debug_cmd_attach,   0},
                                       {'d', "(detach) detaches from the current inferior process",          debug_cmd_detach,   0},
                                       {'c', "(continue) continues execution on the inferior process",       debug_cmd_continue, 0},
                                       {'i', "(info) displays process info",                                 debug_cmd_info,     0},
                                       {'q', "(quit) terminates the debugging session",                      debug_cmd_quit,     0},
                                       {'h', "(help) displays a list of the available commands",             debug_cmd_help,     0}};

static int unquote_args(const char *str, size_t len, struct list *args)
{
    int skip = 1;
    size_t i, j;
    enum {
        INITIAL, IN_QUOTED
    } state = INITIAL;
    char cur_arg[256];

    assert(list_create(args, 0) != -1);
    for (i = 0, j = 0; i <= len; i++) {
        switch (state) {
        case INITIAL:
            if (str[i] == '"') {
                state = IN_QUOTED;
                continue;
            } else if (isspace(str[i]) || str[i] == '\0') {
                /* push argument */
                if (!j && skip)
                    continue;
                cur_arg[j++] = '\0';
                char *arg = malloc(j);
                if (!arg)
                    return -1;
                memcpy(arg, cur_arg, j);
                assert(list_insert(args, arg) != -1);
                j = 0;
                skip = 1;
                continue;
            }
            break;
        case IN_QUOTED:
            if (str[i] == '"') {
                state = INITIAL;
                skip = 0; /* don't skip seemingly empty ending argument */
                continue;
            }
            break;
        }
        cur_arg[j++] = str[i];
    }
    if (state == IN_QUOTED) {
        printf(ERROR "expected closing quote at column %lu\n", i);
        return -1;
    }
    return 0;
}

static void args_free(const struct list *args)
{
    for (size_t i = 0; i < args->len; i++)
        free(list_at(void *, args, i));
}

int debug_eval(struct debug_ctx *ctx, const char *expr)
{
    size_t len = strlen(expr);
    char *cmd = (char *) expr;
    struct list args = {0};
    long car = 1;
    int res = 0;

    if (!len)
        return 0;

    /* cardinality */
    if (isdigit(cmd[0])) {
        car = strtol(expr, &cmd, 10);
        if (car == LONG_MAX || car <= 0) {
            printf(ERROR "command cardinality is out of range\n");
            return -1;
        }
        len -= cmd - expr;
        if (!len)
            return 0;
    }

    if (!isalpha(cmd[0])) {
        printf(ERROR "invalid command name\n");
        return -1;
    }
    for (size_t i = 0; i < sizeof(_commands) / sizeof(struct debug_cmd); i++) {
        if (_commands[i].name != cmd[0])
            continue;
        /* command name matched */
        if (len > 1 && isspace(cmd[1]) &&
            unquote_args(cmd + 2, len, &args) == -1) {
            args_free(&args);
            list_free(&args);
            return -1;
        }
        while (car--) {
            /* "consume" cardinality and execute command */
            if ((res = _commands[i].fn(ctx, &args)) < 0)
                return res;
        }
        args_free(&args);
        list_free(&args);
    }
    if (car != -1) {
        /* cardinality not "consumed" */
        printf(ERROR "unrecognized debugger command\n");
        return -1;
    }
    return res;
}

/* reset debugger status from a previous attached process */
static void flush_prev(struct debug_ctx *ctx)
{
    ctx->status = PAUSED;
    if (ctx->argvl.data)
        list_free(&ctx->argvl);
    if (ctx->ipath != NULL) {
        free(ctx->ipath);
        ctx->ipath = NULL;
    }
    printf("detached from process %u\n", ctx->pid);
    ctx->pid = 0;
}

int debug_loop(struct debug_ctx *ctx)
{
    int status;

    if (!ctx->pid)
        return 0;
    if (ctx->status == RUNNING) {
        if (waitpid(ctx->pid, &status, 0) && WIFEXITED(status)) {
            printf("inferior process %u exited with status %d\n", ctx->pid,
                   status);
            flush_prev(ctx);
            return 0;
        }
        if (ptrace(PTRACE_CONT, ctx->pid, NULL, NULL) == -1) {
            perror(ERROR "ptrace[PTRACE_CONT]");
            if (errno == ESRCH) /* no such process */
                flush_prev(ctx);
            return 0;
        }
    }
    return 0;
}

int debug_cmd_exec(struct debug_ctx *ctx, const struct list *args)
{
    pid_t pid;

    if (!args->len) {
        printf(ERROR "expected a command line\n");
        return -1;
    }
    if (ctx->pid) {
        printf("inferior process %u is still attached. Terminate it? [y/n]: ",
               ctx->pid);
        if (yes()) {
            if (kill(ctx->pid, SIGTERM) == -1) {
                perror(WARNING "kill[SIGTERM]");
                /* previous inferior probably exited -- attach anyway */
            }
        }
        debug_cmd_detach(ctx, NULL);
    }
    switch (pid = fork()) {
    case 0: /* child */
        /* NUL-terminate argv */
        assert(list_insert((struct list *) args, NULL) != -1);
        if (execvp(list_at(char *, args, 0), (char **) args->data)) {
            perror(ERROR "execvp");
            exit(0);
        }
        exit(0);
    default: { /* parent */
        /* construct args for (a)ttach command */
        char pid_s[8];
        struct list att_args;
        sprintf(pid_s, "%u", pid);
        assert(list_create(&att_args, 1) != -1);
        assert(list_insert(&att_args, pid_s) != -1);
        /* clone argv so debug_cmd_attach doesn't have to retrieve from procfs */
        assert(list_create(&ctx->argvl, args->len) != -1);
        for (size_t i = 0; i < args->len; i++) {
            const char *arg = list_at(const char *, args, i);
            const size_t len = strlen(arg);
            assert(list_insert(&ctx->argvl, malloc(1 + len)) != -1);
            strncpy(list_at(char *, &ctx->argvl, i), arg, len);
        }
        /* actually attach to PID */
        if (debug_cmd_attach(ctx, &att_args) < 0) {
            list_free(&att_args);
            return -1;
        }
        list_free(&att_args);
        return 0;
    }
    }
}

int debug_cmd_attach(struct debug_ctx *ctx, const struct list *args)
{
    unsigned long pid;

    if (ctx->pid)
        debug_cmd_detach(ctx, NULL);
    if (args->len != 1) {
        printf(ERROR "expected process ID as argument\n");
        return -1;
    }
    pid = strtoul(list_at(char *, args, 0), NULL, 10);
    if (!pid || pid == ULONG_MAX) {
        printf(ERROR "invalid process ID\n");
        return -1;
    }
    printf("attaching to process %lu\n", pid);
    if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
        perror(ERROR "ptrace[PTRACE_ATTACH]");
        return -1;
    }
    ctx->pid = pid;
    ctx->status = PAUSED;
    if (ctx->argvl.data == NULL) {
        // TODO: retrieve from /proc/<pid>/cmdline
    }
    return 0;
}

int debug_cmd_continue(struct debug_ctx *ctx, const struct list *args)
{
    if (!ctx->pid) {
        printf(ERROR "no process attached\n");
        return -1;
    }
    if (ptrace(PTRACE_CONT, ctx->pid, NULL, SIGCONT) == -1) {
        perror(ERROR "ptrace[PTRACE_CONT]");
        return -1;
    }
    ctx->status = RUNNING;
    return 0;
}

int debug_cmd_detach(struct debug_ctx *ctx,
                     const struct list *args __attribute__((unused)))
{
    if (!ctx->pid) {
        printf(ERROR "no process attached\n");
        return -1;
    }
    if (ptrace(PTRACE_DETACH, ctx->pid, NULL, NULL) == -1) {
        /* inferior probably exited -- ignore */
        perror(WARNING "ptrace[PTRACE_DETACH]");
    }
    flush_prev(ctx);
    return 0;
}

int debug_cmd_info(struct debug_ctx *ctx, const struct list *args)
{
    if (!ctx->pid) {
        printf(ERROR "no process attached\n");
        return -1;
    }
    printf("pid: %u\n", ctx->pid);
    printf("args(%lu):\n", ctx->argvl.len);
    for (size_t i = 0; i < ctx->argvl.len; i++)
        printf("\t%lu: %s\n", i, list_at(char *, &ctx->argvl, i));
    printf("image: %s\n", ctx->ipath ? ctx->ipath : "");
    return 0;
}

int debug_cmd_quit(struct debug_ctx *ctx, const struct list *args)
{
    if (!ctx->pid)
        exit(0);
    printf("inferior process %u is still attached. Terminate it? [y/n]: ",
           ctx->pid);
    if (yes()) {
        if (kill(ctx->pid, SIGTERM) == -1) {
            /* the inferior process probably exited -- quit anyway */
            perror(WARNING "kill[SIGTERM]");
        }
        exit(0);
    }
    debug_cmd_detach(ctx, NULL);
    exit(0);
}

int debug_cmd_help(struct debug_ctx *ctx, const struct list *args)
{
    for (size_t i = 0; i < sizeof(_commands) / sizeof(struct debug_cmd); i++) {
        if (args->len) {
            for (size_t j = 0; j < args->len; j++) {
                if (strlen(list_at(const char *, args, j)) == 1 &&
                    list_at(const char *, args, j)[0] == _commands[i].name &&
                    !_commands[i].user) {
                    printf("%c  %s\n", _commands[i].name, _commands[i].help);
                    /* mark this command so it's not shown again this time */
                    _commands[i].user = 1;
                }
            }
        } else {
            printf("%c: %s\n", _commands[i].name, _commands[i].help);
        }
        _commands[i].user = 0; /* reset user flag for every command */
    }
    return 0;
}
