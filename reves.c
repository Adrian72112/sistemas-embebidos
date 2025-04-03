#include <stdio.h>
#include <string.h>

char* reverse_string(char *string) {
    char *inicio = string;                   
    char *fin = string + strlen(string) - 1;

    while (inicio < fin) {
        char temp = *inicio; 
        *inicio = *fin;
        *fin = temp;

        inicio++; 
        fin--;    
    }
    return string;
}

int main() {
    char str[] = "como estas?";
    printf("original: %s\n", str);
    printf("invertida: %s\n", reverse_string(str));
    return 0;
}

