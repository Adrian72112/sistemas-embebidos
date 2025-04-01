#include <stdio.h>

int main() {
    char palabra[] = "hola";  
    int contador = 0;

    for (int i = 0; palabra[i] != '\0'; i++) 
        contador++; 
    

    printf("La longitud del string es: %d\n", contador);

    return 0;
}
