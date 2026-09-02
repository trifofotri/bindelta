#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    int total = 0;
    for (int i = 0; i < 8; i++) {          // changed: 5 -> 8
        total = add(total, i);
    }

    if (total > 20) {                       // changed: 10 -> 20
        printf("big: %d\n", total);
    } else {
        printf("small: %d\n", total);
    }

    return 0;
}