#include <mex.h>

static inline void add_matrix(double* m1, double* m2, int n, int m)
{
    int i;
    int j;
    for(i=0; i < n; ++i)
    {
        for(j=0; j<m; ++j)
        {
            m1[i*m+j] += m2[i*m+j];
        }
    }
}

void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )
{
    mexPrintf("left = %d, right = %d\n", nlhs, nrhs);
}