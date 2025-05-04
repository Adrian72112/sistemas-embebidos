#include <stdio.h>

#define ARRAY_SIZE 10

int exampleData;
char exampleArray[ARRAY_SIZE];

void app_main(void)
{
    exampleData = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        exampleArray[i] = 'A' + i;
    }
    printf("exampleData: %d\n", exampleData);
}