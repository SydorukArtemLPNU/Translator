#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(void) {
    int16_t a;
    int16_t b;
    int16_t maxval;
    float x;
    float y;

    a = 15;
    b = 25;
    if (!((a > b))) goto label_else_0;
    maxval = a;
    goto label_end_1;

label_else_0:
    maxval = b;

label_end_1:
    x = 10.5;
    y = 20.3;
    if (!(((x + y) > 30.0))) goto label_else_2;
    printf("l0\n");
    goto label_end_3;

label_else_2:
    printf("l1\n");

label_end_3:
    if (!((a == b))) goto label_end_4;
    printf("l2\n");

label_end_4:
    printf("%d\n", (int)(maxval));
    return 0;
}
