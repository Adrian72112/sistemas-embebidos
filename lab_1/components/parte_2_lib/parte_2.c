#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parte_2.h"

// Crea el primer nodo (cabeza) de la lista de estudiantes a partir de los datos proporcionados.
nodo_estudiante_t *crear_lista_de_estudiantes(const estudiante_t *estudiante)
{
    if (estudiante == NULL)
    {
        return NULL; // No se proporcionaron datos de estudiante.
    }

    nodo_estudiante_t *nuevo_nodo = malloc(sizeof(nodo_estudiante_t));
    if (nuevo_nodo == NULL)
    {
        perror("Error al asignar memoria para la lista");
        return NULL;
    }
    nuevo_nodo->dato = *estudiante;
    nuevo_nodo->siguiente = NULL;
    return nuevo_nodo;
}

// Agrega un nuevo estudiante al final de la lista.
void agregar_estudiante(nodo_estudiante_t *lista, const estudiante_t *estudiante)
{
    if (lista == NULL || estudiante == NULL)
    {
        return;
    }

    // Recorremos la lista hasta llegar al último nodo.
    nodo_estudiante_t *actual = lista;
    while (actual->siguiente != NULL)
    {
        actual = actual->siguiente;
    }

    // Reservar memoria para un nuevo nodo.
    nodo_estudiante_t *nuevo = malloc(sizeof(nodo_estudiante_t));
    if (nuevo == NULL)
    {
        perror("Error al asignar memoria para un nuevo estudiante");
        return;
    }

    nuevo->dato = *estudiante;
    nuevo->siguiente = NULL;

    // Enlazar el nuevo nodo al final de la lista.
    actual->siguiente = nuevo;
}

// Elimina el estudiante cuya cédula (ci) coincida. Se usa un doble puntero para actualizar
// la cabeza de la lista en caso de que se elimine el primer nodo.
void eliminar_estudiante(nodo_estudiante_t **lista, const char *ci)
{
    if (lista == NULL || *lista == NULL || ci == NULL)
    {
        printf("La lista está vacía o los parámetros son inválidos.\n");
        return;
    }

    // Utilizamos un puntero a puntero para recorrer y modificar la lista.
    nodo_estudiante_t **indirect = lista;
    while (*indirect != NULL)
    {
        if (strcmp((*indirect)->dato.ci, ci) == 0)
        {
            nodo_estudiante_t *temp = *indirect;
            *indirect = (*indirect)->siguiente; // Desconectar el nodo encontrado.
            free(temp);
            printf("Estudiante con CI %s eliminado.\n", ci);
            return;
        }
        indirect = &((*indirect)->siguiente);
    }
    printf("Estudiante con CI %s no encontrado.\n", ci);
}

// Muestra los estudiantes de la lista en base al filtro recibido. Si el filtro no coincide
// con un atributo específico se muestra toda la información.
void mostrar_estudiantes(const nodo_estudiante_t *lista, atributo_estudiante_t filter)
{
    if (lista == NULL)
    {
        printf("No hay estudiantes para mostrar.\n");
        return;
    }

    while (lista != NULL)
    {
        switch (filter)
        {
        case NOMBRE:
            printf("Nombre: %s\n", lista->dato.nombre);
            break;
        case APELLIDO:
            printf("Apellido: %s\n", lista->dato.apellido);
            break;
        case CI:
            printf("CI: %s\n", lista->dato.ci);
            break;
        case GRADO:
            printf("Grado: %c\n", lista->dato.grado);
            break;
        case PROMEDIO_CALIFICACION:
            printf("Promedio de Calificación: %.2f (%s)\n",
                   lista->dato.promedio_calificacion,
                   calificacion_letra(lista->dato.promedio_calificacion));
            break;
        default:
            /* Si se pasa un filtro no definido, se muestra toda la información */
            printf("Nombre: %s, Apellido: %s, CI: %s, Grado: %c, Promedio: %.2f (%s)\n",
                   lista->dato.nombre,
                   lista->dato.apellido,
                   lista->dato.ci,
                   lista->dato.grado,
                   lista->dato.promedio_calificacion,
                   calificacion_letra(lista->dato.promedio_calificacion));
            break;
        }
        lista = lista->siguiente;
    }
}

void mostrar_lista_ordenada_por_ci(nodo_estudiante_t *lista)
{
    if (!lista)
    {
        printf("No hay estudiantes para mostrar.\n");
        return;
    }

    // Contar cuántos elementos hay
    int count = 0;
    nodo_estudiante_t *temp = lista;
    while (temp != NULL)
    {
        count++;
        temp = temp->siguiente;
    }

    // Crear array de punteros a estudiantes
    estudiante_t *arr[count];
    temp = lista;
    for (int i = 0; i < count; i++)
    {
        arr[i] = &temp->dato;
        temp = temp->siguiente;
    }

    // Ordenar array de punteros usando strcmp por CI
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = i + 1; j < count; j++)
        {
            if (strcmp(arr[i]->ci, arr[j]->ci) > 0)
            {
                estudiante_t *aux = arr[i];
                arr[i] = arr[j];
                arr[j] = aux;
            }
        }
    }

    // Mostrar la lista ordenada
    printf("Estudiantes ordenados por CI:\n");
    for (int i = 0; i < count; i++)
    {
        printf("Nombre: %s, Apellido: %s, CI: %s, Grado: %c, Promedio: %.2f (%s)\n",
               arr[i]->nombre,
               arr[i]->apellido,
               arr[i]->ci,
               arr[i]->grado,
               arr[i]->promedio_calificacion,
               calificacion_letra(arr[i]->promedio_calificacion));
    }
}

const char *calificacion_letra(float calificacion)
{
    if (calificacion <= 30)
        return "D";
    else if (calificacion <= 60)
        return "R";
    else if (calificacion <= 75)
        return "B";
    else if (calificacion <= 81)
        return "BMB";
    else if (calificacion <= 94)
        return "MB";
    else
        return "S";
}

void liberarLista(nodo_estudiante_t *lista)
{
    nodo_estudiante_t *temp;
    while (lista != NULL)
    {
        temp = lista;
        lista = lista->siguiente;
        free(temp);
    }
}