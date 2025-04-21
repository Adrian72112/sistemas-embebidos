#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
/*******************************************************************
 *  Obligatorio 1 - Biblioteca de Funciones
 *
 *  Archivo: my_lib_1.h
 *  Autor  : Grupo 1
 *  Fecha  : 29/03/2025
 *
 *  Descripción:
 *      Biblioteca que proporciona funciones básicas para el
 *      funcionamiento del Obligatorio 1 parte 1.
 *
 *
 *******************************************************************/

#ifndef MY_LIB_1_H_
#define MY_LIB_1_H_

typedef struct
{
    float a;
    float b;
    float c;
} coeff_t;

typedef struct
{
    int num_roots;
    double x1;
    double x2;
} root_t;

typedef struct
{
    float real;
    float imag;
} complex_t;

typedef struct
{
    int dia;
    int mes;
    int anio;
} date_t;

typedef struct
{
    int rows;
    int cols;
    int **data;
} matriz_t;

/*******************************************************************
 *  Función: init_lab
 *
 *  Descripción:
 *      Despliegue en pantalla el mensaje “Laboratorio lenguaje C de ..."
 *
 *  Parámetros:
 *      void
 *
 *  Retorno:
 *      void
 *******************************************************************/
void init_lab(void);

/*******************************************************************
 *  Función: eq_solver
 *  Descripción:
 *      Resuelve una ecuación cuadrática de la forma ax^2 + bx + c = 0,
 *      utilizando los coeficientes provistos. Calcula la cantidad de
 *      soluciones reales y sus valores (si existen).
 *
 *  Parámetros:
 *      coeff_t *coeficientes - Puntero a una estructura que contiene
 *                              los coeficientes a, b y c de la ecuación.
 *
 *  Retorno:
 *      root_t - Estructura que contiene el número de soluciones reales
 *               (0, 1 o 2) y los valores correspondientes de las raíces.
 *******************************************************************/
root_t eq_solver(coeff_t *coeficientes);

/*******************************************************************
 *  Función: bin2dec
 *  Descripción:
 *      Convierte un número binario representado como un entero decimal
 *      (por ejemplo, 1011) a su valor equivalente en base decimal.
 *      Si el parámetro `sign` es verdadero, el resultado se devuelve como negativo.
 *
 *  Parámetros:
 *      int32_t binary - Número binario representado como entero (ej: 1011).
 *      bool sign      - Indica si el número binario debe interpretarse como negativo.
 *
 *  Retorno:
 *      int32_t - Valor decimal equivalente, con signo si corresponde.
 *******************************************************************/
int32_t bin2dec(int32_t binary, bool sign);

/*******************************************************************
 *  Función: matrix_sub
 *
 *  Descripción:
 *      Devuelve la resta de dos matrices A y B.
 *
 *  Parámetros:
 *      matriz_t A - Primera matriz.
 *      matriz_t B - Segunda matriz.
 *
 *  Retorno:
 *      matriz_t - Matriz resultante de la resta.
 *******************************************************************/
matriz_t matrix_sub(matriz_t A, matriz_t B);

/*******************************************************************
 *  Función: swap
 *
 *  Descripción:
 *      Intercambia el contenido de dos elementos.
 *
 *  Parámetros:
 *      void *elem_1    - Primer elemento.
 *      void *elem_2    - Segundo elemento.
 *      size_t data_type - Tamanio del tipo de dato.
 *
 *  Retorno:
 *      int - 0 si la operación se realizó con éxito,
 *            -1 si falló.
 *******************************************************************/
int swap(void *elem_1, void *elem_2, size_t data_type);

/*******************************************************************
 *  Función: consonantes
 *
 *  Descripción:
 *      Cuenta el número de consonantes en un string.
 *
 *  Parámetros:
 *      char *string - String de entrada.
 *
 *  Retorno:
 *      int - Número total de consonantes en el string.
 *******************************************************************/
int consonantes(char *string);

/*******************************************************************
 *  Función: print_reverse_array
 *  Descripción:
 *      Imprime los elementos de un arreglo en orden inverso.
 *      El arreglo puede ser de cualquier tipo de datos.
 *
 *  Parámetros:
 *      void *array       - Puntero al inicio del arreglo.
 *      size_t data_type  - Tamaño en bytes de cada elemento (por ejemplo, sizeof(int)).
 *      size_t array_size - Cantidad de elementos en el arreglo.
 *
 *  Retorno:
 *      void - No retorna valor. Imprime directamente los valores en consola.
 *
 *  Nota:
 *      Actualmente soporta impresión de tipos básicos como int, float y char.
 *******************************************************************/
void print_reverse_array(void *array, size_t data_type, size_t array_size);

/*******************************************************************
 *  Función: vocales
 *
 *  Descripción:
 *      Cuenta el número de vocales y consonantes en un string.
 *
 *  Parámetros:
 *      char *string - String de entrada.
 *
 *  Retorno:
 *      int - Número total de consonantes en el string.
 *******************************************************************/
int vocales(char *string);

/*******************************************************************
 *  Función: remplazar caracteres en minúsculas con mayúsculas
 *
 *  Descripción:
 *      Lee una oración y reemplaza los caracteres en minúsculas con mayúsculas o viceversa según decida el usuario
 *
 *  Parámetros:
 *      char *string - String de entrada.
 *
 *  Retorno:
 *      void
 *******************************************************************/
