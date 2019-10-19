#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("N00b\n");
    }
    int iterations = atoi(argv[1]);
    int sum = 0;
    for (int i = iterations; i--; i > 0)
    {
        sum+=i;
    }
    printf("The sum of all numbers 1-%d is %d \n", iterations, sum);
}