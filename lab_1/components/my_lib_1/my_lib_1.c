#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "my_lib_1.h"

void init_lab(void)
{
    printf("Laboratorio lenguaje C de Grupo 1\n");
    printf("Integrantes:\n");
    printf("1. Sofia Nicoletti   1\n");
    printf("2. Pierina Borsieri 2\n");
    printf("3. Agustina Bacigalupe 3\n");
    printf("4. Adrián Tesore 4\n");
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
    printf("Vocales: %d, Consonantes: %d\n", vowels, cons);
    return cons;
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

// Funcion days_left realiza la diferencia de dias entre dos fechas
//  Imprime el resultado de la diferencia

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
// Calcula la diferencia de dias entre dos fechas
int days_left(date_t start, date_t finish)
{
    int start_days = total_days_until(start);
    int end_days = total_days_until(finish);
    return end_days - start_days;
}

int binario(int binario)
{
    int decimal = 0, potencia = 0, digito;
    scanf("%lld", &binario); // "%lld" lee un número ingresado por el usuario y lo guarda en la variable binario

    while (binario > 0)
    {
        digito = binario % 10;
        decimal += digito * pow(2, potencia); // sumar su valor en decimal
        binario /= 10;
        potencia++;
    }

    printf("El equivalente decimal es: %d\n", decimal);
    return 0;
}

int string_words(char *string)
{
    int contador = 0;
    int enpalabra = 0; // 0 si está fuera de una palabra, 1 si está dentro.

    while (*string)
    { // mientras no lleguemos al final del string
        if (*string != ' ' && *string != '\n')
        { // distno a espacio, distino a enter
            if (!enpalabra)
            {                  //
                contador++;    //
                enpalabra = 1; //
            }
        }
        else
        {
            enpalabra = 0; // cuando no estamos en palbra osea estoy en espacio
        }
        string++; // pasamos al siguiente carácter
    }
    return contador;
}

int cuentaletras(char *palabra)
{
    int contador = 0;

    for (int i = 0; palabra[i] != '\0'; i++)
    { // recorre la cadena hasta encontrar el carácter nulo '\0'
        contador++;
    }

    return contador;
}

int ecuacion(float a, float b, float c)
{

    double dentroraiz, x1, x2;

    if (a == 0)
    {
        printf("no es de segundo grado");
        return 0;
    }

    dentroraiz = b * b - 4 * a * c;
    if (dentroraiz > 0)
    {
        x1 = (-b + sqrt(dentroraiz)) / (2 * a);
        x2 = (-b - sqrt(dentroraiz)) / (2 * a);
        printf("Las soluciones son: x1 = %.2lf y x2 = %.2lf\n", x1, x2);
    }
    return 0;
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

int string_length(char *string)
{
    int contador = 0;

    // Corrección: Incrementar contador dentro del for
    for (; *string != '\0'; string++)
    {
        contador++;
    }

    return contador;
}

int find_in_string(char *palabra, char *palabra2)
{
    for (; *palabra2 != '\0'; palabra2++)
    { // Recorremos palabra2 con punteros
        if (*palabra2 == *palabra)
        {                        // Si encontramos el primer carácter de palabra./compara los valores
            char *p1 = palabra;  //*p1 apunta al valor donde comienza la subcadena. es decir donde comienzan a coincidir
            char *p2 = palabra2; //*p2 apunta a la posicion actual en palabra2

            while (*p1 != '\0' && *p2 == *p1)
            { // mientras *p1 no sea nulo y *p2 sea igual a *p1 sumo los punteros.
                p1++;
                p2++;
            }

            if (*p1 == '\0')
            { // Si llegamos al final de 'palabra', significa que la encontramos
                return 1;
            }
        }
    }
    return 0; // Si no encontramos la palabra, retornamos 0
}