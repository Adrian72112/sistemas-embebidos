#include <stdio.h>
#include <math.h> //calcular potencias 

int main() {
    
}

int ecuacion(float a, float b, float c) {

    double dentroraiz, x1, x2;  
    
    if (a==0) {
        printf("no es de segundo grado");
        return 0;
    }

dentroraiz= b*b -4*a*c;
    if (dentroraiz > 0) {
        x1 = (-b + sqrt(dentroraiz)) / (2 * a);
        x2 = (-b - sqrt(dentroraiz)) / (2 * a);
        printf("Las soluciones son: x1 = %.2lf y x2 = %.2lf\n", x1, x2);
    } 
    return 0;
}
