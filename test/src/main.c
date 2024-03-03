#include <lib.h>
#include <stdio.h>

int main(const int argc, const char** argv)
{
    printf("test_lib_add(3, 5) = %i\n", test_lib_add(3, 5));
    printf("test_lib_mul(2, 4) = %i\n", test_lib_mul(2, 4));
    printf("test_lib_sub(9, 1) = %i\n", test_lib_sub(9, 1));
    return 0;
}
