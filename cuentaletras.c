#include <stdio.h>

int cuentaletras(char *palabra) {
    int contador = 0;
    
    for (int i = 0; palabra[i] != '\0'; i++) { // recorre la cadena hasta encontrar el carácter nulo '\0'
        contador++; 
    }
    
    return contador;
}

int main() {
    char palabra[] = "holaaaa";  
    int longitud = cuentaletras(palabra);
    printf("La longitud del string es: %d\n", longitud);

    return 0;
}
