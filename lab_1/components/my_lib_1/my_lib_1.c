#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "my_lib_1.h"

void init_lab(void)
{
    printf(
        "Laboratorio lenguaje C de Grupo 1\n"
        "Integrantes:\n"
        "1. Sofia Nicoletti\t1\n"
        "2. Pierina Borsieri\t2\n"
        "3. Agustina Bacigalupe\t3\n"
        "4. Adrián Tesore\t4\n");
}

root_t eq_solver(coeff_t *coeficientes)
{
    root_t resultado = {0, 0.0, 0.0};

    float a = coeficientes->a;
    float b = coeficientes->b;
    float c = coeficientes->c;

    if (a == 0.0f)
    {
        printf("No es una ecuación de segundo grado.\n");
        return resultado;
    }

    double discriminante = b * b - 4 * a * c;

    if (discriminante > 0.0)
    {
        resultado.x1 = (-b + sqrt(discriminante)) / (2 * a);
        resultado.x2 = (-b - sqrt(discriminante)) / (2 * a);
        resultado.num_roots = 2;
    }
    else if (discriminante == 0.0)
    {
        resultado.x1 = resultado.x2 = -b / (2 * a);
        resultado.num_roots = 1;
    }
    else
    {
        printf("No tiene soluciones reales.\n");
    }

    return resultado;
}

int32_t bin2dec(int32_t binary, bool sign)
{
    int32_t decimal = 0;
    int32_t potencia = 0;
    int32_t digito;

    while (binary > 0)
    {
        digito = binary % 10;
        if (digito == 1)
        {
            decimal += (1 << potencia); // equivalente a pow(2, potencia)
        }
        binary /= 10;
        potencia++;
    }

    return sign ? -decimal : decimal;
}

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
// Recibe punteros a los elementos y el tamanio en bytes del tipo de dato.
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
    return cons;
}

int vocales(char *string)
{
    int letras = 0;
    if (!string)
        return 0;

    for (char *p = string; *p; p++)
    {
        if (isalpha(*p))
            letras++;
    }

    int cons = consonantes(string); // ya imprime
    return letras - cons;
}
// Funcion string_to_caps reemplaza todas las minusculas por mayusculas
void string_to_caps(char *string)
{
    while (*string != '\0')
    {
        if (*string >= 'a' && *string <= 'z')
        {
            *string = *string - ('a' - 'A');
        }
        string++;
    }
}

// Funcion string_to_min reemplaza todas las mayusculas por minusculas
void string_to_min(char *string)
{
    while (*string != '\0')
    {
        if (*string >= 'A' && *string <= 'Z')
        {
            *string = *string - ('A' - 'a');
        }
        string++;
    }
}

// Funcion sum suma dos numeros complejos
// Imprime el resultado de la suma.
complex_t sum(complex_t a, complex_t b)
{
    complex_t resultado;
    resultado.real = a.real + b.real;
    resultado.imag = a.imag + b.imag;
    return resultado;
}

// Funcion prod realiza el productos de dos numeros complejos
//  Imprime el resultado de la multiplicacion
complex_t prod(complex_t a, complex_t b)
{
    complex_t resultado;
    resultado.real = (a.real * b.real) - (a.imag * b.imag);
    resultado.imag = (a.real * b.imag) + (a.imag * b.real);
    return resultado;
}

//  Funcion auxiliar para verificar si el anio es bisiesto
int is_leap_year(int anio)
{
    return (anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0);
}

// Funcion auxiliar devuelve la cantidad de días de un mes específico
int days_in_month(int month, int year)
{
    int days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year))
    {
        return 29;
    }
    return days_per_month[month - 1];
}

// Funcion auxiliar calcula el número total de días desde 01/01/0000 hasta la fecha dada
int total_days_until(date_t date)
{
    int days = 0;

    for (int y = 0; y < date.anio; y++)
    {
        days += is_leap_year(y) ? 366 : 365;
    }

    for (int m = 1; m < date.mes; m++)
    {
        days += days_in_month(m, date.anio);
    }

    days += date.dia;

    return days;
}

int days_left(date_t start, date_t finish)
{
    int start_days = total_days_until(start);
    int end_days = total_days_until(finish);
    return end_days - start_days;
}

