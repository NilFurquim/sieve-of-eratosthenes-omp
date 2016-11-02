//
// NOTE: This program is intended to be compiled on Linux with gcc.
//
// REQUIRED FLAGS:
//  -fopenmp:   Enable OpenMP directives and linking
//  -lm:        Link with libmath
//
// OPTIONAL FLAGS:
//  -DSIEVE_EXTENDED:   Use integers the size of this platform's pointer instead of fixed 32bits
//  -DSIEVE_FAST:       Compile with a faster sieve parallel algorithm   
//
// RECOMMENDED:
//  - For the (exercise) slower version:
//      gcc sieve.c -lm -fopenmp -Wall -Wextra -Werror -std=c99 -o sieve
//  - For the faster method:
//      gcc sieve.c -DSIEVE_FAST -lm -fopenmp -Wall -Wextra -Werror -std=c99 -o sieve 


#define _XOPEN_SOURCE 700
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//
// NOTE: Usage message is up here to help understanding how to run this application
//
void ShowUsage(char* Cmd) {
    // getopts "qtn:l:s:h"
    #define INDENT "  "
    #define OPTION INDENT "%-20s"
    fprintf(stderr, "\nUSAGE: %s [OPTIONS] MAX\n\n"
            "WARNING:\n" INDENT 
            "MAX is the highest number to look for primes. IT SHOULD BE ALWAYS AT THE END.\n\n"
            "OPTIONS:\n"
            OPTION "Shows this help message\n"
            OPTION "Hint to OpenMP the number of threads\n"
            OPTION "Print the time taken by the Sieve algorithm\n"
            OPTION "Number of results to print per line\n"
            OPTION "Separator for results on the same line (usually between single quotes)\n"
            OPTION "Skip printing the results (intended for performance measuring)\n\n"
            "EXAMPLES:\n" 
            INDENT "%s -n 4 -t 409600\n"
            INDENT "%s -n 1 -l 2 -s \', \' 819200\n",
            Cmd, "-h", "-n", "-t", "-l nums-per-line", "-s \'separator\'", "-q", 
            Cmd, Cmd);

    #undef OPTION
    #undef INDENT
}

//
// NOTE: Fixed width boolean and integer types
//
typedef uint8_t b8;
#define true 1
#define false 0

typedef uint32_t u32;
#define PrintU32 PRIu32
#define MAX_U32 UINT32_MAX

//
// NOTE: When compiling with SIEVE_EXTENDED, all sieve calculations will
//       be performed with an integer type that can index the biggest array
//       this platform can theoretically make. Although allocation limits
//       are usually the bottleneck.
//
//       If SIEVE_EXTENDED is not set, all sieve calculations will be on
//       unsigned 32bit integer
//
#ifdef SIEVE_EXTENDED
#define sieve_num size_t
#define PrintSieveNum "zu"  // No ideal portable version on inttypes.h :(
#define MAX_SIEVE SIZE_MAX
#else
#define sieve_num u32
#define PrintSieveNum PrintU32
#define MAX_SIEVE MAX_U32
#endif

//
// NOTE: String to unsigned integer convertion function
//
uintmax_t StringToUMax(char* Str, uintmax_t MaxVal);
#define StringToSieveNum(Str) (sieve_num) StringToUMax(Str, MAX_SIEVE)

//
// NOTE: AUTOFREE macro ensures freeing memory the variable points to 
//       once it goes out of scope. Only meant to be used with dynamic memory
//       (Compiling with gcc is assumed here for brevity).       
//
void _Autofree(void* Ptr);
#define AUTOFREE(Ptr) __attribute__((cleanup(_Autofree))) Ptr = NULL

//
// NOTE: Input arguments
//
b8 ShouldPrintPrimes = true;
b8 ShouldPrintTime = false;
u32 PrimesPerLine = 10;
char* Separator = "\t";
sieve_num MaxPrime;
b8 ParseArguments(int NumberOfArgs, char** Args);

// Results printing function
void PrintPrimes(b8* IsComposite);

