#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//
// NOTE: Type definitions
//
typedef uint8_t b8;
#define true 1
#define false 0

typedef uint32_t u32;
#define PrintU32 PRIu32
#define MAX_U32 UINT32_MAX

#ifdef SIEVE_EXTENDED
#define sieve_num size_t
#define PrintSieveNum "zu"  // No ideal portable version on inttypes :(
#define MAX_SIEVE SIZE_MAX
#else
#define sieve_num u32
#define PrintSieveNum PrintU32
#define MAX_SIEVE MAX_U32
#endif

uintmax_t StringToUMax(char* Str, uintmax_t MaxVal) {
    uintmax_t Val = strtoumax(Str, NULL, 10);
    if (Val > MaxVal) {
        Val = MaxVal;
        errno = ERANGE;
    }
    //
    // NOTE: In case Val > UINTMAX_MAX, errno is already set to ERANGE,
    //       there's no need for additional checks
    //

    return Val;
}
#define StringToSieveNum(Str) (sieve_num) StringToUMax(Str, MAX_SIEVE)

//
// NOTE: Scope based free (assuming gcc here)
//
void _Autofree(void* Ptr) {
    void** _Ptr = Ptr;
    if (*_Ptr)
        free(*_Ptr);
}
#define AUTOFREE(Ptr) __attribute__((cleanup(_Autofree))) Ptr = NULL

void ShowUsage(char* Cmd) {
    fprintf(stderr, "USAGE: %s <max>\n", Cmd);
}

int main(int NumberOfArgs, char** Args) {
    //
    // NOTE: Parsing arguments
    //
    if (NumberOfArgs != 2) {
        ShowUsage(Args[0]);
        return EXIT_FAILURE;
    }

    sieve_num Max = StringToSieveNum(Args[1]);
    // if (errno == EINVAL) *((b8*)0) = 0;
    if (errno == ERANGE || Max == MAX_SIEVE) 
    {
        // 
        // NOTE: Passed value is over our limit (MAX_SIEVE-1)
        // We will cap it and warn the user
        // 
        Max = MAX_SIEVE - 1;  
        fprintf(stderr,
                "%s is higher than we can handle, "
                "looking to a max of %" PrintSieveNum " instead\n",
                Args[1], Max);
    }
    // Biased float conversion seems safe until 64bits integer
    sieve_num MaxSqrt = (sieve_num)(sqrtf((float)Max) + 1e-5f);

    //
    // NOTE: Sieve table allocation
    //
    sieve_num ElementsToAllocate = Max + 1;
    b8* AUTOFREE(IsComposite);
    IsComposite = (b8*)calloc((size_t)ElementsToAllocate, sizeof(b8));
    if (IsComposite == 0 || errno == ENOMEM) {
        fprintf(stderr,
                "Problem allocating memory, try looking for lower maximum "
                "values\n");
        return EXIT_FAILURE;
    }

    //
    // NOTE: Sieve algorithm
    //
    #ifndef SIEVE_FAST
    #pragma omp parallel for shared(MaxSqrt) shared(IsComposite) schedule(static)
    #endif
    for (sieve_num Number = 2; Number <= MaxSqrt; Number++) {
        if (!IsComposite[Number]) {
            #ifdef SIEVE_FAST
            #pragma omp parallel for firstprivate(Number) schedule(static)
            #endif
            for (sieve_num Composite = Number * Number; Composite <= Max;
                 Composite += Number) {
                IsComposite[Composite] = true;
            }
        }
    }
#if 0
    //
    // NOTE: Output
    //
    u32 PrimeInLine = 0;
    const u32 PrimesPerLine = 12;
    for (sieve_num Number = 2; Number <= Max; Number++) {
        if (!IsComposite[Number]) {
            PrimeInLine = (PrimeInLine + 1) % PrimesPerLine;
            char* Separator = (PrimeInLine == 0) ? "\n" : ", ";
            printf("%" PrintSieveNum "%s", Number, Separator);
        }
    }

    // Append empty line if missing  
    if(PrimeInLine > 0) {
        printf("\n");
    }
#endif
    return EXIT_SUCCESS;
}