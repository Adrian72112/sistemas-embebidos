/*******************************************************************
 *  Laboratorio 1 - Biblioteca de Funciones parte 2
 *
 *  Archivo: my_lib_2.h
 *  Autor  : Grupo 1
 *  Fecha  : 29/03/2025
 *
 *  Descripción:
 *      Biblioteca que proporciona funciones básicas para el
 *      funcionamiento del Laboratorio 1.
 *
 *
 *******************************************************************/

#ifndef MY_LIB_2_H_
#define MY_LIB_2_H_

// Enum para respresental los atributos de un estudiante
typedef enum
{
    NOMBRE,
    APELLIDO,
    CI,
    GRADO,
    PROMEDIO_CALIFICACION
} atributo_estudiante_t;

// Estructura para almacenar datos de un estudiante
typedef struct
{
    char nombre[50];
    char apellido[50];
    char ci[9];
    char grado;
    float promedio_calificacion;
} estudiante_t;

// Lista enlazada para almacenar estudiantes
typedef struct nodo_estudiante
{
    estudiante_t dato;
    struct nodo_estudiante *siguiente;
} nodo_estudiante_t;

/*******************************************************************
 *  Función: crear_lista_de_estudiantes
 *
 *  Descripción:
 *      Crea y devuelve una lista enlazada de estudiantes, iniciando la
 *      lista con el estudiante proporcionado.
 *
 *  Parámetros:
 *      estudiante_t *estudiante - Puntero a la estructura del estudiante
 *                                 que se usará para crear el primer nodo.
 *
 *  Retorno:
 *      nodo_estudiante_t - Nodo de la lista enlazada inicializado con el estudiante.
 *******************************************************************/
nodo_estudiante_t *crear_lista_de_estudiantes(estudiante_t *estudiante);

/*******************************************************************
 *  Función: agregar_estudiante
 *
 *  Descripción:
 *      Agrega un nuevo estudiante a la lista enlazada de estudiantes.
 *
 *  Parámetros:
 *      nodo_estudiante_t *lista   - Puntero a la lista enlazada de estudiantes.
 *      estudiante_t      *estudiante - Puntero a la estructura del estudiante
 *                                      a agregar.
 *
 *  Retorno:
 *      void - La función no retorna valor.
 *******************************************************************/
void agregar_estudiante(nodo_estudiante_t *lista, estudiante_t *estudiante);

/*******************************************************************
 *  Función: eliminar_estudiante
 *
 *  Descripción:
 *      Elimina de la lista enlazada al estudiante cuyo número de cédula
 *      de identidad (ci) coincide con el valor proporcionado.
 *
 *  Parámetros:
 *      nodo_estudiante_t *lista - Puntero a la lista enlazada de estudiantes.
 *      char              *ci    - Cédula de identidad del estudiante a eliminar.
 *
 *  Retorno:
 *      void - La función no retorna valor.
 *******************************************************************/
void eliminar_estudiante(nodo_estudiante_t *lista, char *ci);

/*******************************************************************
 *  Función: mostrar_estudiantes
 *
 *  Descripción:
 *      Recorre y muestra por pantalla la información de los estudiantes
 *      presentes en la lista, aplicando un filtro basado en el atributo
 *      especificado.
 *
 *  Parámetros:
 *      nodo_estudiante_t       *lista  - Puntero a la lista enlazada de estudiantes.
 *      atributo_estudiante_t    filter - Filtro que determina el atributo de
 *                                         visualización de los estudiantes.
 *
 *  Retorno:
 *      void - La función no retorna valor.
 *******************************************************************/
void mostrar_estudiantes(nodo_estudiante_t *lista, atributo_estudiante_t filter);

#endif /* MY_LIB_2_H_ */