int main(int NumberOfArgs, char** Args) {
    
    if(!ParseArguments(NumberOfArgs, Args)) {
        ShowUsage(Args[0]);
        return EXIT_FAILURE;        
    }

    //
    // NOTE: Sieve table allocation:
    //       - We allocate 1 extra element to include check on MaxPrime input
    //       - Memory, more then performance, seems to be the limiting factor for high values
    //
    sieve_num ElementsToAllocate = MaxPrime + 1;

    b8* AUTOFREE(IsComposite);
    IsComposite = (b8*)calloc((size_t)ElementsToAllocate, sizeof(b8));

    if (IsComposite == 0 || errno == ENOMEM) {
        fprintf(stderr,
                "Problem allocating memory, try looking for lower maximum "
                "values\n");
        return EXIT_FAILURE;
    }

    sieve_num MaxPrimeSqrt = (sieve_num)(sqrtf((float)MaxPrime) + 1e-5f /* <- bias*/);

    // Start the sieve timer
    double TimeBeforeSieve = omp_get_wtime();

    //
    // NOTE: Perform the sieve algorithm. 2 Parallelization possibilites:
    //       - (Default) Slower one parallelizing the outer loop. It is
    //         wasteful because previous iterations are not necessarily 
    //         done, so it is possible for the inner loop to be run for
    //         additional (composite) numbers, that would make no effect.
    //
    //       - A faster process is to parallelize the inner loop, it will
    //         mark composites faster on itself but most importantly, it 
    //         would only be run for prime numbers, achieving the result
    //         with the fewest number of runs intended.
    //         (This one is used when compiling with SIEVE_FAST defined).    
    //
    #ifndef SIEVE_FAST
    #pragma omp parallel for firstprivate(MaxPrimeSqrt) firstprivate(IsComposite) schedule(static)
    #endif
    for (sieve_num Number = 2; Number <= MaxPrimeSqrt; Number++) {
        if (!IsComposite[Number]) {
            #ifdef SIEVE_FAST
            #pragma omp parallel for firstprivate(Number) schedule(static)
            #endif
            for (sieve_num Composite = Number * Number; Composite <= MaxPrime;
                 Composite += Number) {
                IsComposite[Composite] = true;
            }
        }
    }

    // End and print the sieve timer
    double TimeAfterSieve = omp_get_wtime();
    if(ShouldPrintTime) {
        printf("Sieve took %.5f seconds.\n", TimeAfterSieve - TimeBeforeSieve);
    }

    if (ShouldPrintPrimes) {
        PrintPrimes(IsComposite);
    }
    

    return EXIT_SUCCESS;
}

void PrintPrimes(b8* IsComposite) {
    u32 PrimeInLine = 0;
    for (sieve_num Number = 2; Number <= MaxPrime; Number++) {
            if (!IsComposite[Number]) {
                PrimeInLine = (PrimeInLine + 1) % PrimesPerLine;
                char* LocalSeparator = (PrimeInLine == 0) ? "\n" : Separator;
                printf("%" PrintSieveNum "%s", Number, LocalSeparator);
            }
    }
        
    // Append empty line if missing  
    if(PrimeInLine > 0) {
        printf("\n");
    }
}

uintmax_t StringToUMax(char* Str, uintmax_t MaxVal) {
    uintmax_t Val = strtoumax(Str, NULL, 10);
    if (Val > MaxVal) {
        Val = MaxVal;
        errno = ERANGE;
    }

    return Val;
}

b8 ParseArguments(int NumberOfArgs, char** Args) {
    extern char *optarg;
    extern int optopt, optind;
    int Flag;
    
    while((Flag = getopt(NumberOfArgs, Args, "qtn:l:s:h")) != -1) {
        switch(Flag) {

            // -q (quiet) Skip printing primes, for performance measuring purposes
            case 'q': {
                ShouldPrintPrimes = false;
            } break;

            // -t (timer) if enabled will print sieve calculation time
            case 't': {
                ShouldPrintTime = true;
            } break;

            // -n (number) Number of threads, this is a hint for OpenMP
            case 'n': {
                int NumThreads = atoi(optarg);
                if(NumThreads < 1) {
                    fprintf(stderr, "The number of threads must be greater or equal to 1\n");
                    return false;
                }
                    omp_set_num_threads(NumThreads);
            } break;

            // -l (line) Number of primes to print per line
            case 'l': {
                PrimesPerLine = (u32)StringToUMax(optarg, MAX_U32);
                if(errno == ERANGE) {
                    fprintf(stderr, "-l option needs a positive integer value\n");
                }
            } break;

            // -s (separator) character used to separate primes on the same line
            case 's': {
                Separator = optarg; //TODO: Improve safety
            } break;

            // -h (help) Show usage message
            case 'h': {
                return false;
            } break;

            // ':' missing value for argument
            case ':': {
                fprintf(stderr, "Option -%c requires a value\n", optopt);
                return false;
            } break;

            // '?' Option unknown
            case '?': {
                // fprintf(stderr, "Unrecognized option: -%c\n", (char) optopt);
                return false;
            } break;
        }
    }

    if (optind < NumberOfArgs) {
         MaxPrime = StringToSieveNum(Args[optind]);
        if (errno == EINVAL) *((b8*)0) = 0;
        if (errno == ERANGE || MaxPrime == MAX_SIEVE) 
        {
            // 
            // NOTE: Passed value is over our limit (MAX_SIEVE-1)
            // We will cap it and warn the user
            // 
            MaxPrime = MAX_SIEVE - 1;  
            fprintf(stderr,
                    "%s is higher than we can handle, "
                    "looking to a max of %" PrintSieveNum " instead\n",
                    Args[1], MaxPrime);
        } 
        return true;
    } else {
        fprintf(stderr, "Missing MAX value at the end of command\n");
    }

    return false;
}

void _Autofree(void* Ptr) { void** _Ptr = Ptr; if (*_Ptr) free(*_Ptr); }