#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

uint8_t ilog2_test(uint32_t val) {
  uint8_t res = 1;
  while ((1 << res) < val)
    res++;
  return res;
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < 33; i++)
    printf("%d:%u\r\n", i, ilog2_test(i));

  return 0;
}
