#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t size;

typedef char* str;

#define ALIGN(x, by) (x % by == 0 ? x : x + (by - (x % by)))