void string_to_caps(char *string);

/*******************************************************************
 *  Función: remplazar caracteres en mayúsculas con minúsculas
 *
 *  Descripción:
 *      Lee una oración y reemplaza los caracteres en mayúsculas con minúsculas o viceversa según decida el usuario
 *
 *  Parámetros:
 *      char *string - String de entrada.
 *
 *  Retorno:
 *      void
 *******************************************************************/
void string_to_min(char *string);

/*******************************************************************
 *  Función: Sumar dos numeros complejos
 *
 *  Descripción:
 *      suma dos numeros complejos
 *
 *  Parámetros:
 *      complex_t a - numero complejo 1.
 *      complex_t b - numero complejo 2.
 *
 *  Retorno:
 *      complex_t
 *******************************************************************/
complex_t sum(complex_t a, complex_t b);

/*******************************************************************
 *  Función: multiplicar dos numeros complejos
 *
 *  Descripción:
 *      multiplicar dos numeros complejos
 *
 *  Parámetros:
 *      complex_t a - numero complejo 1.
 *      complex_t b - numero complejo 2.
 *
 *  Retorno:
 *      complex_t
 *******************************************************************/
complex_t prod(complex_t a, complex_t b);

/*******************************************************************
 *  Función: dada dos fechas devolver la diferencia de dias entre ellas
 *  Descripción:
 *      realizar la diferencia de dias entre las fechas
 *
 *  Parámetros:
 *      date_t start - fecha inicio.
 *      date_t finish - fecha fin.
 *
 *  Retorno:
 *      int
 *******************************************************************/
int days_left(date_t start, date_t finish);

/*******************************************************************
 *  Función: reverse_string
 *  Descripción:
 *      Invierte el contenido de una cadena de texto.
 *
 *  Parámetros:
 *      char *string - Cadena de caracteres a invertir.
 *
 *  Retorno:
 *      char* - Cadena invertida.
 *******************************************************************/
char *reverse_string(char *string);

/*******************************************************************
 *  Función: string_copy
 *  Descripción:
 *      Copia el contenido de un string origen (`source`) a un string destino (`destination`)
 *      incluyendo el carácter nulo de terminación. No utiliza funciones de biblioteca estándar.
 *
 *  Parámetros:
 *      char *source      - Puntero al string fuente.
 *      char *destination - Puntero al string destino (debe tener espacio suficiente).
 *
 *  Retorno:
 *      int - Retorna 0 si la copia fue exitosa, o -1 si algún puntero es nulo.
 *******************************************************************/
int string_copy(char *source, char *destination);

/*******************************************************************
 *  Función: string_words
 *  Descripción:
 *      Cuenta la cantidad de palabras en una cadena de texto.
 *      Se considera palabra a cualquier grupo de caracteres separados por espacios.
 *
 *  Parámetros:
 *      char *string - Cadena de texto a analizar.
 *
 *  Retorno:
 *      int - Número de palabras en la cadena.
 *******************************************************************/
int32_t string_words(char *string);

/*******************************************************************
 *  Función: string_length
 *  Descripción:
 *      Calcula la longitud de una cadena de caracteres.
 *
 *  Parámetros:
 *      char *string - Cadena de texto a analizar.
 *
 *  Retorno:
 *      int - Longitud de la cadena (sin contar el carácter nulo).
 *******************************************************************/
int32_t string_length(char *string);

/*******************************************************************
 *  Función: max_index
 *  Descripción:
 *      Encuentra el índice del valor máximo en un arreglo genérico.
 *
 *  Parámetros:
 *      void *array       - Puntero al inicio del arreglo.
 *      size_t data_type  - Tamaño en bytes de cada elemento.
 *      size_t array_size - Cantidad de elementos en el arreglo.
 *
 *  Retorno:
 *      void - Imprime el índice del valor máximo si es un tipo soportado.
 *******************************************************************/
void max_index(void *array, size_t data_type, size_t array_size);

/*******************************************************************
 *  Función: min_index
 *  Descripción:
 *      Encuentra el índice del valor mínimo en un arreglo genérico.
 *
 *  Parámetros:
 *      void *array       - Puntero al inicio del arreglo.
 *      size_t data_type  - Tamaño en bytes de cada elemento.
 *      size_t array_size - Cantidad de elementos en el arreglo.
 *
 *  Retorno:
 *      void - Imprime el índice del valor mínimo si es un tipo soportado.
 *******************************************************************/
void min_index(void *array, size_t data_type, size_t array_size);

/*******************************************************************
 *  Función: find_in_string
 *  Descripción:
 *      Busca si una subcadena (needle) está contenida dentro de otra (haystack).
 *      Retorna la posición donde comienza la coincidencia, o -1 si no se encuentra.
 *      No utiliza funciones de biblioteca estándar.
 *
 *  Parámetros:
 *      char *haystack - Cadena donde se buscará.
 *      char *needle   - Subcadena a buscar.
 *
 *  Retorno:
 *      int - Índice donde comienza needle dentro de haystack, o -1 si no se encuentra.
 *******************************************************************/
int find_in_string(char *haystack, char *needle);

void print_coeff(coeff_t c);
void print_root(root_t r);
void print_complex(complex_t c);
void print_date(date_t d);
void print_matrix(matriz_t m);

#endif /* MY_LIB_1_H_ */