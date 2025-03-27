#include <stdio.h>
#include <stdint.h>

void hello_world(void)
{
    printf("Hello world!");
}

void size_of_types(void)
{
    printf("Size of int8_t: %zu\n", sizeof(int8_t));
    printf("Size of int16_t: %zu\n", sizeof(int16_t));
    printf("Size of int32_t: %lu\n", (unsigned long)sizeof(int32_t));
    printf("Size of int64_t: %lu\n", (unsigned long)sizeof(int64_t));
    printf("Size of uint8_t: %zu\n", sizeof(uint8_t));
    printf("Size of uint16_t: %zu\n", sizeof(uint16_t));
    printf("Size of uint32_t: %zu\n", sizeof(uint32_t));
    printf("Size of uint64_t: %zu\n", sizeof(uint64_t));
}

void intercambiar_elementos_punteros(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void contar_vocales(char *cadena)
{
    int vocales = 0;
    int i = 0;
    
    while (cadena[i] != '\0')
    {
        if (cadena[i] == 'a' || cadena[i] == 'e' || cadena[i] == 'i' || cadena[i] == 'o' || cadena[i] == 'u')
        {
            vocales++;
        }
        i++;
    }

    printf("La cadena tiene %d vocales\n", vocales);
    printf("La cadena tiene %d consonantes\n", i - vocales);
}

// para imprimir una cadena de caracteres al revés utilizando un puntero.
void imprimir_cadena_al_reves(char *cadena)
{
    int i = 0;
    while (cadena[i] != '\0')
    {
        i++;
    }
    i--;
    while (i >= 0)
    {
        printf("%c", cadena[i]);
        i--;
    }
    printf("\n");
}

int length(char *cadena)
{
    int i = 0;
    while (cadena[i] != '\0')
    {
        i++;
    }

    return i;
}

int main()
{
    contar_vocales("hola mundo");
    printf("%i", length("Buen dia"));

    return 1;
}
