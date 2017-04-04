
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <OpenCL/opencl.h>

#include "config.h"
#include "prng.h"

#include <locale.h>
#include <pthread.h>

#include <lmice_trace.h> /*EAL library */


/*****************************************************************************/
/** Global definition
*/


#define DATA_SIZE (1024)

struct __attribute__((aligned(8))) ChinaL1Msg
{
    uint64_t m_inst;
    int64_t m_time_micro; /* time in epoch micro seconds */
    double m_bid;
    double m_offer;
    int32_t m_bid_quantity;
    int32_t m_offer_quantity;
    int32_t m_volume;
    double m_notional;
    double m_limit_up;
    double m_limit_down;
};

/*****************************************************************************/

/** Simple compute kernel which computes the square of an input array
*/
//const char *KernelSource = "\n";
static inline void print_usage(void)
{
    printf("Signal Simulation App\n"
           "Usage:\n"
           "myopencl -argn1 argv1 --argn2=argv2...>\n"
           "\t-h,--help\t\tPrint this message.\n"
           "\t-d,--device=d\t\tChoose calculation device.\n");
}

////////////////////////////////////////////////////////////////////////////////

static inline void print_platform_info(cl_platform_id platform_id, int pos)
{
    char buff[128];
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
    char buff[128];
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
    printf(" Version: %s ", buff);
    clGetDeviceInfo(device_id,
                    CL_DEVICE_NAME,
                    sizeof(buff),
                    buff,
                    NULL);
    printf("Name: %s\n", buff);
}

static inline void replace_str(char* buff, const char* sep, const char* rep, size_t rep_len)
{
    char *bpos;
    char* temp;
    size_t len;
    size_t len2;
    size_t len3;

    if(!buff)
        return;

    len = strlen(sep);
    len2 = strlen(buff);

    temp = (char*)malloc(len2+rep_len);
    do {
        bpos = strstr(buff, sep);
        if(bpos)
        {
            len3 = strlen(bpos+len);
            strncpy(temp, bpos+len, len3);
            if(len > rep_len)
                buff[len2 - len + rep_len] = '\0';

            strncpy(bpos, rep, rep_len);
            strncpy(bpos+rep_len, temp, len3);
        }
    } while(bpos);
}

