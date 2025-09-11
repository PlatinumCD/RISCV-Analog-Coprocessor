#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
int k[2] = {0, 0};
omp_set_num_threads(2);
#pragma omp parallel num_threads(2) shared(k)
{
int tid = omp_get_thread_num();
k[tid] = tid;
}

printf("%d %d\n", k[0], k[1]);
}
