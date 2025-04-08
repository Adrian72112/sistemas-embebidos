
#include <stdio.h>
#include <stdint.h>  // Necesario para int32_t

int32_t string_length(char *string) {
    int contador = 0;

    // Corrección: Incrementar contador dentro del for
    for (; *string != '\0'; string++) {
        contador++;
    }

    return contador;
}

int main() {
    char palabra[] = "hola";
    
    // Corrección: Llamar a la función y luego imprimir el resultado
    printf("La longitud del string es: %d\n", string_length(palabra));

    return 0;
}

/*
#include <stdio.h>

int contiene(char *palabra, char *palabra2) {//*palabra2, valor al que apunta el puntero palabra2.
    for (; *palabra2 != '\0'; palabra2++) {  // Recorremos palabra2 con punteros
        if (*palabra2 == *palabra) {  // Si encontramos el primer carácter de palabra./compara los valores
            char *p1 = palabra;//*p1 apunta al valor donde comienza la subcadena. es decir donde comienzan a coincidir
            char *p2 = palabra2;//*p2 apunta a la posicion actual en palabra2

            while (*p1 != '\0' && *p2 == *p1) {  // mientras *p1 no sea nulo y *p2 sea igual a *p1 sumo los punteros. 
                p1++;
                p2++;
            }

            if (*p1 == '\0') { // Si llegamos al final de 'palabra', significa que la encontramos
                return 1;
            }
        }
    }
    return 0; // Si no encontramos la palabra, retornamos 0
}

int main() {
    char palabra[] = "ola";
    char palabra2[] = "paola";

    if (contiene(palabra, palabra2)) {
        printf("La palabra '%s' está dentro de '%s'.\n", palabra, palabra2);
    } else {
        printf("La palabra '%s' no está dentro de '%s'.\n", palabra, palabra2);
    }

    return 0;
}

*/















