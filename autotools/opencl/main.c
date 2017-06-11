#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/** OpenCL */
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#include <sys/mman.h>
#include <mach/vm_statistics.h>
#elif defined(__linux__)
#include <CL/opencl.h>
#include <sys/mman.h>
#endif

#include "config.h"
#include "prng.h"
#include "fitting.h"

#include <locale.h>
#include <pthread.h>

#include "lmice_trace.h" /*EAL library */
#include "lmice_eal_time.h"

/*****************************************************************************/
/** Global definition
*/
extern const char* genopencltask(const char*);
#define PROP_TYPE float
#define SIG_TYPE float

#define SIG_VALUE fsig_data

#define GPU_DEBUG

typedef struct _ChinaL1Msg
{
    uint64_t m_inst;
    int64_t m_time_micro; /* time in epoch micro seconds */
    double m_bid;
    double m_offer;
    int32_t m_bid_quantity;
    int32_t m_offer_quantity;
    int32_t m_volume;
    int32_t m_reserved_0; /* Padding only, no use*/
    double m_notional;
    double m_limit_up;
    double m_limit_down;
}__attribute__((aligned(8))) ChinaL1Msg;

typedef struct _OutputMsg
{
    int m_order;
    float m_result;
}OutputMsg;


/*****************************************************************************/
/** Helper functions
*/

static inline void print_usage(void)
{
    printf("Signal Simulation App\n"
           "Usage:\n"
           "myopencl -argn1 argv1 --argn2=argv2...>\n"
           "\t-h,--help\t\tPrint this message.\n"
           "\t-d,--device=d\t\tChoose calculation device.\n"
           "\t-v,--verify=d\t\tVerify [0--n]highest data.\n"
           "\t-sfs,--simfitsim=d\t\tDo Simulation - Datafitting - Simulation.\n"
           "\t-i,--input=s\t\tChoose config file.\n");
}

static inline void print_platform_info(cl_platform_id platform_id, int pos)
{
    char buff[256];
    clGetPlatformInfo(platform_id,
                      CL_PLATFORM_VENDOR,
                      sizeof(buff),
                      buff,
                      NULL);
    printf("platform[%d] Vendor: %s", pos, buff);
    clGetPlatformInfo(platform_id,
                      CL_PLATFORM_VERSION,
                      sizeof(buff),
                      buff,
                      NULL);
    printf(" Version: %s ", buff);
    clGetPlatformInfo(platform_id,
                      CL_PLATFORM_NAME,
                      sizeof(buff),
                      buff,
                      NULL);
    printf("Name: %s\n", buff);
}

static inline void print_device_info(cl_device_id device_id, int pos)
{
    char buff[256];
    clGetDeviceInfo(device_id,
                    CL_DEVICE_VENDOR,
                    sizeof(buff),
                    buff,
                    NULL);
    printf("device[%d] Vendor: %s", pos, buff);
    clGetDeviceInfo(device_id,
                    CL_DEVICE_VERSION,
                    sizeof(buff),
                    buff,
                    NULL);
    printf("\tVersion: %s ", buff);
    clGetDeviceInfo(device_id,
                    CL_DEVICE_NAME,
                    sizeof(buff),
                    buff,
                    NULL);
    printf("\tName: %s\n", buff);
}

static inline void replace_str(char* buff, const char* sep, const char* rep, size_t rep_len)
{
    char *bpos;
    size_t len;
    size_t len2;
    size_t len3;

    if(!buff)
        return;

    len = strlen(sep);
    len2 = strlen(buff);

    do {
        bpos = strstr(buff, sep);
        if(bpos)
        {
            len3 = strlen(bpos+len);
            memcpy(bpos+rep_len, bpos+len, len3);
            memcpy(bpos, rep, rep_len);
            buff[len2 - len + rep_len] = '\0';

        }
    } while(bpos);
}

static inline int proc_cmdline(int argc, char** argv, int* dev_id, const char** cfg_file,
                               int *verify_pos, int *sfs_mode)
{
    int i;
    for(i=0; i<argc; ++i)
    {
        const char* cmd = argv[i];
        if(strcmp(cmd, "-h") ==0 ||
                strcmp(cmd, "--help") == 0)
        {
            print_usage();
            return 0;
        }
        else if(strcmp(cmd, "-d") == 0 ||
                strcmp(cmd, "--device") == 0)
        {
            if(i+1 < argc)
            {
                cmd = argv[i+1];
                *dev_id = atoi(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require module name\n", cmd);
                return 0;
            }
        }
        else if(strcmp(cmd, "-v") == 0 ||
                strcmp(cmd, "--verify") == 0)
        {
            if(i+1 < argc)
            {
                cmd = argv[i+1];
                *verify_pos = atoi(cmd);
            }
            else
            {
                lmice_error_print("Command(%s) require module name\n", cmd);
                return 0;
            }
        }
        else if(strncmp(cmd, "--device=", 9) == 0)
        {
            cmd += 9;
            *dev_id = atoi(cmd);
        }
        else if(strcmp(cmd, "-i") == 0 ||
                strcmp(cmd, "--input") == 0)
        {
            if(i+1 < argc)
            {
                cmd = argv[i+1];
                *cfg_file = cmd;
            }
            else
            {
                lmice_error_print("Command(%s) require input file name\n", cmd);
                return 0;
            }
        }
        else if(strncmp(cmd, "--input=",8) == 0)
        {
            cmd += 8;
            *cfg_file = cmd;
        }
        else if(strcmp(cmd, "-sfs") == 0 ||
                strcmp(cmd, "--simfitsim") == 0)
        {
            *sfs_mode = 1;
        }
    }
    return 0;
}

static inline int load_kernel_from_source(const char* cfg_file, cl_context context, cl_device_id device_id, cl_program* pp)
{
    cl_int err;
    const char* kernel_src;
    cl_program program;

    // Generate Kernel source
    lmice_warning_print("gen opencl task...\n");
    kernel_src = genopencltask(cfg_file);
    lmice_warning_print("create program with source...\n");
    {
        FILE* fp = fopen("temp.cl", "w");
        fprintf(fp, "%s", kernel_src);
        fclose(fp);
    }

    /// Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char **) & kernel_src, NULL, &err);
    if (!program)
    {
        lmice_error_print("Failed to create compute program from source\n");
        return EXIT_FAILURE;
    }
    lmice_warning_print("Build the program executable...");
    /// Build the program executable
    err = clBuildProgram(program, 1, &device_id, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[8192*32];

        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        lmice_error_print("Failed to build program executable[%d]\n%s", err, buffer);
        return EXIT_FAILURE;
    }

    *pp = program;

    return err;
}

static inline int load_kernel_from_binary(const char* bin_name, const char* cfg_file, cl_context context, cl_device_id device_id, cl_program* pp)
{
    cl_int err;
    size_t size;
    unsigned char src[8192*512];
    const unsigned char *psrc[1];
    cl_program program;
    psrc[0] = src;
    memset(src, 0, 8192*512);
    FILE* fp = fopen(bin_name, "rb");

    lmice_warning_print("fp = %p\n", fp);
    if(!fp)
    {
        err = load_kernel_from_source(cfg_file, context, device_id, &program);
        if(err != CL_SUCCESS)
            return EXIT_FAILURE;

        err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
                               sizeof(size_t),
                               &size, NULL);
        if(err != CL_SUCCESS)
        {
            lmice_error_print("Failed to get program info[%d]\n", err);
            return EXIT_FAILURE;
        }
        if(size > 8192*512)
        {
            lmice_error_print("Failed to get program binary too big[%lu]\n", size);
            return EXIT_FAILURE;
        }
        //src[0] = (unsigned char*)malloc(size);
        err = clGetProgramInfo(program, CL_PROGRAM_BINARIES,
                               sizeof(char*),
                               (void*)psrc, NULL);
        if(err != CL_SUCCESS)
        {
            lmice_error_print("Failed to get program binary[%d]\n", err);
            return EXIT_FAILURE;
        }

        fp = fopen(bin_name, "wb");
        fwrite(src, 1, size, fp);
        fclose(fp);

    }
    else
    {
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        //src[0] = (unsigned char*)malloc(size);
        if(size > 8192*512)
        {
            lmice_error_print("Failed to get program binary too big[%lu]\n", size);
            return EXIT_FAILURE;
        }
        fread(src, 1, size, fp);
        fclose(fp);
        program = clCreateProgramWithBinary(context, 1, &device_id, &size, psrc, NULL, &err);
        if (!program)
        {
            lmice_error_print("Failed to create compute program from binary\n");
            return EXIT_FAILURE;
        }
        /// Build the program executable
        err = clBuildProgram(program, 1, &device_id, "-cl-std=CL1.2", NULL, NULL);
        if (err != CL_SUCCESS)
        {
            size_t len=0;
            char buffer[8192*32];

            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
            lmice_error_print("Failed to build program executable[%d]:\n%s", err, buffer);
            return EXIT_FAILURE;
        }
    }

    *pp = program;
    //free(src[0]);
    return err;

}

