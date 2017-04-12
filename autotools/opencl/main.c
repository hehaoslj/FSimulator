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

static inline int proc_cmdline(int argc, char** argv, int* dev_id, const char** cfg_file)
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
    }
    return 0;
}

static inline int load_kernel_from_source(const char* cfg_file, cl_context context, cl_device_id device_id, cl_program* pp)
{
    cl_int err;
    const char* kernel_src;
    cl_program program;

    // Generate Kernel source
    kernel_src = genopencltask(cfg_file);

    /// Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char **) & kernel_src, NULL, &err);
    if (!program)
    {
        lmice_error_print("Failed to create compute program from source\n");
        return EXIT_FAILURE;
    }

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

    /** Config param */
    config_t cfg;                       // Config object
    float* highest_data = NULL;                // Output highest data
    float* lowest_data = NULL;                 // Output lowest data
    int highest;                        // size of highest data
    int lowest;                         // size of lowest data

    /** OpenCL param */
    size_t global = 65536;              // global domain size for our calculation
    size_t local=1;                       // local domain size for our calculation

    /** Device side param */
    cl_mem cl_output = NULL;                   // results

    cl_mem cl_prop_data = NULL;                // property data
    cl_mem cl_msg_data = NULL;                 // message data
    cl_mem cl_sig_data = NULL;                 // sub signals data
    cl_mem cl_mkt_data = NULL;                 // Market data [mid, ask, bid]
    cl_mem cl_mid_data = NULL;                 // Market mid data
    cl_mem cl_ask_data = NULL;                 // Market ask data
    cl_mem cl_bid_data = NULL;                 // Market bid data


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


    long i = 0;
    long j = 0;

    //setlocale(LC_TIME, "zh_CN.UTF-8");

    /** Process command line */
    err = proc_cmdline(argc, argv, &dev_id, &cfg_file);
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

    /** Create the host side input and output */
    {
        lmice_info_print("Create the host side input and output\n");
        const char* pattern;
        const char* file;
        const char* stype;
        const char* sdate;
        const char* stime;
        char name[256] ={0};
        char* bpos;
        long sig_aligned = 1;


        int brep_count;
        const char* brep;
        const char* bsep = "__";

        size_t pd_size = 0; //(prop data size(bytes)
        size_t sg_size = 0; //signal data size(bytes)
        size_t fs_size = 0; //(float signal data size(bytes)
        size_t mm_size = 0; // market message size(bytes)

        FILE* fp_sig;   //signal file
        FILE* fp_msg;   //market message file

        const char* outpath = cfg_get_string(cfg, "signal_calc.outpath");
        file = cfg_get_list_string(cfg, "signal_calc.files", 0);
        if(!file)
        {
            lmice_error_print("signal_calc files is missing\n");
            return EXIT_FAILURE;
        }

        bpos = strstr(file, bsep);
        sdate = bpos -10;
        stime = bpos+2;
        stype = cfg_get_string(cfg, "insttype");
        pattern = cfg_get_string(cfg, "signal_calc.pattern");
        strcat(name, outpath);
        strcat(name, "/");
        strcat(name, pattern);

        //printf("%s\n", buff);
        replace_str(name, "%type", stype, strlen(stype));
        //printf("%s\n", buff);
        replace_str(name, "%date", sdate, 10);
        //printf("%s\n", buff);
        replace_str(name, "%time", stime, 8);
        //printf("%s\n", buff);
        brep_count = cfg_get_list_size(cfg, "optimizer.clreps");
        printf("brep_count = %d\n", brep_count);
        for(i=0; i<brep_count/2; ++i)
        {
            bsep = cfg_get_list_string(cfg, "optimizer.clreps", i*2);
            brep = cfg_get_list_string(cfg, "optimizer.clreps", i*2+1);
            replace_str(name, bsep, brep, strlen(brep));
        }

        // Read prop_count from index file
        {
            config_t idx;
            strcat(name, ".idx");
            printf("nm = %s\n", name);
            idx = cfg_init_file(name);
            if(!idx)
            {
                lmice_error_print("cannot open index file %s\n", name);
                return EXIT_FAILURE;
            }
            prop_count = cfg_get_integer(idx,   "size");
            lmice_info_print("sub signals AKA prop_count size %u\n", prop_count);
            cfg_close(idx);

            // Reset name extension; the number 4 is strlen('.idx')
            name[strlen(name)-4] = '\0';
        }

        sig_aligned = cfg_get_integer(cfg,  "optimizer.aligned");
        prop_count_an = ((prop_count + sig_aligned -1)/sig_aligned)*sig_aligned;
        lmice_info_print("prop_count aligned AKA prop_count_an size %u\n", prop_count_an);

        prop_seed =     cfg_get_integer(cfg,    "optimizer.seed");
        prop_pos =      cfg_get_integer(cfg,    "optimizer.pos");
        prop_group =    cfg_get_integer(cfg,    "optimizer.group");
        prop_trial =    cfg_get_integer(cfg,    "optimizer.trial");

        // Calc prop_data size sizeof(float)*prop_count_an*prop_group
        pd_size = sizeof(PROP_TYPE)*prop_count_an*prop_group;

        // Read signal from forecast file
        strcat(name, ".fc");
        fp_sig = fopen(name, "rb");
        fseek(fp_sig, 0L, SEEK_END);
        sg_size = ftell(fp_sig);
        fseek(fp_sig, 0L, SEEK_SET);
        if(sg_size % (sizeof(double)*prop_count) != 0 || sg_size == 0)
        {
            lmice_error_print("forecast content is incorrect of size %ld %ld\n", sg_size, sg_size % (sizeof(double)*prop_count));
            return EXIT_FAILURE;
        }
        sig_count = sg_size / (sizeof(double)*prop_count);
        sg_size = sizeof(double)*prop_count_an*sig_count;


        // Generate float type signal data
        fs_size = sizeof(float)*prop_count_an*sig_count;


        // Read market data from market file
        strcpy(name+strlen(name)-3, ".dat");
        fp_msg = fopen(name, "rb");
        fseek(fp_msg, 0L, SEEK_END);
        mm_size = ftell(fp_msg);
        fseek(fp_msg, 0L, SEEK_SET);
        if(mm_size % sizeof(ChinaL1Msg) != 0 || mm_size == 0)
        {
            lmice_error_print("market content is incorrect of size %ld %ld\n", mm_size, mm_size % sizeof(ChinaL1Msg));
            return EXIT_FAILURE;
        }
        msg_count = mm_size / sizeof(ChinaL1Msg);

        // Generate market data vectorized


        // Get result data size
        highest = cfg_get_integer(cfg, "optimizer.highest");
        lowest =  cfg_get_integer(cfg, "optimizer.lowest");


        // Signal multiple
        double mult = cfg_get_real(cfg, "optimizer.sig_multiple");
        if(mult != 0)
            prop_multi = mult;

        /** Create memory bulk at host side*/
        hb_size =  pd_size + sg_size + fs_size + mm_size + sizeof(float)*3*msg_count*2 //mkt
                + sizeof(OutputMsg) * prop_group //output
                +(sizeof(OutputMsg) + sizeof(float) * prop_count)* highest
                +(sizeof(OutputMsg) + sizeof(float) * prop_count)* lowest;
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
            lmice_error_print("Create memory bulk at host size[%luMB] failed[%p]\n", hb_size/(1024*1024), host_bulk);
            perror(NULL);
            return EXIT_FAILURE;
        }

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

        // Read Signal Data
        memset(sig_data, 0, sg_size);
        for(i=0; i<sig_count; ++i)
        {
            fread(sig_data+i*prop_count_an, sizeof(double)*prop_count, 1, fp_sig);
        }
        fclose(fp_sig);

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
        fread(msg_data, mm_size, 1, fp_msg);
        fclose(fp_msg);

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
        }

        // Init result data
        memset(highest_data, 0, (sizeof(OutputMsg) + sizeof(float) * prop_count)* highest);
        memset(lowest_data, 'c',  (sizeof(OutputMsg) + sizeof(float) * prop_count)* lowest);



    }

    cl_mid_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);
    cl_ask_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);
    cl_bid_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);

    /// Create the input and output arrays in device memory for our calculation
    cl_sig_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(SIG_TYPE)*prop_count_an*sig_count, NULL, NULL);
    //cl_msg_data = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(struct ChinaL1Msg) * msg_count, NULL, NULL);
    cl_prop_data = clCreateBuffer(context,  CL_MEM_READ_ONLY,  prop_count_an*sizeof(PROP_TYPE)*prop_group, NULL, NULL);
    //cl_mkt_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * msg_count * 3, NULL, NULL);
    cl_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(OutputMsg) * prop_group, NULL, NULL);
    if (!cl_sig_data || !cl_prop_data || !cl_output)// || !cl_mkt_data)
    {
        lmice_error_print("Error: Failed to allocate device memory! sig=%p prop=%p output=%p mkt=%p prop_data=%p\n", cl_sig_data, cl_prop_data, cl_output, cl_mkt_data, prop_data);
        exit(1);
    }

    //return 0;
    /// Set the arguments to our compute kernel
    err = 0;
    i=0;
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_prop_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_sig_data);
    //err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_mkt_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_output);
    //err |= clSetKernelArg(kernel, i++, sizeof(float), &prop_multi);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &msg_count);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &prop_count_an);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &prop_group);
    err |= clSetKernelArg(kernel, i++, sizeof(unsigned int), &dev_id);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_mid_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_ask_data);
    err |= clSetKernelArg(kernel, i++, sizeof(cl_mem), &cl_bid_data);
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

    err = clEnqueueWriteBuffer(commands, cl_mid_data, CL_TRUE, 0, sizeof(float) * msg_count, mkt_mid, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(commands, cl_ask_data, CL_TRUE, 0, sizeof(float) * msg_count, mkt_ask, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(commands, cl_bid_data, CL_TRUE, 0, sizeof(float) * msg_count, mkt_bid, 0, NULL, NULL);

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

    prng_t mt19937 = prng_init_seed(prop_seed);
    for(j=0; j<prop_pos; ++j)
    {
        prng_next(mt19937);
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
                //prop_data[j*prop_count_an + pc_idx] = prng_next(mt19937);
                prop_data[j*prop_count_an+pc_idx] = prop_multi * 2 *(prng_next(mt19937) - 0.5);
            }
        }
        cl_event event;
        err = clEnqueueWriteBuffer(commands, cl_prop_data, CL_TRUE, 0, sizeof(PROP_TYPE) * prop_count_an * prop_group, prop_data, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            printf("Error: Failed to write prop_data to source array!\n");
            exit(1);
        }


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

        for(j=0; j<prop_group; ++j)
        {
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
    printf("best %f  %f\n", highest_data[0], highest_data[1]);
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
        FILE* fp = fopen("lowest.csv", "w");
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
        FILE* fp = fopen("highest.csv", "w");
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
