#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "my_lib_1.h"

// Función que devuelve la resta de dos matrices A - B.
// Se asume que ambas matrices tienen las mismas dimensiones.
matriz_t matrix_sub(matriz_t A, matriz_t B)
{
    matriz_t result = {0, 0, NULL};
    if (A.rows != B.rows || A.cols != B.cols)
    {
        fprintf(stderr, "Error: Las matrices deben tener las mismas dimensiones.\n");
        return result;
    }
    result.rows = A.rows;
    result.cols = A.cols;
    result.data = malloc(result.rows * sizeof(int *));
    if (!result.data)
    {
        fprintf(stderr, "Error en la asignación de memoria.\n");
        return result;
    }
    for (int i = 0; i < result.rows; i++)
    {
        result.data[i] = malloc(result.cols * sizeof(int));
        if (!result.data[i])
        {
            fprintf(stderr, "Error en la asignación de memoria.\n");
            for (int j = 0; j < i; j++)
                free(result.data[j]);
            free(result.data);
            result.data = NULL;
            result.rows = result.cols = 0;
            return result;
        }
        // Resta elemento a elemento
        for (int j = 0; j < result.cols; j++)
        {
            result.data[i][j] = A.data[i][j] - B.data[i][j];
        }
    }
    return result;
}

// Función swap: intercambia el contenido de dos elementos.
// Recibe punteros a los elementos y el tamaño en bytes del tipo de dato.
int swap(void *elem_1, void *elem_2, size_t data_type)
{
    if (!elem_1 || !elem_2)
        return -1;
    char *p = (char *)elem_1;
    char *q = (char *)elem_2;
    for (size_t i = 0; i < data_type; i++)
    {
        char temp = p[i];
        p[i] = q[i];
        q[i] = temp;
    }
    return 0;
}

// Función que cuenta el número de vocales y consonantes en un string.
// Imprime ambos valores y devuelve el número de consonantes.
int consonantes(char *string)
{
    int vowels = 0, cons = 0;
    if (!string)
        return 0;
    for (char *p = string; *p; p++)
    {
        if (isalpha(*p))
        {
            char ch = tolower(*p);
            if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
                vowels++;
            else
                cons++;
        }
    }
    printf("Vocales: %d, Consonantes: %d\n", vowels, cons);
    return cons;
}