#define aligned_size(size, a) ((size + a-1)/a) *a

struct sig_count_param
{
    char file_name[256];
    int64_t prop_count;
    int64_t sig_count;
    int64_t msg_count;
};
static inline int get_sig_count(config_t cfg, struct sig_count_param** ppsc, int32_t* file_count)
{
    int count;
    int i, ic;
    int ret;
    char* bpos;
    const char* bsep = "__";
    const char* file;
    const char* pattern;
    const char* stype;
    const char* sdate;
    const char* stime;
    char *name;
    const char* outpath;
    struct sig_count_param* psc;
    struct stat st;

    stype = cfg_get_string(cfg, "insttype");
    outpath = cfg_get_string(cfg, "signal_calc.outpath");
    pattern = cfg_get_string(cfg, "signal_calc.pattern");
    count = cfg_get_list_size(cfg, "signal_calc.files");

    psc = (struct sig_count_param*)malloc( sizeof(struct sig_count_param) * count);
    memset(psc, 0, sizeof(struct sig_count_param) * count);

    *file_count = count;

    lmice_info_print("Signal files count = %d\n", count);
    for(i=0; i<count; ++i)
    {
        struct sig_count_param* scur = psc+i;
        name = scur->file_name;

        file = cfg_get_list_string(cfg, "signal_calc.files", i);
        if(!file)
        {
            lmice_error_print("signal_calc files([string,...]) is incorrect\n");
            return EXIT_FAILURE;
        }

        lmice_critical_print("file = %s\n", file);

        bpos = strstr(file, bsep);
        sdate = bpos -10;
        stime = bpos+2;

        strcat(name, outpath);
        strcat(name, "/");
        strcat(name, pattern);
        strcat(name, ".idx");

        replace_str(name, "%type", stype, strlen(stype));
        replace_str(name, "%date", sdate, 10);
        replace_str(name, "%time", stime, 8);

        int brep_count = cfg_get_list_size(cfg, "optimizer.clreps");
        for(ic=0; ic<brep_count/2; ++ic)
        {
            /** Check index file is existing */
            ret = stat(name, &st);
            if(ret == 0) {
                break;
            }

            const char* srep = cfg_get_list_string(cfg, "optimizer.clreps", ic*2);
            const char* drep = cfg_get_list_string(cfg, "optimizer.clreps", ic*2+1);
            replace_str(name, srep, drep, strlen(drep));
        }


        {
            /// Read prop_count from index file
            long pc;
            long sc;
            long mc;
            config_t idx;

            //strcat(name, ".idx");
            idx = cfg_init_file(name);
            if(!idx)
            {
                lmice_error_print("cannot open index file %s\n", name);
                return EXIT_FAILURE;
            }
            pc = cfg_get_integer(idx,   "size");
            lmice_info_print("sub signals AKA prop_count[%s] size %ld\n", name, pc);
            cfg_close(idx);

            // Reset name extension; the number 4 is strlen('.idx')
            name[strlen(name)-4] = '\0';

            /// Read signal from forecast file

            strcat(name, ".fc");
            stat(name, &st);
            sc = st.st_size;
            if( (sc % (sizeof(double)*pc) ) != 0 || sc == 0)
            {
                lmice_error_print("forecast content is incorrect of size %ld %ld\n", sc, sc % (sizeof(double)*pc));
                return EXIT_FAILURE;
            }
            sc = sc / (sizeof(double)*pc);

            name[strlen(name)-3] = '\0';

            /// Read market data from market file
            strcat(name, ".dat");
            stat(name, &st);
            mc = st.st_size;
            if(mc % sizeof(ChinaL1Msg) != 0 || mc == 0)
            {
                lmice_error_print("market content is incorrect of size %ld %ld\n", mc, mc % sizeof(ChinaL1Msg));
                return EXIT_FAILURE;
            }
            mc = mc / sizeof(ChinaL1Msg);

            name[strlen(name)-4] = '\0';

            lmice_info_print("mc=%ld  sc=%ld pc=%ld\n", mc,
                             sc, pc);

            scur->msg_count = mc;
            scur->prop_count = pc;
            scur->sig_count = sc;
        }


    }

    *ppsc = psc;

    return 0;
}

struct host_side_param
{
    uint32_t prop_count;
    uint32_t prop_count_an;
    uint32_t prop_group;
};

