#include <stdlib.h>
#include <math.h>

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UMASK 0x80000000UL /* most significant w-r bits */
#define LMASK 0x7fffffffUL /* least significant r bits */
#define MIXBITS(u,v) ( ((u) & UMASK) | ((v) & LMASK) )
#define TWIST(u,v) ((MIXBITS(u,v) >> 1) ^ ((v)&1UL ? MATRIX_A : 0UL))

typedef struct mt19937_in_s {
unsigned long state[N]; /* the array for the state vector  */
int left;
int initf;
unsigned long *next;
}mt19937_in;



typedef struct mt19937_in_s* mt19937;

typedef struct gauss_rand_s {
    mt19937 mt1;
    mt19937 mt2;
} gauss_rand_in;

typedef gauss_rand_in* gauss_rand;

/* initializes state[N] with a seed */
static inline mt19937 init_genrand(unsigned long s)
{
    mt19937 in = (mt19937)malloc(sizeof(struct mt19937_in_s));
    int j;
    in->state[0]= s & 0xffffffffUL;
    for (j=1; j<N; j++) {
        in->state[j] = (1812433253UL * (in->state[j-1] ^ (in->state[j-1] >> 30)) + j);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array state[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        in->state[j] &= 0xffffffffUL;  /* for >32 bit machines */
    }
    in->left = 1;
    in->initf = 1;

    return in;
}

static inline void next_state(mt19937 pv)
{
    unsigned long *p=pv->state;
    int j;

    /* if init_genrand() has not been called, */
    /* a default initial seed is used         */
    if (pv->initf==0) init_genrand(5489UL);

    pv->left = N;
    pv->next = pv->state;

    for (j=N-M+1; --j; p++)
        *p = p[M] ^ TWIST(p[0], p[1]);

    for (j=M; --j; p++)
        *p = p[M-N] ^ TWIST(p[0], p[1]);

    *p = p[M-N] ^ TWIST(p[0], pv->state[0]);
}

/* generates a random number on [0,0xffffffff]-interval */
static inline unsigned long genrand_int32(mt19937 pv)
{
    unsigned long y;

    if (--pv->left == 0) next_state(pv);
    y = *pv->next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}


/* generates a random number on [0,1]-real-interval */
static inline double genrand_real1(mt19937 pv)
{
    unsigned long y;

    if (--pv->left == 0) next_state(pv);
    y = *pv->next++;

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return (double)y * (1.0/4294967295.0);
    /* divided by 2^32-1 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
static inline double genrand_res53(mt19937 pv)
{
    unsigned long a=genrand_int32(pv)>>5, b=genrand_int32(pv)>>6;
    return(a*67108864.0+b)*(1.0/9007199254740992.0);
}

void* prng_init_seed(unsigned long seed)
{
    mt19937 in = init_genrand(seed);
    return in;
}

void prng_free(void* pv)
{
    free(pv);
}

double prng_next(void* pv)
{
    mt19937 in = (mt19937)pv;
    return genrand_res53(in);
}

void* gauss_init_seed(unsigned long seed)
{
    gauss_rand in = (gauss_rand)malloc(sizeof(gauss_rand_in));
    in->mt1 = init_genrand(seed);
    in->mt2 = init_genrand(seed*2+1);
    return in;
}
void gauss_free(void* pv)
{
    gauss_rand in = (gauss_rand)pv;
    free(in->mt1);
    free(in->mt2);
    free(pv);
}

double gauss_next(void* pv)
{
    gauss_rand in = (gauss_rand)pv;
    double u = genrand_res53(in->mt1);
    double v = genrand_res53(in->mt2);
    return sqrt(-2.0 * log(u))* sin(2.0 * M_PI * v);
}