int32_t string_words(char *string)
{
    if (!string)
        return 0;

    int32_t contador = 0;
    int en_palabra = 0;

    while (*string)
    {
        if (*string != ' ' && *string != '\n')
        {
            if (!en_palabra)
            {
                contador++;
                en_palabra = 1;
            }
        }
        else
        {
            en_palabra = 0;
        }
        string++;
    }

    return contador;
}

char *reverse_string(char *string)
{
    char *inicio = string;
    char *fin = string + strlen(string) - 1;

    while (inicio < fin)
    {
        char temp = *inicio;
        *inicio = *fin;
        *fin = temp;

        inicio++;
        fin--;
    }
    return string;
}

int32_t string_length(char *string)
{
    if (!string)
        return -1;

    int32_t contador = 0;
    while (*string != '\0')
    {
        contador++;
        string++;
    }

    return contador;
}

int find_in_string(char *haystack, char *needle)
{
    if (!haystack || !needle)
        return -1;

    int index = 0;

    for (; *haystack != '\0'; haystack++, index++)
    {
        if (*haystack == *needle)
        {
            char *p1 = needle;
            char *p2 = haystack;

            while (*p1 && *p2 && *p1 == *p2)
            {
                p1++;
                p2++;
            }

            if (*p1 == '\0')
            {
                return index; // Se encontró la subcadena
            }
        }
    }

    return -1; // No se encontró
}

void print_reverse_array(void *array, size_t data_type, size_t array_size)
{
    for (size_t i = array_size; i > 0; i--)
    {
        void *element = (char *)array + (i - 1) * data_type;

        if (data_type == sizeof(int))
            printf("%d ", *(int *)element);
        else if (data_type == sizeof(float))
            printf("%.2f ", *(float *)element);
        else if (data_type == sizeof(char))
            printf("%c ", *(char *)element);
        else
            printf("? "); // Tipo no soportado
    }
    printf("\n");
}

void max_index(void *array, size_t data_type, size_t array_size)
{
    if (!array || array_size == 0)
        return;

    size_t max_i = 0;
    for (size_t i = 1; i < array_size; i++)
    {
        void *curr = (char *)array + i * data_type;
        void *max = (char *)array + max_i * data_type;

        if (data_type == sizeof(int) && *(int *)curr > *(int *)max)
            max_i = i;
        else if (data_type == sizeof(float) && *(float *)curr > *(float *)max)
            max_i = i;
        else if (data_type == sizeof(char) && *(char *)curr > *(char *)max)
            max_i = i;
    }

    printf("Índice del valor máximo: %zu\n", max_i);
}

void min_index(void *array, size_t data_type, size_t array_size)
{
    if (!array || array_size == 0)
        return;

    size_t min_i = 0;
    for (size_t i = 1; i < array_size; i++)
    {
        void *curr = (char *)array + i * data_type;
        void *min = (char *)array + min_i * data_type;

        if (data_type == sizeof(int) && *(int *)curr < *(int *)min)
            min_i = i;
        else if (data_type == sizeof(float) && *(float *)curr < *(float *)min)
            min_i = i;
        else if (data_type == sizeof(char) && *(char *)curr < *(char *)min)
            min_i = i;
    }

    printf("Índice del valor mínimo: %zu\n", min_i);
}

int string_copy(char *source, char *destination)
{
    if (!source || !destination)
        return -1;

    while (*source)
    {
        *destination = *source;
        source++;
        destination++;
    }

    *destination = '\0'; // Terminar el string copiado
    return 0;
}

void print_matrix(matriz_t m)
{
    printf("Matriz de %d filas x %d columnas:\n", m.rows, m.cols);
    for (int i = 0; i < m.rows; i++)
    {
        for (int j = 0; j < m.cols; j++)
        {
            printf("%d ", m.data[i][j]);
        }
        printf("\n");
    }
}
void print_date(date_t d)
{
    printf("Fecha: %02d/%02d/%d\n", d.dia, d.mes, d.anio);
}
void print_complex(complex_t c)
{
    printf("Complejo: %.2f + %.2fi\n", c.real, c.imag);
}
void print_root(root_t r)
{
    printf("Raíces: num_roots = %d, x1 = %.2lf, x2 = %.2lf\n", r.num_roots, r.x1, r.x2);
}
void print_coeff(coeff_t c)
{
    printf("Coeficientes: a = %.2f, b = %.2f, c = %.2f\n", c.a, c.b, c.c);
}