void verify(
        const float* param_data, /* param_count/4 * param_group */
        const float* sig_data,   /* param_count/4 * msg_count */
        //__global const ChinaL1Msg* msg_data,/* msg_count */
        //__global const float3* mkt_data,    /* msg_count */
        OutputMsg* output,         /* param_group */
        float* debug_log,
        //const float g_signal_multiple,      /* initialied multiple */
        const int msg_count,                /* sizeof msg */
        const int param_count,              /* sizeof parameter */
        const int param_group,              /* sizeof param_data */
        const int device_type,              /* 0-CPU    1-GPU */
        const float* mkt_mid,    /* $:(clvec)-vectorized mid data */
        const float* mkt_ask,    /* $:(clvec)-vectorized ask data */
        const float* mkt_bid,     /* $:(clvec)-vectorized bid data */
        const float* last_ask,    /* last ask price every session */
        const float* last_bid,    /* last bid price every session */
        const int* trd_lv,
        const int* session_list,
        const int session_count);

/*****************************************************************************/
/** main function
  All objects/bulk datas are 8-bytes aligned
  */
int main(int argc, char** argv)
{
    int err;                            // error code returned from api calls

    /** Host side input/output data */
    size_t hb_size;
    void* host_bulk;
    OutputMsg *results;                 // results returned from device

    PROP_TYPE *prop_data = NULL;        // property data from PRNG (prop_count_an*prop_group)
    unsigned int prop_count;            // size of subsignals
    unsigned int prop_count_an;         // aligned size of subsignals (optimizer.aligned)
    unsigned int prop_group;            // each calc trial count(= global);
    float prop_multi = 1.0f;            // prng multiple
    long prop_trial;                    // total trial count
    long prop_seed;                     // prng seed
    long prop_pos;                      // prng number position

    double *sig_data = NULL;                   // sub signals data from .fc file
    float *fsig_data = NULL;                   // sub signals data float version
    unsigned int sig_count;             // sub signals data size

    ChinaL1Msg* msg_data = NULL;               // message data from .dat file
    unsigned int msg_count;             // message data size
    float* mkt_data = NULL;                    // Market data[mid, ask, bid]
    float* mkt_mid = NULL;                     // Market mid data
    float* mkt_ask = NULL;                     // Market ask data
    float* mkt_bid = NULL;                     // Market bid data
    int* trd_lv = NULL;                        // trade success or not
#ifdef GPU_DEBUG    
	float *debug_log = NULL;
#endif
	float *last_ask = NULL;
    float *last_bid = NULL;

    cl_mem cl_trd_lv = NULL;

    /** Config param */
    config_t cfg;                       // Config object
    float* highest_data = NULL;                // Output highest data
    float* lowest_data = NULL;                 // Output lowest data
    int highest;                        // size of highest data
    int lowest;                         // size of lowest data

    int* ses_data= NULL; //session list

    /** OpenCL param */
    size_t global = 65536;              // global domain size for our calculation
    size_t local=1;                       // local domain size for our calculation

    /** Device side param */
    cl_mem cl_output = NULL;                   // results

#ifdef  GPU_DEBUG
	cl_mem cl_debug = NULL;
#endif

    cl_mem cl_prop_data = NULL;                // property data
    cl_mem cl_msg_data = NULL;                 // message data
    cl_mem cl_sig_data = NULL;                 // sub signals data
    cl_mem cl_mkt_data = NULL;                 // Market data [mid, ask, bid]
    cl_mem cl_mid_data = NULL;                 // Market mid data
    cl_mem cl_ask_data = NULL;                 // Market ask data
    cl_mem cl_bid_data = NULL;                 // Market bid data
    cl_mem cl_ses_data = NULL;                 // Session list data
	cl_mem cl_last_ask = NULL;                 // last ask price every session 
	cl_mem cl_last_bid = NULL;                 // last bid price every session

    cl_uint platforms;
    cl_uint device_num = 0;

    cl_platform_id platform_id[4];
    cl_device_id device_ids[16];
    cl_device_id  device_id=NULL;             // compute device id pointed to->device_ids[dev_id]


    cl_context context = NULL;                 // compute context
    cl_command_queue commands = NULL;          // compute command queue
    cl_program program = NULL;                 // compute program
    cl_kernel kernel = NULL;                   // compute kernel

    /** Command-line param */
    int dev_id = -1;                    // Device id
    const char* cfg_file=NULL;          // Config file
    const char* svalue = NULL;          // Config string value
    bool        bvalue = false;         // Config boolean value
    int bverify = -1;
    int bsfs_mode = -1;      //do sim-fitting-sim
    gauss_fitting_t* dfitting;


    long i = 0;
    long j = 0;

    //setlocale(LC_TIME, "zh_CN.UTF-8");

    /** Process command line */
    err = proc_cmdline(argc, argv, &dev_id, &cfg_file, &bverify, &bsfs_mode);
    if(err != 0)
        return err;

    /** Load config object */
    cfg=cfg_init_file(cfg_file);
    if(!cfg)
    {
        lmice_error_print("Config file cannot open [%s]\n", cfg_file);
        return 1;
    }

    /** Set thread name linux(centos7.2) may crash */
#if defined(__MACH__)
    svalue = cfg_get_string(cfg, "conftype");
    if(svalue)
        pthread_setname_np(svalue);
#endif

    lmice_info_print("Calc instument type=%s\n", cfg_get_string(cfg, "insttype"));



    /** Calc environment initialization */
    // Get Platform id
    clGetPlatformIDs(0,0,&platforms);
    lmice_info_print("OpenCL platform count=%d\n", platforms);
    if(platforms > 4) platforms = 4;
    //platform_id = (cl_platform_id*)malloc(platforms * sizeof(cl_platform_id));
    clGetPlatformIDs (platforms, platform_id, NULL);

    // Print device info
    for(i=0; i< platforms; ++i)
    {
        print_platform_info(platform_id[i], i);
        err = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_ALL, 0, 0, &device_num);
        if(device_num > 16) device_num = 16;
        //device_ids = (cl_device_id *)malloc(sizeof(cl_device_id)*device_num);
        err = clGetDeviceIDs(platform_id[i],CL_DEVICE_TYPE_ALL,device_num,&device_ids[0],NULL);

        printf("device num %u\n", device_num);
        for(j=0; j< device_num; ++j)
        {
            printf("\t");
            print_device_info(device_ids[j], j);
        }
    }


    // Load default dev_id if not set
    if(dev_id == -1)
    {
        dev_id = cfg_get_integer(cfg, "optimier.deviceid");
    }

    /** Initialized device */
    if (dev_id >= device_num)
    {
        lmice_error_print("Failed to create[device id too big %d > %d]!!\n", dev_id, device_num);
        return EXIT_FAILURE;
    }

    // Print current device info
    device_id = device_ids[dev_id];
    print_device_info(device_id, dev_id);

    // Create a compute context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context)
    {
        lmice_error_print("Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }

    // Gen kernel source
    bvalue = cfg_get_bool(cfg, "optimizer.clbuild");
    if(bvalue)
    {
        const char* name;
        char bin_name[512];
        name = cfg_get_string(cfg, "optimizer.clfile");
        if(strlen(name) > 504)
        {
            lmice_error_print("Failed to use optimizer,clfile, long than 504\n");
            return EXIT_FAILURE;
        }
        memset(bin_name, 0, 512);
        strncpy(bin_name, name, strlen(name));
        sprintf(bin_name + strlen(bin_name), ".%d", dev_id);

        lmice_warning_print("load kernel from binary (%s)\n", bin_name);
        err = load_kernel_from_binary(bin_name, cfg_file, context, device_id, &program);
    }
    else
    {

        err = load_kernel_from_source(cfg_file, context, device_id, &program);
    }
    if(err != CL_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    // Create the compute kernel in the program we wish to run
    const char* clfunc = cfg_get_string(cfg, "optimizer.clfunc");
    kernel = clCreateKernel(program, clfunc, &err);
    if (!kernel || err != CL_SUCCESS)
    {
        lmice_error_print("Failed to create compute kernel!\n");
        return EXIT_FAILURE;
    }

    // Get the maximum work group size for executing the kernel on the device
    err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if (err != CL_SUCCESS)
    {
        lmice_error_print("Error: Failed to retrieve kernel work group info! %d\n", err);
        return EXIT_FAILURE;
    }
    //local = 128;

    int64_t pc, sc,mc;
    int32_t fc;
    struct sig_count_param* psc;
    get_sig_count(cfg, &psc, &fc);
    lmice_info_print("count files=%d\n", fc);
    for(i=0; i<fc; ++i)
    {
        struct sig_count_param* scur = psc+i;
        pc = scur->prop_count;
        sc = scur->sig_count;
        mc = scur->msg_count;
        if(mc == 0 || sc == 0 || pc == 0)
        {
            lmice_error_print("invalid state in file[%s]\n", scur->file_name);
            return EXIT_FAILURE;
        }
        lmice_info_print("file=%s\n", scur->file_name);
		
	//	printf("== prop:%d - sig:%d - msg:%d ==\n", pc, sc, mc);
    }


    /** Create the host side input and output */
    {


        lmice_info_print("Create the host side input and output\n");
        //        const char* pattern;
        //        const char* file;
        //        const char* stype;
        //        const char* sdate;
        //        const char* stime;
        //        char name[256] ={0};
        //        char* bpos;
        long sig_aligned = 1;


        //        int brep_count;
        //        const char* brep;
        //        const char* bsep = "__";

        size_t pd_size = 0; //(prop data size(bytes)
        size_t sg_size = 0; //signal data size(bytes)
        size_t fs_size = 0; //(float signal data size(bytes)
        size_t mm_size = 0; // market message size(bytes)
        size_t op_size = 0; //output size
        size_t hi_size = 0; //highest size
        size_t lo_size = 0; //lowest size

        int clvec_aligned=1;


        FILE* fp_sig;   //signal file
        FILE* fp_msg;   //market message file

        //        const char* outpath = cfg_get_string(cfg, "signal_calc.outpath");
        //        file = cfg_get_list_string(cfg, "signal_calc.files", 0);
        //        if(!file)
        //        {
        //            lmice_error_print("signal_calc files is missing\n");
        //            return EXIT_FAILURE;
        //        }

        //        bpos = strstr(file, bsep);
        //        sdate = bpos -10;
        //        stime = bpos+2;
        //        stype = cfg_get_string(cfg, "insttype");
        //        pattern = cfg_get_string(cfg, "signal_calc.pattern");
        //        strcat(name, outpath);
        //        strcat(name, "/");
        //        strcat(name, pattern);

        //        //printf("%s\n", buff);
        //        replace_str(name, "%type", stype, strlen(stype));
        //        //printf("%s\n", buff);
        //        replace_str(name, "%date", sdate, 10);
        //        //printf("%s\n", buff);
        //        replace_str(name, "%time", stime, 8);
        //        //printf("%s\n", buff);
        //        brep_count = cfg_get_list_size(cfg, "optimizer.clreps");
        //        printf("brep_count = %d\n", brep_count);
        //        for(i=0; i<brep_count/2; ++i)
        //        {
        //            bsep = cfg_get_list_string(cfg, "optimizer.clreps", i*2);
        //            brep = cfg_get_list_string(cfg, "optimizer.clreps", i*2+1);
        //            replace_str(name, bsep, brep, strlen(brep));
        //        }

        // Read prop_count from index file
        //        {
        //            config_t idx;
        //            strcat(name, ".idx");
        //            printf("nm = %s\n", name);
        //            idx = cfg_init_file(name);
        //            if(!idx)
        //            {
        //                lmice_error_print("cannot open index file %s\n", name);
        //                return EXIT_FAILURE;
        //            }
        //            prop_count = cfg_get_integer(idx,   "size");
        //            lmice_info_print("sub signals AKA prop_count size %u\n", prop_count);
        //            cfg_close(idx);

        //            // Reset name extension; the number 4 is strlen('.idx')
        //            name[strlen(name)-4] = '\0';
        //        }

        prop_count = pc;
        sig_aligned = cfg_get_integer(cfg,  "optimizer.aligned");
        prop_count_an = aligned_size(prop_count, sig_aligned);
        lmice_info_print("prop_count aligned AKA prop_count_an size %u\n", prop_count_an);

        prop_seed =     cfg_get_integer(cfg,    "optimizer.seed");
        prop_pos =      cfg_get_integer(cfg,    "optimizer.pos");
        prop_group =    cfg_get_integer(cfg,    "optimizer.group");
        prop_trial =    cfg_get_integer(cfg,    "optimizer.trial");
        
	// Calc prop_data size sizeof(float)*prop_count_an*prop_group
        pd_size = sizeof(PROP_TYPE)*prop_count_an*prop_group;

        // Read signal from forecast file
        //        strcat(name, ".fc");
        //        fp_sig = fopen(name, "rb");
        //        fseek(fp_sig, 0L, SEEK_END);
        //        sg_size = ftell(fp_sig);
        //        fseek(fp_sig, 0L, SEEK_SET);
        //        if(sg_size % (sizeof(double)*prop_count) != 0 || sg_size == 0)
        //        {
        //            lmice_error_print("forecast content is incorrect of size %ld %ld\n", sg_size, sg_size % (sizeof(double)*prop_count));
        //            return EXIT_FAILURE;
        //        }
        //        sig_count = sg_size / (sizeof(double)*prop_count);
        //        sg_size = sizeof(double)*prop_count_an*sig_count;

        clvec_aligned = cfg_get_integer(cfg, "optimizer.clvec");
        sig_count= 0;
        sg_size = 0;
        mm_size = 0;
        for(i=0;i<fc;++i)
        {
            int64_t an;
            struct sig_count_param *scur = psc+i;
            an = aligned_size(scur->sig_count, clvec_aligned);
            sig_count += an;
            sg_size += sizeof(double)*prop_count_an*an;

            an = aligned_size(scur->msg_count, clvec_aligned);
            mm_size += sizeof(ChinaL1Msg) * an;

        }


        // Generate float type signal data
        fs_size = sizeof(float)*prop_count_an*sig_count;


        // Read market data from market file
        //        strcpy(name+strlen(name)-3, ".dat");
        //        fp_msg = fopen(name, "rb");
        //        fseek(fp_msg, 0L, SEEK_END);
        //        mm_size = ftell(fp_msg);
        //        fseek(fp_msg, 0L, SEEK_SET);
        //        if(mm_size % sizeof(ChinaL1Msg) != 0 || mm_size == 0)
        //        {
        //            lmice_error_print("market content is incorrect of size %ld %ld\n", mm_size, mm_size % sizeof(ChinaL1Msg));
        //            return EXIT_FAILURE;
        //        }
        msg_count = mm_size / sizeof(ChinaL1Msg);


        // Generate market data vectorized


        // Get result data size
        highest = cfg_get_integer(cfg, "optimizer.highest");
        lowest =  cfg_get_integer(cfg, "optimizer.lowest");


        // Signal multiple
        double mult = cfg_get_real(cfg, "optimizer.sig_multiple");
        if(mult != 0)
            prop_multi = mult;

        // Output data
        op_size = sizeof(OutputMsg) * prop_group; //output
        //Highest size
        hi_size = (sizeof(OutputMsg) + sizeof(float) * prop_count)* highest;
        lo_size = (sizeof(OutputMsg) + sizeof(float) * prop_count)* lowest;

        /** Create memory bulk at host side*/
        hb_size =  pd_size + sg_size + fs_size + mm_size + sizeof(float)*3*msg_count*2 //mkt
                + op_size
                + hi_size
                +lo_size;
        hb_size += sizeof(int)*msg_count;

		//hb_size += fc*sizeof(float);
		//hb_size += fc*sizeof(float);

        lmice_info_print("host size %lu\n"
                         "\tpd:%lu\tsg:%lu\tfs:%lu\tmm:%lu\tsg:%u\n"
                         "\top:%lu\thi:%lu\tlo:%lu\tmc:%u\n", hb_size,
                         pd_size, sg_size, fs_size, mm_size, sig_count,
                         op_size, hi_size, lo_size
                         ,msg_count);

        hb_size = ((hb_size / (1024*1024*2)) + 1)*(1024*1024*2);
        host_bulk = mmap(NULL,
                         hb_size,
                         PROT_READ|PROT_WRITE,
                         MAP_ANON|MAP_PRIVATE,
                 #if defined(__MACH__)
                         VM_FLAGS_SUPERPAGE_SIZE_2MB,
                 #elif defined(__linux__)
                         MAP_HUGETLB,
                 #endif
                         0
                         );

        if(host_bulk == MAP_FAILED)
        {
            lmice_error_print("Create memory bulk by host super page size[%luMB] failed[%p]\n", hb_size/(1024*1024), host_bulk);
            host_bulk = mmap(NULL,
                             hb_size,
                             PROT_READ|PROT_WRITE,
                             MAP_ANON|MAP_PRIVATE,
                 #if defined(__MACH__)
                             SUPERPAGE_NONE,
                 #elif defined(__linux__)
                             MAP_HUGETLB,
                 #endif
                             0
                             );
            if(host_bulk == MAP_FAILED) {
                lmice_error_print("Create memory bulk by host size[%luMB] failed[%p]\n", hb_size/(1024*1024), host_bulk);
                perror(NULL);
                return EXIT_FAILURE;
            }
        }
		memset(host_bulk, 0, hb_size);

        // Init pointers
        prop_data = (float*)host_bulk;
        sig_data = (double*)((char*)host_bulk + pd_size);
        fsig_data = (float*)((char*)host_bulk + pd_size + sg_size);
        msg_data = (ChinaL1Msg*)((char*)host_bulk + pd_size + sg_size + fs_size);
        mkt_data = (float*)((char*)msg_data + mm_size);
        mkt_mid = (float*)((char*)mkt_data + sizeof(float)*3*msg_count);
        mkt_ask = (float*)((char*)mkt_mid + sizeof(float)*msg_count);
        mkt_bid = (float*)((char*)mkt_ask + sizeof(float)*msg_count);
        results = (OutputMsg*)((char*)mkt_bid+ sizeof(float)*msg_count);
        highest_data = (float*)((char*)results + sizeof(OutputMsg) * prop_group);
        lowest_data = (float*)((char*)highest_data + (sizeof(OutputMsg) + sizeof(float) * prop_count)* highest);
        trd_lv = (int*)((char*)lowest_data+(sizeof(OutputMsg) + sizeof(float) * prop_count)* lowest);
		
#ifdef GPU_DEBUG		
		debug_log = (float*)malloc(sizeof(float)*1024);
		memset( debug_log, 0, sizeof(float)*1024 );
#endif		
		last_ask = (float *)malloc(sizeof(float)*fc);
		memset( last_ask, 0, sizeof(float)*fc );
		last_bid = (float *)malloc(sizeof(float)*fc);
		memset( last_bid, 0, sizeof(float)*fc );

        //session data
        ses_data = (int*)malloc(sizeof(int)*fc);
        clvec_aligned = cfg_get_integer(cfg, "optimizer.clvec");
        for(i=0;i<fc;++i)
        {
            struct sig_count_param* scur=psc+i;
            ses_data[i] = aligned_size(scur->msg_count, clvec_aligned);
        }

        // Read Signal Data
        memset(sig_data, 0, sg_size);
        size_t pos = 0;
        char name[256];
        for(i=0; i<fc; ++i)
        {

            size_t rt = 0;
            struct sig_count_param *scur = psc+i;
            memcpy(name, scur->file_name, 256);
            strcat(name, ".fc");
			printf("======================= fopen:%s\n", name);
            fp_sig = fopen(name, "rb");
            for(;;)
            {
                rt = fread(sig_data+pos*prop_count_an, sizeof(double)*prop_count, 1, fp_sig);

/*				
				static int ppp = 0;
				if( ppp <= 999 )
				{
					double *ttt = sig_data+pos*prop_count_an;
					printf("sig- %lf,%lf,%lf,%lf,%lf,%lf\n",  *ttt, *(ttt+1), *(ttt+2), *(ttt+3), *(ttt+4), *(ttt+5) );
				}
*/				
				
                if(rt == 0)
                {
                    break;
                }
                ++pos;
                if(pos > sig_count)
                {
                    lmice_error_print("read sig data failed too large\n");
                    return E2BIG;
                }
            }
            fclose(fp_sig);
            pos = aligned_size(pos, clvec_aligned);
        }

        // Construct signal data (float version)
        memset(fsig_data, 0, fs_size);
        for(i=0; i<sig_count; ++i)
        {
            for(j=0; j< prop_count; ++j)
            {
                fsig_data[i*prop_count_an+j] = (float)sig_data[i*prop_count_an+j];
            }
        }

        // Read market message data
        pos = 0;
        for(i=0; i<fc; ++i)
        {
            size_t rt=0;
            struct sig_count_param *scur = psc+i;
            memcpy(name, scur->file_name, 256);
            strcat(name, ".dat");
            fp_msg = fopen(name, "rb");
            do {
                rt = fread(msg_data+pos, sizeof(ChinaL1Msg), 1, fp_msg);
                //printf("rt = %lu pos = %lu\n", rt ,pos);
                //printf("%f-%f\n", msg_data[pos].m_bid, msg_data[pos].m_offer);
                if(rt != 1)
                    break;
                pos ++;
            } while(rt != 0);
			last_ask[i] = msg_data[pos-1].m_offer;
			last_bid[i] = msg_data[pos-1].m_bid;
			printf("==%f-%f==\n", last_bid[i], last_ask[i]);
            pos = aligned_size(pos, clvec_aligned);
            printf("return %lu\n", pos);
            fclose(fp_msg);
            printf("reading ...%s, pos=%lu\n", name, pos);
        }

        // Construct market data(vectorized version)
        for(i=0; i<msg_count; ++i)
        {
            ChinaL1Msg* pc = msg_data+i;
            float* mkt = mkt_data+i*3;
            *mkt = (float)((pc->m_bid+pc->m_offer)*0.5);
            *(mkt+1) = (float)(pc->m_offer);
            *(mkt+2) = (float)(pc->m_bid);
            *(mkt_mid+i) = *(mkt+0);
            *(mkt_ask+i) = *(mkt+1);
            *(mkt_bid+i) = *(mkt+2);
            //printf("%lf\n", pc->m_offer);
        }

//        pintf("pos :16000, sig:\n");
//        for(i=0; i<prop_count_an; ++i)
//        {
//            printf("sig[i]=%lf\t", sig_data[16000*prop_count_an+i]);
//        }
//        for(i=0; i<prop_count_an; ++i)
//        {
//            printf("ask[i]=%lf\t", mkt_ask[16000+i]);
//        }
//        printf("\n");

        // Init result data
        memset(highest_data, 0, (sizeof(OutputMsg) + sizeof(float) * prop_count)* highest);
        memset(lowest_data, 'c',  (sizeof(OutputMsg) + sizeof(float) * prop_count)* lowest);

    }

    prng_t mt_trd = prng_init_seed(prop_seed);
    prng_t mt19937 = prng_init_seed(prop_seed);
    //gauss_t gauss_rand = gauss_init_seed(prop_seed);
    gauss_t* gauss = (gauss_t*)malloc(sizeof(gauss_t)*prop_count);
    for(j=0; j<prop_count;++j)
    {
        gauss[j] = gauss_init_seed(prop_seed + j);
    }
    for(j=0; j<prop_pos; ++j)
    {
        prng_next(mt19937);
        prng_next(mt_trd);
    }

		printf( " prop_data_1:%f, sig_data_1:%lf, fsig_data_1:%f, msg_data_1:%lf-%lf, mkt_data_1:%f-%f-%f, mkt_mid:%f, mkt_ask:%f, mkt_bid:%f, results_1:%d\n", *prop_data, *sig_data, *fsig_data, msg_data->m_bid,  msg_data->m_offer, *mkt_data,
		*(mkt_data+1), *(mkt_data+2), *mkt_mid, *mkt_ask, *mkt_bid, results->m_order );
	
    if(bverify >= 0 && bverify < 1000)
    {
        int vpos = 0;
        FILE* fp = fopen("highest.csv", "r");
        char *line = NULL;
        size_t linecap = 0;
        ssize_t linelen;
        linelen = getline(&line, &linecap, fp);

        for(vpos =0; vpos <= bverify; ++vpos)
        {
            //verify mode
            linelen = getline(&line, &linecap, fp);

            float buf[128];
            char *test = line;
            char *sep = ",";
            char *word, *brkt;
            i=0;
            memset(buf, 0, sizeof(float)*128);
            printf("double test[]={");
            for (word = strtok_r(test, sep, &brkt);
                 word;
                 word = strtok_r(NULL, sep, &brkt))
            {
                buf[i] = atof(word);
                //printf("%s %f\n", word, buf[i]);
                if(i>=2 && i<prop_count+1)
                    printf("%s, ", word);
                if(i%4 == 0)
                    printf("\n");
                if(i == prop_count +1)
                    printf("%s};\n", word);
                i++;
            }

            for(i=0; i<prop_count_an; ++i)
            {
                prop_data[i] = buf[i+2];
                printf("%f\t", prop_data[i]);
            }
            printf("\n");

            for(j=0; j< msg_count; ++j)
            {
                trd_lv[j] = prng_next(mt_trd) < 0.4?1:0;
                //trd_lv[j] =1;
            }

            verify(prop_data,
                   fsig_data,
                   results,
                   debug_log,
                   msg_count,
                   prop_count_an,
                   prop_group,
                   0,
                   mkt_mid,
                   mkt_ask,
                   mkt_bid,
                   last_ask,
                   last_bid,
                   trd_lv,
                   ses_data,
                   fc);
			
            if(results[0].m_result>0)
            {
	            lmice_critical_print("output[%d] order:%d\tPL:+%f\n", vpos,
    	                             results[0].m_order, results[0].m_result);
            }
            else
            {
                lmice_critical_print("output[%d] order:%d\tPL:%f\n", vpos,
                                     results[0].m_order, results[0].m_result);
            }

        }
/*
		int res_pos = 0;
		for( res_pos=0; res_pos<=; res_pos++ )
		{
			printf("%d,%f\n", results[res_pos].m_order, results[res_pos].m_result);
		}*/
		
        return 0;
    }

    //simulation- datafitting -simulation

    if(bsfs_mode == 1)
    {
        size_t i;
        int row;
        int c;
        double *data;
        FILE* fp;
        lmice_info_print("Simulation fitting simulation mode %lu\n", sizeof(gauss_fitting_t));
        dfitting = (gauss_fitting_t*)malloc(sizeof(gauss_fitting_t)*prop_count);

        // Load highest data
        fp = fopen("highest.csv", "r");

        // Ignore the first line
        do {
            c = fgetc(fp);
            if(c == '\n')
                break;
        }while(c != EOF);

        for(row = 0; row<highest; ++row)
        {
            // Read order
            fscanf(fp, "%d", (int*)&highest_data[row*(2+prop_count) + 0]);

            for(i=0; i<prop_count+1; ++i)
            {
                fscanf(fp, ",%f", &highest_data[row*(2+prop_count) + i+1]);
            }
            //printf("h[%d]=%d", row, *(int*)&highest_data[row*(2+prop_count)] );
            //for(i=0; i<prop_count+1; ++i)
            //{
            //    printf(",%5.7f", highest_data[row*(2+prop_count) + i+1] );
            //}
            //printf("\n");
            // Ignore the new line char
            do {
                c = fgetc(fp);
                if(c == '\n')
                    break;
            }while(c != EOF);

        }

        fclose(fp);

        // Fitting
        data = (double*)malloc(sizeof(double)*highest);
        for(i=0; i<prop_count; ++i)
        {
            for(c=0; c<highest; ++c)
            {
                data[c] = highest_data[c*(2+prop_count) + i+2];
            }

            dfitting[i] = gauss_fitting_create(data, highest);
            gauss_fitting_init_seed(dfitting[i], prop_seed+i);

        }
        free(data);

        // Clean
        memset(highest_data, 0, (sizeof(OutputMsg) + sizeof(float) * prop_count)* highest);

        //return 0;
    }

    cl_ses_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * fc, NULL, NULL);

    cl_mid_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);
    cl_ask_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);
    cl_bid_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);

    /// Create the input and output arrays in device memory for our calculation
    cl_sig_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(SIG_TYPE)*prop_count_an*sig_count, NULL, NULL);
    //cl_msg_data = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(struct ChinaL1Msg) * msg_count, NULL, NULL);
    cl_prop_data = clCreateBuffer(context,  CL_MEM_READ_ONLY,  prop_count_an*sizeof(PROP_TYPE)*prop_group, NULL, NULL);
    //cl_mkt_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * msg_count * 3, NULL, NULL);
    cl_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(OutputMsg) * prop_group, NULL, NULL);

