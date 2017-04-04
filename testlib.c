#if !(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "autotools/clibapp.h"
//#include "PyPy.h"
char* guess_pypyhome(void) ;
extern double do_stuff3(int,int);

extern double do_stuff4(int x, int y);

extern void guavatask(const char* fn);

extern void test();

extern int fc_calc(const char* cfg);

extern int forecastertask(const char*);

extern void optimizetask(const char*);

extern const char* genopencltask(const char*);

int main(int argc, char* argv[])
{
//    double rt = 0;
//    int i=0;

//    rpython_startup_code();
//    //char* home = guess_pypyhome();
//    //printf("pypy home is %s\n", home);
//    char *home = (char*)malloc(64);
//    memset(home, 0, 64);
//    strcpy(home, "/usr/lib64/pypy-5.0.1/lib_pypy");
//    strcpy(home, "/usr/lib64/libpypy-c.so");
//    printf("home is %s\n", home);

//    pypy_setup_home(home, 1);

//    if(argc == 1) {
//        rt = do_stuff4(3,4);
//    } else {
//        rt = do_stuff3(3,4);
//    }
//    printf("result is %lf\n", rt);


    //guavatask(argv[1]);
//    if(argc > 2)
//

    if(argc >1) {
//        forecastertask(argv[1]);
//        optimizetask(argv[1]);
        printf("%s\n", genopencltask(argv[1]) );
    }

    //if(argc == 2)
     //   fc_calc(argv[1]);
//    forecaster_t fc;
//    fc= fc_create("rb_0");
//    const char* code;
//    fc_get_trading_instrument(fc, &code);
    //printf("code is %s\n", code);
    return 0;
}




////caller should free returned pointer to avoid memleaks
//// returns NULL on error
//char* guess_pypyhome(void) {
//    // glibc-only (dladdr is why we #define _GNU_SOURCE)
//    Dl_info info;
//    void *_rpython_startup_code = dlsym(0,"rpython_startup_code");
//    if (_rpython_startup_code == 0) {
//        return 0;
//    }
//    if (dladdr(_rpython_startup_code, &info) != 0) {
//        const char* lib_path = info.dli_fname;
//        char* lib_realpath = realpath(lib_path, 0);
//        return lib_realpath;
//    }
//    return 0;
//}
