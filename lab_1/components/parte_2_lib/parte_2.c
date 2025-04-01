#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "parte_2.h"

nodo_estudiante_t *crear_lista_de_estudiantes(estudiante_t *estudiante)
{
    nodo_estudiante_t *nuevo_nodo = malloc(sizeof(nodo_estudiante_t));
    if (nuevo_nodo == NULL)
    {
        return NULL; // Error al asignar memoria
    }

    nuevo_nodo->dato = *estudiante;
    nuevo_nodo->siguiente = NULL;

    return nuevo_nodo;
}

void agregar_estudiante(nodo_estudiante_t *lista, estudiante_t *estudiante)
{
    // Recorremos la lista hasta llegar al último nodo.
    while (lista->siguiente != NULL)
    {
        lista = lista->siguiente;
    }

    // Reservar memoria para un nuevo nodo.
    nodo_estudiante_t *nuevo = malloc(sizeof(nodo_estudiante_t));
    if (nuevo == NULL)
    {
        // Manejo de error: no se pudo asignar memoria.
        return;
    }

    // Asignar los datos del estudiante y establecer el siguiente en NULL.
    nuevo->dato = *estudiante;
    nuevo->siguiente = NULL;

    // Enlazar el nuevo nodo al final de la lista.
    lista->siguiente = nuevo;
}