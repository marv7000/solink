#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    size_t capacity;
    size_t length;
    uint8_t* data;
} str;

/// \brief      Creates a new empty string.
/// \returns    A reference to the newly created string.
str str_new();

/// \brief                  Creates a new string from a null terminated string.
/// \param  [in]    text    A null terminated string.
/// \returns                A reference to the newly created string.
str str_new_text(const char* text);

/// \brief                  Frees the string.
/// \param  [in]    string  A reference to the string to free.
void str_free(str string);

/// \brief                  Expands the capacity to ensure that enough memory is available.
/// \param  [in]    this    A reference to the object to operate on.
/// \param          cap     The minimum capacity this string should have.
void str_reserve(str* this, size_t cap);

/// \brief                  Copies the given string.
/// \param  [in]    string  The string to copy.
/// \returns				A copy of the string.
str str_copy(str this);

/// \brief                  Concatenates two strings together.
/// \param  [in]    this    A reference to the object to operate on.
/// \param  [in]    other   The string to concatenate.
void str_concat(str* this, str other);

/// \brief                  Checks if the string is empty.
/// \param  [in]    string  A reference to the string to check.
/// \returns                True if empty, otherwise false.
bool str_empty(str string);

/// \brief                  Converts the string into a null terminated string.
/// \param  [in]    string  A reference to the string.
/// \returns                A null terminated string.
char* str_cstr(str string);

/// \brief                  Checks if two strings are equal.
/// \param  [in]    first   The first string.
/// \param  [in]    second  The second string.
/// \returns                True if equal, otherwise false.
bool str_equal(str first, str second);

bool str_equal_c(str first, const char* second);
