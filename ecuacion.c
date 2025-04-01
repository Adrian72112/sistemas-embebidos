#include <stdio.h>
#include <math.h> //calcular potencias

int main() {
    double a, b, c;  // Declaramos las variables
    double dentroraiz, x1, x2;  
    printf ("ingrese el valor de a");
    scanf("%lld", &a);
    printf ("ingrese el valor de b");
    scanf("%lld", &b);
    printf ("ingrese el valor de c");
    scanf("%lld", &c);
    #include <stdio.h>
    #include <math.h> // Para calcular raíces cuadradas
    
    if (a==0) {
        printf("no es de segundo grado");
    }
   
    dentroraiz= b*b -4*a*c;
        if (dentroraiz > 0) {
            x1 = (-b + sqrt(dentroraiz)) / (2 * a);
            x2 = (-b - sqrt(dentroraiz)) / (2 * a);
            printf("Las soluciones son: x1 = %.2lf y x2 = %.2lf\n", x1, x2);
        } 
        return 0;
    }
    