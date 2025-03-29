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
 *      size_t data_type - Tamaño del tipo de dato.
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

#endif /* MY_LIB_1_H_ */