/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytche@lackname.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _STRING_H_
#define _STRING_H_

#include <stdarg.h>

bool
isspace(char c);

bool
strncmp(const char *s1, const char *s2, size_t len);

bool
strcmp(const char *s1, const char *s2);

size_t
strlen(const char *s);

char *
strtok(char *str, const char *sep);

size_t
strlcpy(char *dst, const char *src, size_t max);

size_t
snprintf(char *str, size_t max, const char *fmt, ...);

int
sscanf(const char *str, const char *fmt, ...);

long
strtol(const char *nptr, char **endptr, int base);

size_t
vsnprintf(char *str, size_t max, const char *fmt, va_list ap);

int
vfscanf(int fd, const char *fmt, va_list ap);

int
vsscanf(const char *str, const char *fmt, va_list ap);

#endif

