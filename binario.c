#include <stdio.h>
#include <math.h> 

int main() {
    int pipipopo=1010;
    int resultado = binario(pipipopo);
    
}
    
int binario (int binario) {
    int decimal = 0, potencia = 0, digito;
    scanf("%lld", &binario); // "%lld" lee un número ingresado por el usuario y lo guarda en la variable binario

    while (binario > 0) {
        digito = binario % 10;  
        decimal += digito * pow(2, potencia);  // sumar su valor en decimal
        binario /= 10;  
        potencia++;      
    }

    printf("El equivalente decimal es: %d\n", decimal);
    return 0;
}
