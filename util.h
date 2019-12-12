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

#include <unistd.h>

#define ERROR "\x1b[0;31merror:\x1b[0m "
#define WARNING "\x1b[0;33mwarning:\x1b[0m "

#define statf(m, ...) fprintf(stderr, "\x1b[2m" m "\x1b[0m", ##__VA_ARGS__)

static inline int yes(void)
{
    char res[2];

    if (read(STDIN_FILENO, res, sizeof(res)) != sizeof(res))
        return 0;
    return res[0] == 'y' || res[0] == 'Y';
}
