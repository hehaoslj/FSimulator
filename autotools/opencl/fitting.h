#ifndef EXP_FITTING_H
#define EXP_FITTING_H

typedef void* gauss_fitting_t;
gauss_fitting_t gauss_fitting_create(double*x, int nx);
void gauss_fitting_init_seed(gauss_fitting_t pv, unsigned long seed);
void gauss_fitting_free(gauss_fitting_t pv);
double gauss_fitting_next(gauss_fitting_t pv);
#endif /** EXP_FITTING_H */
