#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "parte_2.h"

void test_crear_lista_de_estudiantes(void)
{
    estudiante_t e;
    strcpy(e.nombre, "Juan");
    strcpy(e.apellido, "Perez");
    strcpy(e.ci, "12345678");
    e.grado = 'A';
    e.promedio_calificacion = 8.5f;

    nodo_estudiante_t *lista = crear_lista_de_estudiantes(&e);
    assert(lista != NULL);
    assert(strcmp(lista->dato.nombre, "Juan") == 0);
    assert(strcmp(lista->dato.apellido, "Perez") == 0);
    assert(strcmp(lista->dato.ci, "12345678") == 0);
    assert(lista->dato.grado == 'A');
    assert(lista->dato.promedio_calificacion == 8.5f);

    // Liberar la memoria utilizada por la lista
    liberarLista(lista);
}

void test_agregar_estudiante(void)
{
    estudiante_t e1, e2;
    strcpy(e1.nombre, "Juan");
    strcpy(e1.apellido, "Perez");
    strcpy(e1.ci, "12345678");
    e1.grado = 'A';
    e1.promedio_calificacion = 8.5f;

    strcpy(e2.nombre, "Ana");
    strcpy(e2.apellido, "Garcia");
    strcpy(e2.ci, "87654321");
    e2.grado = 'B';
    e2.promedio_calificacion = 7.0f;

    // Crear la lista con el primer estudiante
    nodo_estudiante_t *lista = crear_lista_de_estudiantes(&e1);
    assert(lista != NULL);

    // Agregar un segundo estudiante
    agregar_estudiante(lista, &e2);

    // Verificamos que el segundo nodo se agregó correctamente
    assert(lista->siguiente != NULL);
    assert(strcmp(lista->siguiente->dato.nombre, "Ana") == 0);
    assert(strcmp(lista->siguiente->dato.apellido, "Garcia") == 0);
    assert(strcmp(lista->siguiente->dato.ci, "87654321") == 0);
    assert(lista->siguiente->dato.grado == 'B');
    assert(lista->siguiente->dato.promedio_calificacion == 7.0f);

    liberarLista(lista);
}

void test_eliminar_estudiante(void)
{
    estudiante_t e1, e2, e3;
    strcpy(e1.nombre, "Juan");
    strcpy(e1.apellido, "Perez");
    strcpy(e1.ci, "12345678");
    e1.grado = 'A';
    e1.promedio_calificacion = 8.5f;

    strcpy(e2.nombre, "Ana");
    strcpy(e2.apellido, "Garcia");
    strcpy(e2.ci, "87654321");
    e2.grado = 'B';
    e2.promedio_calificacion = 7.0f;

    strcpy(e3.nombre, "Luis");
    strcpy(e3.apellido, "Martinez");
    strcpy(e3.ci, "11223344");
    e3.grado = 'C';
    e3.promedio_calificacion = 9.0f;

    // Crear lista con tres estudiantes
    nodo_estudiante_t *lista = crear_lista_de_estudiantes(&e1);
    assert(lista != NULL);
    agregar_estudiante(lista, &e2);
    agregar_estudiante(lista, &e3);

    // La lista tiene: [Juan] -> [Ana] -> [Luis]
    // Eliminar el nodo intermedio (Ana)
    eliminar_estudiante(&lista, "87654321");
    assert(lista != NULL);
    // Verificar que el nodo siguiente de Juan es Luis
    assert(lista->siguiente != NULL);
    assert(strcmp(lista->siguiente->dato.nombre, "Luis") == 0);

    // Eliminar la cabeza (Juan)
    eliminar_estudiante(&lista, "12345678");
    // Ahora la cabeza debe ser Luis
    assert(lista != NULL);
    assert(strcmp(lista->dato.nombre, "Luis") == 0);

    // Eliminar el último nodo (Luis)
    eliminar_estudiante(&lista, "11223344");
    // La lista ahora debe estar vacía
    assert(lista == NULL);
}

void test_mostrar_lista_ordenada_por_ci(void)
{
    estudiante_t e1, e2, e3;
    strcpy(e1.nombre, "Juan");
    strcpy(e1.apellido, "Perez");
    strcpy(e1.ci, "22334455");
    e1.grado = 'A';
    e1.promedio_calificacion = 8.5f;

    strcpy(e2.nombre, "Ana");
    strcpy(e2.apellido, "Garcia");
    strcpy(e2.ci, "11223344");
    e2.grado = 'B';
    e2.promedio_calificacion = 7.0f;

    strcpy(e3.nombre, "Luis");
    strcpy(e3.apellido, "Martinez");
    strcpy(e3.ci, "33445566");
    e3.grado = 'C';
    e3.promedio_calificacion = 9.0f;

    nodo_estudiante_t *lista = crear_lista_de_estudiantes(&e1);
    agregar_estudiante(lista, &e2);
    agregar_estudiante(lista, &e3);

    printf(">>> Test mostrar_lista_ordenada_por_ci:");
    mostrar_lista_ordenada_por_ci(lista);

    liberarLista(lista);
}

int main(void)
{
    test_crear_lista_de_estudiantes();
    printf("✅ test_crear_lista_de_estudiantes passed.\n");

    test_agregar_estudiante();
    printf("✅ test_agregar_estudiante passed.\n");

    test_eliminar_estudiante();
    printf("✅ test_eliminar_estudiante passed.\n");

    test_mostrar_lista_ordenada_por_ci();
    printf("✅ test_mostrar_lista_ordenada_por_ci ran (verificar orden visual).\n");

    printf("🎉 Todos los tests de parte 2 completados.\n");
    return 0;
}
