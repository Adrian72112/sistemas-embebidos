#include <stdio.h>
#include <string.h>

char* reverse_string(char *string) {
    char *inicio = string;                   // puntero al primer carácter
    char *fin = string + strlen(string) - 1; // puntero al último carácter

    while (inicio < fin) {
        char temp = *inicio; //intercambio
        *inicio = *fin;
        *fin = temp;

        inicio++; // avanzar puntero al siguiente carácter
        fin--;    // retroceder puntero al anterior carácter
    }
    return string;
}

int main() {
    char str[] = "como estas?";
    printf("original: %s\n", str);
    printf("invertida: %s\n", reverse_string(str));
    return 0;
}

