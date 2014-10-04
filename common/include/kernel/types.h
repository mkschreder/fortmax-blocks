#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define SUCCESS 1
#define FAIL (-1)

typedef void* handle_t;

#define INVALID_HANDLE 0
#define DEFAULT_HANDLE ((handle_t)0xffff)

typedef int16_t id_t;

#define container_of(ptr, type, member) ({                      \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
