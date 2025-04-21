#include <stdio.h>
#include <stdlib.h>
#include "my_lib_1.h"
#include "parte_2.h"

// Prototipos de funciones auxiliares para cada acción del menú
void agregarEstudianteMenu(nodo_estudiante_t **lista);
void eliminarEstudianteMenu(nodo_estudiante_t **lista);
void mostrarEstudiantesMenu(nodo_estudiante_t *lista);
void liberarLista(nodo_estudiante_t *lista);

int main(void)
{
    int opcion;
    nodo_estudiante_t *lista = NULL; // Cabeza de la lista enlazada de estudiantes

    do
    {
        // Mostrar el menú principal
        printf("\n****** MENU DE ESTUDIANTES ******\n");
        printf("1. Agregar Estudiante\n");
        printf("2. Eliminar Estudiante\n");
        printf("3. Mostrar Estudiantes\n");
        printf("4. Mostrar Estudiantes Ordenados por CI\n");
        printf("5. Salir\n");
        printf("Seleccione una opcion: ");

        if (scanf("%d", &opcion) != 1)
        {
            fprintf(stderr, "Entrada invalida.\n");
            exit(EXIT_FAILURE);
        }

        switch (opcion)
        {
        case 1:
            // Se pasa la dirección de la cabeza para poder actualizarla
            agregarEstudianteMenu(&lista);
            break;
        case 2:
            if (lista == NULL)
            {
                printf("\nNo existen estudiantes para eliminar.\n");
            }
            else
            {
                // Se envía la dirección de la cabeza para que 'eliminar_estudiante'
                // pueda modificarla si es necesario (por ejemplo, al eliminar el primer nodo).
                eliminarEstudianteMenu(&lista);
            }
            break;
        case 3:
            if (lista == NULL)
            {
                printf("\nLa lista de estudiantes esta vacia.\n");
            }
            else
            {
                mostrarEstudiantesMenu(lista);
            }
            break;
        case 4:
            if (lista == NULL)
            {
                printf("\nLa lista de estudiantes esta vacia.\n");
            }
            else
            {
                mostrar_lista_ordenada_por_ci(lista);
            }
            break;
        case 5:
            printf("\nSaliendo del programa. Adios!\n");
            break;
        default:
            printf("\nOpcion no valida. Intente de nuevo.\n");
        }
    } while (opcion != 5);

    // Liberar la memoria reservada para la lista antes de salir
    liberarLista(lista);

    return 0;
}

/***************************************************************************
 * Funcion: agregarEstudianteMenu
 *
 * Descripcion:
 *  Solicita al usuario los datos de un nuevo estudiante y lo agrega a la
 *  lista enlazada. Si la lista esta vacia, crea el primer nodo.
 *
 * Parametros:
 *  nodo_estudiante_t **lista - Puntero a la direccion de la cabeza de la lista.
 ***************************************************************************/
void agregarEstudianteMenu(nodo_estudiante_t **lista)
{
    estudiante_t nuevoEstudiante;

    // Solicitar datos del estudiante
    printf("\nIngrese el nombre: ");
    scanf(" %[^\n]", nuevoEstudiante.nombre);
    printf("Ingrese el apellido: ");
    scanf(" %[^\n]", nuevoEstudiante.apellido);
    printf("Ingrese la CI (8 caracteres): ");
    scanf(" %8s", nuevoEstudiante.ci); // Se reserva espacio para 8 caracteres + terminador nulo
    printf("Ingrese el grado (letra): ");
    scanf(" %c", &nuevoEstudiante.grado);
    printf("Ingrese el promedio de calificacion: ");
    scanf("%f", &nuevoEstudiante.promedio_calificacion);

    // Si la lista esta vacia, se crea el primer nodo
    if (*lista == NULL)
    {
        *lista = crear_lista_de_estudiantes(&nuevoEstudiante);
        if (*lista == NULL)
        {
            fprintf(stderr, "Error al crear la lista.\n");
        }
        else
        {
            printf("Estudiante agregado correctamente (primer nodo).\n");
        }
    }
    else
    {
        agregar_estudiante(*lista, &nuevoEstudiante);
        printf("Estudiante agregado correctamente.\n");
    }
}

/***************************************************************************
 * Funcion: eliminarEstudianteMenu
 *
 * Descripcion:
 *  Solicita la CI del estudiante a eliminar y llama a la funcion correspondiente.
 *
 * Parametros:
 *  nodo_estudiante_t **lista - Puntero a la direccion de la lista enlazada.
 ***************************************************************************/
void eliminarEstudianteMenu(nodo_estudiante_t **lista)
{
    char ci_a_eliminar[9];
    printf("\nIngrese la CI del estudiante a eliminar: ");
    scanf(" %8s", ci_a_eliminar);
    eliminar_estudiante(lista, ci_a_eliminar);
}

/***************************************************************************
 * Funcion: mostrarEstudiantesMenu
 *
 * Descripcion:
 *  Solicita al usuario el atributo (filtro) a mostrar y llama a la funcion
 *  de mostrar estudiantes con el filtro seleccionado.
 *
 * Parametros:
 *  nodo_estudiante_t *lista - Puntero a la lista enlazada de estudiantes.
 ***************************************************************************/
void mostrarEstudiantesMenu(nodo_estudiante_t *lista)
{
    int filtro;
    printf("\nSeleccione el atributo a mostrar:\n");
    printf("1. Nombre\n");
    printf("2. Apellido\n");
    printf("3. CI\n");
    printf("4. Grado\n");
    printf("5. Promedio de Calificacion\n");
    printf("6. Mostrar toda la informacion\n");
    printf("Ingrese opcion: ");

    if (scanf("%d", &filtro) != 1)
    {
        fprintf(stderr, "Entrada invalida.\n");
        return;
    }

    // Seleccion de filtro
    switch (filtro)
    {
    case 1:
        mostrar_estudiantes(lista, NOMBRE);
        break;
    case 2:
        mostrar_estudiantes(lista, APELLIDO);
        break;
    case 3:
        mostrar_estudiantes(lista, CI);
        break;
    case 4:
        mostrar_estudiantes(lista, GRADO);
        break;
    case 5:
        mostrar_estudiantes(lista, PROMEDIO_CALIFICACION);
        break;
    case 6:
        // Se pasa un valor fuera del enum para que se muestre toda la informacion (usa el caso default)
        mostrar_estudiantes(lista, -1);
        break;
    default:
        printf("Opcion no valida.\n");
        break;
    }
}