int main(int argc, char** argv)
{
    int err;                            // error code returned from api calls

    /// Host side input/output data
    float *results;           // results returned from device
    float g_signal_multiplier = 1;
    double *prop_data;      //property data from PRNG
    unsigned int prop_count;// = 8; count of subsignals
    unsigned int prop_count_an;// aligned count of subsignals
    unsigned int prop_group;// = global;
    long prop_trial;                //total trial
    long prop_seed;                 // PRNG seed
    long prop_pos;                  // number position
    double *sig_data;               //sub signals data from .fc file
    unsigned int sig_count;         //sub signals data size
    struct ChinaL1Msg* msg_data;    // message data from .dat file
    unsigned int msg_count;         //message data size
    float* mkt_data;                //Market data[mid, ask, bid]
    float* mkt_mid;
    float* mkt_ask;
    float* mkt_bid;

    size_t global = 65536;              // global domain size for our calculation
    size_t local;                       // local domain size for our calculation

    /// Device side param
    cl_mem cl_output;
    cl_mem cl_prop_data;
    //cl_mem cl_msg_data;
    cl_mem cl_sig_data;
    cl_mem cl_mkt_data;     //Market data [mid, ask, bid]

    cl_uint device_num;
    cl_device_id * device_ids;
    cl_device_id device_id;             // compute device id
    cl_context context;                 // compute context
    cl_command_queue commands;          // compute command queue
    cl_program program;                 // compute program
    cl_kernel kernel;                   // compute kernel


    cl_uint platforms;
    cl_platform_id *platform_id;

    /// Config param
    config_t cfg;

    int dev_id;                 // Device id
    const char* cfg_file=NULL;  // Config file


    long i = 0;
    long j = 0;


    clGetPlatformIDs(0,0,&platforms);
    platform_id = (cl_platform_id*)malloc(platforms * sizeof(cl_platform_id));
    clGetPlatformIDs (platforms, platform_id, NULL);
    printf("platform num %u\n", platforms);
    for(i=0; i< platforms; ++i)
    {
        print_platform_info(platform_id[i], i);


        err = clGetDeviceIDs(platform_id[i], CL_DEVICE_TYPE_ALL, 0, 0, &device_num);
        device_ids = (cl_device_id *)malloc(sizeof(cl_device_id)*device_num);
        err = clGetDeviceIDs(platform_id[i],CL_DEVICE_TYPE_ALL,device_num,device_ids,NULL);

        printf("device num %u\n", device_num);
        for(j=0; j< device_num; ++j)
        {
            printf("\t");
            print_device_info(device_ids[j], j);

        }


    }
    //setlocale(LC_TIME, "zh_CN.UTF-8");
    pthread_setname_np("SignalSimulation");

    // Connect to a compute device
    /** Process command line */
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
                dev_id = atoi(cmd);
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
            dev_id = atoi(cmd);
        }
        else if(strcmp(cmd, "-i") == 0 ||
                strcmp(cmd, "--input") == 0)
        {
            if(i+1 < argc)
            {
                cmd = argv[i+1];
                cfg_file = cmd;
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
            cfg_file = cmd;
        }
    }

    if (dev_id >= device_num)
    {
        printf("Error: Failed to create a device group!!\n");
        return EXIT_FAILURE;
    }
    cfg=cfg_init_file(cfg_file);
    lmice_critical_print("cfg insttype=%s\n", cfg_get_string(cfg, "insttype"));
    device_id = device_ids[dev_id];
    print_device_info(device_id, dev_id);

    // Create a compute context
    //
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context)
    {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }



    char* kernel_src = NULL;
    {
        FILE* fp = NULL;
        long fsize = 0;
        const char* name;

        name = cfg_get_string(cfg, "optimizer.clfile");
        if(!name)
        {
            lmice_error_print("CL file is missing in config\n");
            return EXIT_FAILURE;
        }

        fp = fopen(name, "r");
        if(!fp)
        {
            lmice_error_print("CL file(%s) cannot open\n", name);
            return EXIT_FAILURE;
        }

        fseek(fp, 0L, SEEK_END);
        fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        kernel_src = (char*) malloc((fsize+1)*sizeof(char));
        fread(kernel_src, fsize, 1, fp);
        kernel_src[fsize]='\0';
        fclose(fp);
        //printf("code:\n%s\n", kernel_src);
    }

    /// Create the compute program from the source buffer
    program = clCreateProgramWithSource(context, 1, (const char **) & kernel_src, NULL, &err);
    if (!program)
    {
        printf("Error: Failed to create compute program!\n");
        return EXIT_FAILURE;
    }

    /// Build the program executable
    err = clBuildProgram(program, 1, &device_id, "-cl-std=CL1.2", NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        lmice_error_print("Error: Failed to build program executable[%d] %s!\n", err, buffer);
        return EXIT_FAILURE;
    }

    /// Create the compute kernel in the program we wish to run
    const char* clfunc = cfg_get_string(cfg, "optimizer.clfunc");
    kernel = clCreateKernel(program, clfunc, &err);
    if (!kernel || err != CL_SUCCESS)
    {
        lmice_error_print("Error: Failed to create compute kernel!\n");
        return EXIT_FAILURE;
    }

    /// Get the maximum work group size for executing the kernel on the device
    err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
    if (err != CL_SUCCESS)
    {
        lmice_error_print("Error: Failed to retrieve kernel work group info! %d\n", err);
        return EXIT_FAILURE;
    }

    /// Create the host side input and output
    {
        printf("Create the host side input and output\n");
        config_t idx;
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

        const char* outpath = cfg_get_string(cfg, "signal_calc.outpath");
        file = cfg_get_list_string(cfg, "signal_calc.files", 0);
        if(!file)
        {
            lmice_error_print("signal_calc files is missing\n");
            return EXIT_FAILURE;
        }

//        int i=1;
//        float* fi = (float*)&i;
//        float ft=1.0;
//        int* ift = (int*)&ft;
//        printf("int=%d\t float=%f\n", i, *fi);
//        printf("int=%d\t float=%f\n", *ift, ft);
//        i=1065353215;
//        printf("int=%d\t float=%f\n", i, *fi);
//        return 0;

        bpos = strstr(file, bsep);
        sdate = bpos -10;
        stime = bpos+2;
        stype = cfg_get_string(cfg, "insttype");
        pattern = cfg_get_string(cfg, "signal_calc.pattern");
        strcat(name, outpath);
        strcat(name, "/");
        strcat(name, pattern);
        //printf("buff is %s\n d=%s\n t=%s\n t=%s\n", buff, sdate, stime, stype);

        //printf("%s\n", buff);
        replace_str(name, "%type", stype, strlen(stype));
        //printf("%s\n", buff);
        replace_str(name, "%date", sdate, 10);
        //printf("%s\n", buff);
        replace_str(name, "%time", stime, 8);
        //printf("%s\n", buff);
        brep_count = cfg_get_list_size(cfg, "optimizer.clreps");
        printf("brep_count = %d\n", brep_count);

        bsep = cfg_get_list_string(cfg, "optimizer.clreps", 0);
        brep = cfg_get_list_string(cfg, "optimizer.clreps", 1);
        replace_str(name, bsep, brep, strlen(brep));

        /// Create prop data from prng
        strcat(name, ".idx");
        printf("index name is %s\n", name);

        /// Create prop_count from index file
        idx = cfg_init_file(name);
        if(!idx)
        {
            lmice_error_print("cannot open index file %s\n", name);
            return EXIT_FAILURE;
        }
        sig_aligned = cfg_get_integer(cfg,  "optimizer.aligned");
        prop_seed = cfg_get_integer(cfg,    "optimizer.seed");
        prop_pos = cfg_get_integer(cfg,     "optimizer.pos");
        prop_group = cfg_get_integer(cfg,   "optimizer.group");
        prop_count = cfg_get_integer(idx,   "size");
        printf("sub signals size %d\n", prop_count);
        prop_count_an = ((prop_count + sig_aligned -1)/sig_aligned)*sig_aligned;
        printf("sub signals refined size %d\n", prop_count_an);
        prop_trial = cfg_get_integer(cfg, "optimizer.trial");
        prop_data = (double*)malloc(sizeof(double)*prop_count_an*prop_group);
        memset(prop_data, 0, sizeof(double)*prop_count_an*prop_group);


        //prng_free(pn);

        /// Create signal from forecast file
        strcpy(name+strlen(name)-4, ".fc");
        printf("forecast file is %s\n", name);
        FILE* fp = fopen(name, "rb");
        long fsize;
        fseek(fp, 0L, SEEK_END);
        fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        if(fsize % (sizeof(double)*prop_count) != 0 || fsize == 0)
        {
            lmice_error_print("forecast content is incorrect of size %ld %ld\n", fsize, fsize % (sizeof(double)*prop_count));
            return EXIT_FAILURE;
        }
        sig_count = fsize / (sizeof(double)*prop_count);
        sig_data = (double*) malloc( sizeof(double)*prop_count_an*sig_count );
        memset(sig_data, 0, sizeof(double)*prop_count_an*sig_count );
        for(i=0; i<sig_count; ++i)
        {
            fread(sig_data+i*prop_count_an, sizeof(double)*prop_count, 1, fp);
        }

        fclose(fp);

        /// Create message from market file
        strcpy(name+strlen(name)-3, ".dat");
        printf("message file is %s\n", name);
        fp = fopen(name, "rb");
        fseek(fp, 0L, SEEK_END);
        fsize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        if(fsize % sizeof(struct ChinaL1Msg) != 0 || fsize == 0)
        {
            lmice_error_print("market content is incorrect of size %ld %ld\n", fsize, fsize % sizeof(struct ChinaL1Msg));
            return EXIT_FAILURE;
        }
        msg_count = fsize / sizeof(struct ChinaL1Msg);
        msg_data = (struct ChinaL1Msg*) malloc(fsize);
        //memset(msg_data, 0, fsize);
        fread(msg_data, fsize, 1, fp);
        fclose(fp);

        /// Generate market data
        mkt_data = (float*)malloc(sizeof(float)*3*msg_count);
        mkt_mid = (float*)malloc(sizeof(float)*msg_count);
        mkt_ask = (float*)malloc(sizeof(float)*msg_count);
        mkt_bid = (float*)malloc(sizeof(float)*msg_count);
        for(i=0; i<msg_count; ++i)
        {
            struct ChinaL1Msg* pc = msg_data+i;
            float* mkt = mkt_data+i*3;
            *mkt = (pc->m_bid+pc->m_offer)*0.5;
            *(mkt+1) = pc->m_offer;
            *(mkt+2) = pc->m_bid;
            *mkt_mid = *(mkt+0);
            *mkt_ask = *(mkt+1);
            *mkt_bid = *(mkt+2);
        }

        /// Create result
        results = (float*)malloc(sizeof(float) * prop_group);

        /// Create CL script
        long vec_len = cfg_get_integer(cfg, "optimizer.clvec");
        fp = fopen("temp.cl", "w");
        fprintf(fp, "\n"
                    "#pragma OPENCL EXTENSION cl_amd_printf : enable \n"
                    "#pragma OPENCL EXTENSION cl_intel_printf : enable\n"
                    "#pragma OPENCL EXTENSION cl_nv_printf : enable\n"
                    "#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n"
                    "struct ChinaL1Msg\n"
                    "{\n"
                    "     ulong m_inst;\n"
                    "     long m_time_micro; /* time in epoch micro seconds */\n"
                    "     double m_bid;\n"
                    "     double m_offer;\n"
                    "     int m_bid_quantity;\n"
                    "     int m_offer_quantity;\n"
                    "     int m_volume;\n"
                    "     double m_notional;\n"
                    "     double m_limit_up;\n"
                    "     double m_limit_down;\n"
                    " } __attribute__ ((aligned (8))) ;\n"
                    "\n"
                    "__kernel void signal_simulation(\n"
                    "    __global const double4* prop_data,   /* prop_count * prop_group */\n"
                    "    __global const double4* sig_data,    /* prop_count * msg_count*/\n"
                    "    /*__global const struct ChinaL1Msg* msg,  msg_count*/\n"
                    "    __global const float3* mkt_data,    /*msg_count */\n"
                    "    __global float* output,            /* prop_group*/\n"
                    "    const float g_signal_multiplier,\n"
                    "    const unsigned int msg_count,\n"
                    "    const unsigned int prop_count,\n"
                    "    const unsigned int prop_group,\n"
                    "    const unsigned int device_type,/* 0-CPU    1-GPU */\n"
                    "    __global const float%ld* mkt_mid,\n"
                    "    __global const float%ld* mkt_ask,\n"
                    "    __global const float%ld* mkt_bid)\n"
                    "{\n"
                    "    int group = get_global_id(0);\n"
                    "    int j;\n"
                    "    int i;\n"
                    " \n"
                    "    if(group >= prop_group)\n"
                    "        return;\n"

                ,vec_len, vec_len, vec_len);

        fprintf(fp, "\n"
                    "    for(j=0; j<msg_count/%ld; ++j)\n"
                    "    {\n"
                    "        int msg_pos = j*%ld;\n"
                    "        int prop_pos = %u;\n"
                "        float%ld signal =(float%ld)(\n", vec_len, vec_len,  prop_count_an/4, vec_len, vec_len);
        for(i=0; i< vec_len; ++i)
        {
            for(j=0; j<prop_count_an/4 -1; ++j)
            {
                fprintf(fp,
                        "            dot(prop_data[group*prop_pos+%ld], sig_data[(msg_pos+%ld)*prop_pos+%ld]) +\n", j, i, j);
            }

            if(i +1  == vec_len)
                fprintf(fp,
                        "            dot(prop_data[group*prop_pos+%u], sig_data[(msg_pos+%ld)*prop_pos+%u])\n\n", prop_count_an/4 -1, i, prop_count_an/4 -1);
            else
                fprintf(fp,
                        "            dot(prop_data[group*prop_pos+%u], sig_data[(msg_pos+%ld)*prop_pos+%u]),\n\n", prop_count_an/4 -1, i, prop_count_an/4 -1);
        }
        fprintf(fp, "           );/* end-for signal */\n");
        fprintf(fp, "       float%ld forecast = mad(mkt_mid[j], signal, mkt_mid[j]);\n", vec_len);
        fprintf(fp, "       for(i=0; i< %ld; ++i)\n"
                    "       {\n"
                    "           if(forecast[i] > mkt_ask[j][i])\n"
                    "           {\n"
                    "               output[group] = forecast[i];\n"
                    "           }\n"
                    "           else if(forecast[i] < mkt_bid[j][i])\n"
                    "           {\n"
                    "               output[group] = forecast[i];\n"
                    "            }\n"
                    "        }\n"
                    "    }\n"
                    "}\n" ,vec_len);
        fclose(fp);

    }

    cl_mem cl_mid_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);
    cl_mem cl_ask_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);
    cl_mem cl_bid_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * msg_count, NULL, NULL);

    /// Create the input and output arrays in device memory for our calculation
    cl_sig_data = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(double)*prop_count_an*sig_count, NULL, NULL);
    //cl_msg_data = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(struct ChinaL1Msg) * msg_count, NULL, NULL);
    cl_prop_data = clCreateBuffer(context,  CL_MEM_READ_ONLY,  prop_count_an*sizeof(double)*prop_group, NULL, NULL);
    cl_mkt_data = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * msg_count * 3, NULL, NULL);
    cl_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * prop_group, NULL, NULL);
    if (!cl_sig_data || !cl_prop_data || !cl_output || !cl_mkt_data)
    {
        lmice_error_print("Error: Failed to allocate device memory!\n");
        exit(1);
    }

    //return 0;
    /// Set the arguments to our compute kernel
    err = 0;
    err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), &cl_prop_data);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &cl_sig_data);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &cl_mkt_data);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &cl_output);
    err |= clSetKernelArg(kernel, 4, sizeof(float), &g_signal_multiplier);
    err |= clSetKernelArg(kernel, 5, sizeof(unsigned int), &msg_count);
    err |= clSetKernelArg(kernel, 6, sizeof(unsigned int), &prop_count_an);
    err |= clSetKernelArg(kernel, 7, sizeof(unsigned int), &prop_group);
    err |= clSetKernelArg(kernel, 8, sizeof(unsigned int), &dev_id);
    err |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &cl_mid_data);
    err |= clSetKernelArg(kernel, 10, sizeof(cl_mem), &cl_ask_data);
    err |= clSetKernelArg(kernel, 11, sizeof(cl_mem), &cl_bid_data);
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
    err = clEnqueueWriteBuffer(commands, cl_sig_data, CL_TRUE, 0, sizeof(double) * prop_count_an * sig_count, sig_data, 0, NULL, NULL);
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
    err = clEnqueueWriteBuffer(commands, cl_mkt_data, CL_TRUE, 0, sizeof(float) * 3 * msg_count, mkt_data, 0, NULL, NULL);
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

    //lmice_info_print("begin process\n");
    for(i=0; i<prop_trial; i += global)
    {

        // Update subsignals properties
        for(j=0; j < prop_group; ++j)
        {
            int pc_idx = 0;
            for(;pc_idx<prop_count; ++pc_idx)
            {
                prop_data[j*prop_count_an + pc_idx] = prng_next(mt19937);
            }
        }

        err = clEnqueueWriteBuffer(commands, cl_prop_data, CL_TRUE, 0, sizeof(double) * prop_count_an * prop_group, prop_data, 0, NULL, NULL);
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

        err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
        if (err)
        {
            printf("Error: Failed to execute kernel![%d]\n", err);
            return EXIT_FAILURE;
        }

        /// Wait for the command commands to get serviced before reading back results
        clFinish(commands);

        lmice_info_print("calculate process finished prop_group-%u\n", prop_group);

        /// Read back the results from the device to verify the output
        err = clEnqueueReadBuffer( commands, cl_output, CL_TRUE, 0, sizeof(float) * prop_group, results, 0, NULL, NULL );
        if (err != CL_SUCCESS)
        {
            printf("Error: Failed to read output array! %d\n", err);
            exit(1);
        }


    } /* for-i: prop_group */



    /// Print a brief summary detailing the results
    printf("Computed!\n");

    /// Shutdown and cleanup
    clReleaseMemObject(cl_prop_data);
    clReleaseMemObject(cl_sig_data);
    //clReleaseMemObject(cl_msg_data);
    clReleaseMemObject(cl_mkt_data);
    clReleaseMemObject(cl_output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);

    free(msg_data);
    free(mkt_data);
    free(sig_data);
    free(prop_data);
    free(results);

    return 0;
}
