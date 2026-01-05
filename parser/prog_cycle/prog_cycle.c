#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

int main(void) {
    int16_t i;
    int16_t sum;
    int16_t arr[5];
    float prod;

    sum = 0;
    prod = 1;

label_repeat_0:
    sum = (sum + i);
    prod = (prod * 2);
    i = (i + 1);
    if ((i > 5)) goto label_repeat_end_1;
    goto label_repeat_0;

label_repeat_end_1:
    i = 0;

label_repeat_2:
    arr[i] = (i * 2);
    i = (i + 1);
    if ((i >= 5)) goto label_repeat_end_3;
    goto label_repeat_2;

label_repeat_end_3:
    printf("%d\n", (int)(sum));
    printf("%f\n", prod);
    printf("%d\n", (int)(arr[0]));
    printf("%d\n", (int)(arr[4]));
    printf("l0\n");
    return 0;
}
