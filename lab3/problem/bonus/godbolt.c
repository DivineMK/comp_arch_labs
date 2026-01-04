/* Type your code here, or load an example. */
#include <stdint.h>
#include <stdio.h>
#define LIG

#define min(a, b) (a < b) ? a : b
#define max(a, b) (a > b) ? a : b

typedef struct {
    uint8_t r, g, b;
} pixel_t;

int square(pixel_t *A, unsigned int nr_elements) {
    uint8_t grayscale, max_p, min_p;
    for (unsigned int i = 0; i < nr_elements; i++) {
#ifdef LIG
    min_p = min(min(A[i].r, A[i].g), A[i].b);
    max_p = max(max(A[i].r, A[i].g), A[i].b);
    grayscale = (min_p + max_p) >> 1;
#elifdef AVG
    grayscale =
        (uint32_t)((A[i].r + A[i].g + A[i].b) * 342) >> 10; // 1/3=342>>10
#elifdef LUM
    grayscale = ((uint32_t)(307 * A[i].r) + (uint32_t)(604 * A[i].g) +
                 (uint32_t)(113 * A[i].b)) >>
                10;
#else
    printf("Invalid grayscale method\r\n");
    return 1;
#endif
    A[i].r = A[i].g = A[i].b = grayscale;
  }
  return 0;
}
