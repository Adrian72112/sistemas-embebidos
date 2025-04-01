#include <stdio.h>
#include <string.h>

char* reverse_string(char *string) {
    int len = strlen(string);
    int principio = 0;
    int fin = len - 1;

    while (principio < fin) {
        // intercambiar los caracteres
        char temp = string[principio];
        string[principio] = string[fin];
        string[fin] = temp;
        principio++;
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
