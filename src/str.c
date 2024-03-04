#include <stdlib.h>
#include <string.h>

#include <str.h>

str str_new()
{
    str result = {0};
    return result;
}

str str_new_text(const char* text)
{
    str result = str_new();

    if (text)
    {
        const size_t len = strlen(text);
        result.length = len;

        str_reserve(&result, len);
        memcpy(result.data, text, len);
    }

    return result;
}

void str_free(str string)
{
	string.length = 0;
	string.capacity = 0;
    if (string.data)
    {
	    free(string.data);
		string.data = NULL;
	}
}

void str_reserve(str* this, size_t cap)
{
    if (!this)
        return;

    // Also reserve null terminator.
    cap += 1;

    // Don't do anything if we already have enough space.
    if (this->capacity >= cap)
        return;
    
    // Get the next larger power of 2.
    int power = 1;
    while(power < cap)
        power *= 2;

    // Move the data to a resized pointer.
    uint8_t* new_ptr = (uint8_t*)calloc(sizeof(uint8_t), power);
	this->capacity = power;
    if (this->data)
    {
        memcpy(new_ptr, this->data, this->length);
        free(this->data);
    }
    this->data = new_ptr;
}

void str_concat(str* this, str other)
{
    if (!this)
        return;

    str_reserve(this, this->length + other.length);
	memcpy(this->data + this->length, other.data, other.length);
	this->length += other.length;
}

bool str_empty(str this)
{
    if (!this.data || this.length == 0)
        return true;
    return false;
}

char* str_cstr(str this)
{
    return (char*)this.data;
}

bool str_equal(str first, str second)
{
    if (first.length != second.length)
        return false;
    
    return memcmp(first.data, second.data, first.length) == 0;
}

bool str_equal_c(str first, const char* second)
{
    str sec = str_new_text(second);
    bool result = str_equal(first, sec);
    str_free(sec);
    return result;
}

str str_copy(str this)
{
	return str_new_text(str_cstr(this));
}
