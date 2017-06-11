
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "prng.h"

typedef struct gauss_fitting_s {
    double sigma;
    double mu;

    double X1;
    double X2;

    unsigned long seed;
    prng_t ng;
    int call;

} gauss_fitting_t;


void* gauss_fitting_create(double*x, int nx)
{
    double mean = 0.0;
    double stddev = 0.0;

    int i;

    for(i=0; i<nx; ++i)
    {
        mean += x[i];
    }
    mean /= nx;

    for(i=0; i<nx; ++i)
    {
        stddev += pow(x[i]-mean, 2.0);
    }
    stddev = sqrt( stddev / (nx-1) );

    gauss_fitting_t* gf = (gauss_fitting_t*)malloc(sizeof(gauss_fitting_t));
    gf->sigma = stddev;
    gf->mu = mean;
    gf->ng = prng_init_seed(gf->seed);
    gf->call = 0;
    //printf("sigma %lf\tmu %lf\n", stddev, mean);

    return gf;

}

void gauss_fitting_free(void* pv)
{
    gauss_fitting_t* gf = (gauss_fitting_t*)pv;
    free(gf->ng);
    free(gf);
}

double gauss_fitting_next(void* pv)
{
    gauss_fitting_t* gf = (gauss_fitting_t*)pv;
    //double rt;
    //double ep = -pow(x- gf->mu, 2)/(2*pow(gf->sigma, 2) );
    // 1.0/(gf->sigma*sqrt(2*M_PI) )
    //double p = M_2_SQRTPI * M_SQRT1_2  / (2.0*gf->sigma);
    //rt = p *exp( ep );

    double U1;
    double U2;
    double W;
    double mult;


    if (gf->call == 1)
    {
        gf->call = !gf->call;
        return (gf->mu + gf->sigma * gf->X2);
    }

    do
    {
        U1 = -1 + prng_next(gf->ng) * 2;
        U2 = -1 + prng_next(gf->ng) * 2;
        W = pow (U1, 2) + pow (U2, 2);
    }
    while (W >= 1 || W == 0);

    mult = sqrt ((-2 * log (W)) / W);
    gf->X1 = U1 * mult;
    gf->X2 = U2 * mult;

    gf->call = !gf->call;

    return (gf->mu + gf->sigma * gf->X1);

}

void gauss_fitting_init_seed(void* pv, unsigned long seed)
{
    gauss_fitting_t* gf = (gauss_fitting_t*)pv;
    gf->seed = seed;
    free(gf->ng);
    gf->ng = prng_init_seed(seed);
}
