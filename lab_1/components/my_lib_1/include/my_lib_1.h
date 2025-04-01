/*******************************************************************
 *  Obligatorio 1 - Biblioteca de Funciones
 *
 *  Archivo: my_lib_1.h
 *  Autor  : Grupo 1
 *  Fecha  : 29/03/2025
 *
 *  Descripción:
 *      Biblioteca que proporciona funciones básicas para el
 *      funcionamiento del Obligatorio 1.
 *
 *
 *******************************************************************/

#ifndef MY_LIB_1_H_
#define MY_LIB_1_H_

typedef struct
{
    int rows;
    int cols;
    int **data;
} matriz_t;

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
 *      Cuenta el número de vocales y consonantes en un string.
 *
 *  Parámetros:
 *      char *string - String de entrada.
 *
 *  Retorno:
 *      int - Número total de consonantes en el string.
 *******************************************************************/
int consonantes(char *string);
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
void string_to_caps(char*string);
void string_to_min(char*string);
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
typedef struct{
    float real;
    float imag;
}complex_t;
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

 typedef struct{
    int dia;
    int mes;
    int anio;
 }date_t;
int days_left(date_t start, date_t finish);

#endif /* MY_LIB_1_H_ */