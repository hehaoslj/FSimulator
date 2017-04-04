#ifndef PRNG_H
#define PRNG_H
#ifdef __cplusplus
extern "C" {
#endif

typedef  void* prng_t;
prng_t prng_init_seed(unsigned long seed);
void prng_free(prng_t pv);
double prng_next(prng_t pv);

#ifdef __cplusplus
}
#endif

#endif /** PRNG_H */
