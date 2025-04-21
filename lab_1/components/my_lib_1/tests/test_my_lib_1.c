
#include "my_lib_1.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

void test_eq_solver()
{
    coeff_t c = {1, -3, 2}; // x^2 - 3x + 2 = 0, raíces: 2 y 1
    root_t r = eq_solver(&c);
    assert(r.num_roots == 2);
    assert((int)r.x1 == 2 || (int)r.x1 == 1);
    assert((int)r.x2 == 2 || (int)r.x2 == 1);
}

void test_bin2dec()
{
    assert(bin2dec(1010, false) == 10);
    assert(bin2dec(110, true) == -6);
}

void test_string_length()
{
    assert(string_length("hola") == 4);
    assert(string_length("") == 0);
    assert(string_length(NULL) == -1);
}

void test_string_words()
{
    assert(string_words("Hola mundo") == 2);
    assert(string_words("   Hola   mundo   ") == 2);
    assert(string_words("") == 0);
    assert(string_words(NULL) == 0);
}

void test_consonantes_y_vocales()
{
    assert(consonantes("Hola") == 2); // H, l
    assert(vocales("Hola") == 2);     // o, a
    assert(consonantes("") == 0);
    assert(vocales(NULL) == 0);
}

void test_string_copy()
{
    char dest[100];
    assert(string_copy("Hola", dest) == 0);
    assert(strcmp(dest, "Hola") == 0);
    assert(string_copy(NULL, dest) == -1);
}

void test_find_in_string()
{
    assert(find_in_string("hola mundo", "mundo") == 5);
    assert(find_in_string("hola mundo", "xyz") == -1);
}

void test_swap()
{
    int a = 5, b = 10;
    assert(swap(&a, &b, sizeof(int)) == 0);
    assert(a == 10 && b == 5);
}

int main()
{
    test_eq_solver();
    test_bin2dec();
    test_string_length();
    test_string_words();
    test_consonantes_y_vocales();
    test_string_copy();
    test_find_in_string();
    test_swap();

    printf("✅ Todos los tests pasaron correctamente.\n");
    return 0;
}
