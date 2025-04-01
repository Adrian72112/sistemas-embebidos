#include <stdio.h>
#include <stdint.h> // uso esta porque cuenta los 32 bit que devuelve la palabra

int32_t string_words(char *string);

int main() {
    char text[] = "Hola como estas.";
    printf("Número de palabras: %d\n", string_words(text));
    return 0;
}
int32_t string_words(char *string) {
    int32_t contador = 0;
    int enpalabra = 0; // 0 si está fuera de una palabra, 1 si está dentro.

    while (*string) { // mientras no lleguemos al final del string
        if (*string != ' ' && *string != '\n') {  // distno a espacio, distino a enter
            if (!enpalabra) { // 
                contador++;    // 
                enpalabra = 1; //
            }
        } else {
            enpalabra = 0; // cuando no estamos en palbra osea estoy en espacio
        }
        string++; // pasamos al siguiente carácter
    }
    return contador;
}

