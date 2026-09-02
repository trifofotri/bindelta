#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    int total = 0;
    for (int i = 0; i < 5; i++) {
        total = add(total, i);
    }

    if (total > 10) {
        printf("big: %d\n", total);
    } else {
        printf("small: %d\n", total);
    }

    return 0;
}