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
 *  Función: ecuacion
 *  Descripción:
 *      Determina la cantidad de soluciones reales de una ecuación
 *      cuadrática de la forma ax^2 + bx + c = 0.
 *
 *  Parámetros:
 *      float a - Coeficiente cuadrático.
 *      float b - Coeficiente lineal.
 *      float c - Término independiente.
 *
 *  Retorno:
 *      int - Número de soluciones reales (0, 1 o 2).
 *******************************************************************/
int ecuacion(float a, float b, float c);

/*******************************************************************
 *  Función: cuentaletras
 *  Descripción:
 *      Cuenta la cantidad de letras (caracteres alfabéticos) en una palabra.
 *
 *  Parámetros:
 *      char *palabra - Cadena de texto a analizar.
 *
 *  Retorno:
 *      int - Cantidad de letras encontradas.
 *******************************************************************/
int cuentaletras(char *palabra);

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
int string_words(char *string);

/*******************************************************************
 *  Función: binario
 *  Descripción:
 *      Convierte un número binario representado como entero en su equivalente decimal.
 *
 *  Parámetros:
 *      int binario - Número binario (ej: 1011) representado como entero.
 *
 *  Retorno:
 *      int - Valor decimal equivalente.
 *******************************************************************/
int binario(int binario);

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
int string_length(char *string);

int find_in_string(char *palabra, char *palabra2);

#endif /* MY_LIB_1_H_ */