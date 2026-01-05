#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(void) {
    int16_t x;
    int16_t y;
    int16_t z;
    float res;

    x = 10;
    y = 20;
    z = (x + y);
    res = (z * 2);
    printf("%d\n", (int)(x));
    printf("%d\n", (int)(y));
    printf("%d\n", (int)(z));
    printf("%f\n", res);
    printf("l0\n");
    return 0;
}