#ifdef GPU_DEBUG
	cl_debug = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 1024*sizeof(float), NULL, NULL);
	//printf("==%x==\n", cl_debug);
#endif 

	cl_last_ask = clCreateBuffer(context, CL_MEM_READ_ONLY, fc*sizeof(float), NULL, NULL);
	cl_last_bid = clCreateBuffer(context, CL_MEM_READ_ONLY, fc*sizeof(float), NULL, NULL);


    if (!cl_sig_data || !cl_prop_data || !cl_output)// || !cl_mkt_data)
    {
        lmice_error_print("Error: Failed to allocate device memory! sig=%p prop=%p output=%p mkt=%p prop_data=%p\n", cl_sig_data, cl_prop_data, cl_output, cl_mkt_data, prop_data);
        exit(1);
    }

    cl_trd_lv = clCreateBuffer(context,  CL_MEM_READ_ONLY,  msg_count*sizeof(int), NULL, NULL);

    //return 0;
    /// Set the arguments to our compute kernel
    err = 0;
    i=0;
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_prop_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_sig_data);
    //err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_mkt_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_output);
#ifdef GPU_DEBUG
	err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_debug);
	//printf("1======%d=======\n", err);
#endif
    //err |= clSetKernelArg(kernel, i++, sizeof(float), &prop_multi);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &msg_count);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &prop_count_an);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &prop_group);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &dev_id);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_mid_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_ask_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_bid_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_last_ask);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_last_bid);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_trd_lv);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_ses_data);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &fc);

	
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to set kernel arguments! %d\n", err);
        exit(1);
    }

    /// Create a command commands
    commands = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
    if (!commands)
    {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }


    err = clEnqueueWriteBuffer(commands, cl_ses_data, CL_TRUE, 0, sizeof(int) * fc, ses_data, 0, NULL, NULL);

    err = clEnqueueWriteBuffer(commands, cl_mid_data, CL_TRUE, 0, sizeof(float) * msg_count, mkt_mid, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(commands, cl_ask_data, CL_TRUE, 0, sizeof(float) * msg_count, mkt_ask, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(commands, cl_bid_data, CL_TRUE, 0, sizeof(float) * msg_count, mkt_bid, 0, NULL, NULL);

    err = clEnqueueWriteBuffer(commands, cl_last_ask, CL_TRUE, 0, sizeof(float) * fc, last_ask, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(commands, cl_last_bid, CL_TRUE, 0, sizeof(float) * fc, last_bid, 0, NULL, NULL);


    /// Write our data set into the input array in device memory
    err = clEnqueueWriteBuffer(commands, cl_sig_data, CL_TRUE, 0, sizeof(SIG_TYPE) * prop_count_an * sig_count, SIG_VALUE, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to write sig_data to source array!\n");
        exit(1);
    }

    //    err = clEnqueueWriteBuffer(commands, cl_msg_data, CL_TRUE, 0, sizeof(struct ChinaL1Msg) * msg_count, msg_data, 0, NULL, NULL);
    //    if (err != CL_SUCCESS)
    //    {
    //        printf("Error: Failed to write msg_data to source array!\n");
    //        exit(1);
    //    }

    //err = clEnqueueWriteBuffer(commands, cl_mkt_data, CL_TRUE, 0, sizeof(float) * 3 * msg_count, mkt_data, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to write mkt_data to source array!\n");
        exit(1);
    }


    int64_t tbegin;
    get_tick_count(&tbegin);
    //lmice_info_print("begin process\n");
    for(i=0; i<prop_trial; i += global)
    {

        // Update subsignals properties
        for(j=0; j < prop_group; ++j)
        {
            int pc_idx = 0;

            for(;pc_idx<prop_count; ++pc_idx)
            {
                double rt;

                //Simulation-fitting-simulation mode
                if(bsfs_mode==1)
                {
                    rt = gauss_fitting_next( dfitting[pc_idx]);
                }
                else
                {
                    //prop_data[j*prop_count_an + pc_idx] = prng_next(mt19937);
                    //prop_data[j*prop_count_an+pc_idx] = prop_multi * 2 *(prng_next(mt19937) - 0.5);
                   //rt = prop_multi * gauss_next(gauss_rand);
                   rt = prop_multi * gauss_next( gauss[pc_idx]);
                }
                //if(i==0 && j==0)
                //    printf("%lf\n",rt);

                prop_data[j*prop_count_an+pc_idx] = rt;
            }
        }
        for(j=0; j< msg_count; ++j)
        {
            trd_lv[j] = prng_next(mt_trd) < 0.4?1:0;
            //trd_lv[j] =1;
        }
        cl_event event;
        err = clEnqueueWriteBuffer(commands, cl_prop_data, CL_TRUE, 0, sizeof(PROP_TYPE) * prop_count_an * prop_group, prop_data, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            printf("Error: Failed to write prop_data to source array!\n");
            exit(1);
        }
        err = clEnqueueWriteBuffer(commands, cl_trd_lv, CL_TRUE, 0, sizeof(int) * msg_count, trd_lv, 0, NULL, NULL);


        // Execute the kernel over the entire range of our 1d input data set
        // using the maximum number of work group items for this device
        //
        global = prop_group;
        //local = 32;
        printf("process[%ld]:\tglobal=%lu  local=%lu\n", i, global, local);

        err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, &event);
        if (err)
        {
            printf("Error: Failed to execute kernel![%d]\n", err);
            return EXIT_FAILURE;
        }

        /// Wait for the command commands to get serviced before reading back results
        clFinish(commands);

        {
            cl_ulong start, end;
            clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                                    sizeof(cl_ulong), &end, NULL);
            clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                    sizeof(cl_ulong), &start, NULL);
            float executionTimeInMilliseconds = (end - start) * 1.0e-6f;
            lmice_info_print("calculate process finished prop_group-%u time(ms)-%f\n", prop_group,executionTimeInMilliseconds);
        }


        /// Read back the results from the device to verify the output
	err = clEnqueueReadBuffer( commands, cl_output, CL_TRUE, 0, sizeof(OutputMsg) * prop_group, results, 0, NULL, NULL );
	
        if (err != CL_SUCCESS)
        {
            printf("Error: Failed to read output array! %d\n", err);
            exit(1);
        }
        clFinish(commands);

