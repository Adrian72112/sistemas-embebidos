#include <stdio.h>
#include <stdint.h>

void hello_world(void)
{
    printf("Hello world!");
}

void size_of_types(void)
{
    printf("Size of int8_t: %zu\n", sizeof(int8_t));
    printf("Size of int16_t: %zu\n", sizeof(int16_t));
    printf("Size of int32_t: %lu\n", (unsigned long)sizeof(int32_t));
    printf("Size of int64_t: %lu\n", (unsigned long)sizeof(int64_t));
    printf("Size of uint8_t: %zu\n", sizeof(uint8_t));
    printf("Size of uint16_t: %zu\n", sizeof(uint16_t));
    printf("Size of uint32_t: %zu\n", sizeof(uint32_t));
    printf("Size of uint64_t: %zu\n", sizeof(uint64_t));
}

void main()
{
    hello_world();
    size_of_types();
}