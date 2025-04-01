#include <stdio.h>
#include <math.h> //calcular potencias

int main() {
    long long binario;  // Usamos long long para aceptar números grandes
    int decimal = 0, potencia = 0, digito;

    // Pedir el número binario al usuario
    printf("Ingrese un numero binario");
    scanf("%lld", &binario); // "%lld" Lee un número ingresado por el usuario y lo guarda en la variable binario

    // Convertir a decimal
    while (binario > 0) {
        digito = binario % 10;   // Obtener el último dígito (0 o 1)
        decimal += digito * pow(2, potencia);  // Sumar su valor en decimal
        binario /= 10;  // Eliminar el último dígito
        potencia++;      // Aumentar la potencia de 2
    }

    // Mostrar resultado
    printf("El equivalente decimal es: %d\n", decimal);

    return 0;
}