#ifdef GPU_DEBUG

	memset(debug_log, 0, 1024*sizeof(float));
		/// Read back the debug log from the device
	err = clEnqueueReadBuffer( commands, cl_debug, CL_TRUE, 0, 1024*sizeof(float) , debug_log, 0, NULL, NULL );

		if (err != CL_SUCCESS)
		{
			printf("Error: Failed to read debug array! %d\n", err);
			exit(1);
		}
		clFinish(commands);

		//printf("1====================%f==================\n", debug_log[0]);

#endif

        for(j=0; j<prop_group; ++j)
        {
            if(results[j].m_order <50*fc)
                continue;
            float rt = results[j].m_result;
            float* pd = prop_data +j*prop_count_an;
            int rt_idx;
            const int rt_size = sizeof(OutputMsg)+sizeof(float)*prop_count;
            OutputMsg* hlast = (OutputMsg*)((char*)highest_data+rt_size*(highest-1));
            if(hlast->m_result < rt)
            {
                for(rt_idx =0; rt_idx < highest; ++rt_idx)
                {
                    OutputMsg* hcurrent = (OutputMsg*)((char*)highest_data+rt_size*rt_idx);
                    float* hcdata = (float*)(hcurrent+1);
                    OutputMsg* hnext = (OutputMsg*)((char*)highest_data+rt_size*(rt_idx+1));
                    if (hcurrent->m_result < rt)
                    {
                        if(rt_idx < highest-1)
                            memcpy(hnext, hcurrent, rt_size*(highest - rt_idx -1) );
                        //hcurrent[0] = rt;
                        memcpy(hcurrent, &results[j], sizeof(OutputMsg));
                        memcpy(hcdata, pd, sizeof(float)*prop_count);
                        break;
                    }
                }
            }
            hlast = (OutputMsg*)((char*)lowest_data+rt_size*(lowest-1));
            if(hlast->m_result > rt)
            {
                for(rt_idx =0; rt_idx < lowest; ++rt_idx)
                {
                    OutputMsg* hcurrent = (OutputMsg*)((char*)lowest_data+rt_size*rt_idx);
                    float* hcdata = (float*)(hcurrent+1);
                    OutputMsg* hnext = (OutputMsg*)((char*)lowest_data+rt_size*(rt_idx+1));
                    if (hcurrent->m_result > rt)
                    {
                        if(rt_idx < lowest-1)
                            memcpy(hnext, hcurrent, rt_size*(lowest - rt_idx -1) );
                        memcpy(hcurrent, &results[j], sizeof(OutputMsg));
                        memcpy(hcdata, pd, sizeof(float)*prop_count);
                        break;
                    }
                }
            }

        }


    } /* for-i: prop_group */
    int64_t tend;
    get_tick_count(&tend);
    lmice_critical_print("Calc time %lld\n", tend-tbegin);
    OutputMsg* om = (OutputMsg*)((char*)highest_data);
    printf("best %d  %f\n", om->m_order, om->m_result);
    {
        int pc_idx;
        for(pc_idx = 0; pc_idx < prop_count; ++pc_idx)
        {
            printf("a[%d] = %f\t", pc_idx, highest_data[2+pc_idx]);
        }
        printf("\n");
    }
    struct rt_dummy
    {
        OutputMsg msg;
        float data[1];
    };
    const int rt_size = sizeof(OutputMsg)+sizeof(float)*prop_count;
    {
        int i;
        int rt_idx;
        FILE* fp;
        if(bsfs_mode==1)
            fp = fopen("lowest_sfs.csv", "w");
        else
            fp = fopen("lowest.csv", "w");
        fprintf(fp, "\"order\",\"result\"");
        for(i=0; i<prop_count; ++i)
            fprintf(fp, ",\"a[%d]\"", i);
        fprintf(fp, "\n");
        for(rt_idx =0; rt_idx < lowest; ++rt_idx)
        {
            struct rt_dummy* hd = (struct rt_dummy*)((char*)lowest_data+rt_size*rt_idx);

            fprintf(fp, "%d,%f", hd->msg.m_order, hd->msg.m_result);
            for(i=0; i<prop_count; ++i)
                fprintf(fp, ",%f", hd->data[i]);
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
    {
        int i;
        int rt_idx;
        FILE* fp;
        if(bsfs_mode==1)
            fp = fopen("highest_sfs.csv", "w");
        else
            fp = fopen("highest.csv", "w");
        fprintf(fp, "\"order\",\"result\"");
        for(i=0; i<prop_count; ++i)
            fprintf(fp, ",\"a[%d]\"", i);
        fprintf(fp, "\n");
        for(rt_idx =0; rt_idx < highest; ++rt_idx)
        {
            struct rt_dummy* hd = (struct rt_dummy*)((char*)highest_data+rt_size*rt_idx);
            fprintf(fp, "%d,%f", hd->msg.m_order, hd->msg.m_result);
            for(i=0; i<prop_count; ++i)
                fprintf(fp, ",%f", hd->data[i]);
            fprintf(fp, "\n");
        }
        fclose(fp);
    }

    /// Print a brief summary detailing the results
    printf("Computed!\n");

    /// Shutdown and cleanup
    munmap(host_bulk, hb_size);

    clReleaseMemObject(cl_output);
    clReleaseMemObject(cl_prop_data);
    clReleaseMemObject(cl_msg_data);
    clReleaseMemObject(cl_sig_data);
    clReleaseMemObject(cl_mkt_data);
    clReleaseMemObject(cl_mid_data);
    clReleaseMemObject(cl_ask_data);
    clReleaseMemObject(cl_bid_data);

    //free(platform_id);
    //free(device_ids);

    clReleaseContext(context);
    clReleaseCommandQueue(commands);
    clReleaseProgram(program);
    clReleaseKernel(kernel);

    cfg_close(cfg);



    return 0;
}
