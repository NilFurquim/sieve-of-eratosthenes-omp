#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

void print_primes(char *nums_array, long max_prime)
{
	int i;
	for(i = 2; i <= max_prime; i++)
	{
		if(nums_array[i] == 0)
		{
			printf("%d\n", i);
		}
	}
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("missing arg");
		return 1;
	}
	long max_prime = atoi(argv[1]);
	char *nums_array = (char *) calloc(max_prime + 1, sizeof(char));

	long i;
	for(i = 2; i <= max_prime; i++)
	{
		if (nums_array[i] == 0)
		{
			long j;
			#pragma omp parallel for schedule(static)
			for(j = i*i; j <= max_prime; j += i)
			{
//				printf("%d\n", omp_get_thread_num());
				nums_array[j] = 1;
			}
		}
	}

//	print_primes(nums_array, max_prime);
	return 0;
}
