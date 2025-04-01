#include <stdio.h>
#include "my_lib_1.h"

int main()
{
    //funcion string_to_min t string_to_caps
    char oracion[256];
    int opcion;
/*
    printf("Ingrese una oración: ");
    fgets(oracion, sizeof(oracion), stdin);

    printf("Seleccione una opción:\n");
    printf("1. Convertir a MAYÚSCULAS\n");
    printf("2. Convertir a minúsculas\n");
    printf("Opción: ");
    scanf("%d", &opcion);

    if (opcion == 1) {
        string_to_caps(oracion);
    } else if (opcion == 2) {
        string_to_min(oracion);
    } else {
        printf("Opción inválida.\n");
        return 1;
    }

    printf("Resultado: %s\n", oracion);
*/
    //Funcion complex_t
    /*   complex_t num1, num2, resultado;
   
    printf("Ingrese la parte real del primer número complejo: ");
    scanf("%f", &num1.real);
    printf("Ingrese la parte imaginaria del primer número complejo: ");
    scanf("%f", &num1.imag);

    printf("Ingrese la parte real del segundo número complejo: ");
    scanf("%f", &num2.real);
    printf("Ingrese la parte imaginaria del segundo número complejo: ");
    scanf("%f", &num2.imag);
  
    printf("La suma es: %.2f + %.2fi\n", resultado.real, resultado.imag);
    return 0;

resultado = prod(num1, num2);
printf("El producto es: %.2f + %.2fi\n", resultado.real, resultado.imag);
*/
date_t date1, date2;

printf("Ingrese la primera fecha (dd mm aaaa): ");
scanf("%d %d %d", &date1.dia, &date1.mes, &date1.anio);

printf("Ingrese la segunda fecha (dd mm aaaa): ");
scanf("%d %d %d", &date2.dia, &date2.mes, &date2.anio);

int diferencia = days_left(date1, date2);

printf("La diferencia entre las fechas es de %d días.\n", diferencia);

return 0;

}
