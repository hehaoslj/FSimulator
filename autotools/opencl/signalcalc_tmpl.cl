$def with (content)
$ ctx = content.dict()
$ clvec = content.clvec
$ param_count = (content.param_count + content.aligned -1) / content.aligned * content.aligned

/****************************************************************************
**
**  开发单位：$ctx["user_dept"]
**  开发者：$ctx["user_name"]
**  创建时间：$ctx["tm_now"]
**  版本号：$ctx["version"]
$if ctx.has_key('summary'):
    **  描述信息：$ctx['summary']
$else:
    **  描述信息：信号参数优化计算核函数 OpenCL 1.2规格版本
****************************************************************************/

#pragma OPENCL EXTENSION cl_amd_printf : enable
#pragma OPENCL EXTENSION cl_intel_printf : enable
#pragma OPENCL EXTENSION cl_nv_printf : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

/****************************************************************************
*  Structure declaration
****************************************************************************/

/** ChinaL1Msg */
typedef struct _ChinaL1Msg
{
    ulong m_inst;
    long m_time_micro; /* time in epoch micro seconds */
    double m_bid;
    double m_offer;
    int m_bid_quantity;
    int m_offer_quantity;
    int m_volume;
    double m_notional;
    double m_limit_up;
    double m_limit_down;
} __attribute__ ((aligned (8))) ChinaL1Msg;


/****************************************************************************
*  Signal calcuation fallback to non-vectorized version
****************************************************************************/
inline float calc_signal(
    const __global double* prop,
    const __global double* sig,
    const unsigned int pc,
    const unsigned int dtype)
{
    int j;
    double rt = 0;
    int prop_count = pc;
    if(dtype)
    {
        do{
           if(prop_count >= 8)
            {
                double8 pp = vload8(0, prop);
                double8 ps = vload8(0, sig);
                rt += dot(pp.hi, ps.hi) + dot(pp.lo, ps.lo);
                prop += 8;
                sig += 8;
                prop_count -=8;
            }
            else if(prop_count >=4 )
            {
                double4 pp = vload4(0, prop);
                double4 ps = vload4(0, sig);
                rt += dot(pp, ps);
                prop += 4;
                sig += 4;
                prop_count -=4;
            }
            else if(prop_count == 3)
            {
                double3 pp = vload3(0, prop);
                double3 ps = vload3(0, sig);
                rt += dot(pp, ps);
                prop += 3;
                sig += 3;
                prop_count -=3;
            }
            else if(prop_count == 2)
            {
                double2 pp = vload2(0, prop);
                double2 ps = vload2(0, sig);
                rt += dot(pp, ps);
                prop += 2;
                sig += 2;
                prop_count -=2;
            }
            else if(prop_count == 1)
            {
                rt += prop[0] * sig[0];
                prop += 1;
                sig += 1;
                prop_count -=1;
            }
        } while(prop_count > 0);
    }
    else
    {
        for(j=0; j<prop_count; ++j)
        {
            rt += prop[j]*sig[j];
        }
    }
    return (float)rt;

}

$for vec in [2,4,8]:
    /****************************************************************************
    *  Signal calcuation vectorized( $:(vec) ) version
    ****************************************************************************/
    inline float calc_signal_v$:(vec)(
        const __global double$:(vec)* prop,
        const __global double$:(vec)* sig,
        const unsigned int   prop_count,
        const unsigned int dtype)
    {
        int j;
        double rt = 0;
        int pc = prop_count / $:(vec);
        for(j=0; j<pc; ++j)
        {
    $if vec == 8:
                    rt += dot(prop[j].hi, sig[j].hi) + dot(prop[j].lo, sig[j].lo);
    $else:
                    rt += dot(prop[j], sig[j]);
    $#end-if: vec
        }
        return rt;
    }


$#end-for:vec


/****************************************************************************
*  forecast calcuation fallback to non-vectorized version
****************************************************************************/
inline float calc_forecast(__global const double* prop_data,
    __global const double* sig_data,
    __global const float3* mkt_data,
    int prop_pos,
    int msg_pos,
    unsigned int prop_count,
    const unsigned int device_type)
{
    float sig;
    sig = get_forecast(prop_data + prop_pos*prop_count, sig_data+prop_count*msg_pos, prop_count, device_type);

    float mid = mkt_data[msg_pos].x;

    float forecast = mad(mid, sig, mid); //mid(1+sig)

    return forecast;
}
__constant float4 fnorm4 = (float4)(1.0f,1.0f,1.0f,1.0f);
__constant int fone = 0x3f800000; /* float 1.0f */

$ vec_type = 'float'+str(clvec) if clvec > 1 else 'float'
$ sum_type = 'int' + str(clvec) if clvec > 1 else 'int'
/****************************************************************************
*  Kernal for signal's multiple
*  param_count: 4-aligned
****************************************************************************/
__kernel void signal_multiple_simulation (
    __global const double4* param_data, /* param_count/4 * param_group */
    __global const double4* sig_data,   /* param_count/4 * msg_count */
    //__global const ChinaL1Msg* msg_data,/* msg_count */
    __global const float3* mkt_data,    /* msg_count */
    __global float* output,             /* param_group */
    const float g_signal_multiple,      /* initialied multiple */
    const int msg_count,                /* sizeof msg */
    const int param_count,              /* sizeof parameter */
    const int param_group,              /* sizeof param_data */
    const int device_type,              /* 0-CPU    1-GPU */
    __global const $:(vec_type)* mkt_mid,    /* $:(clvec)-vectorized mid data */
    __global const $:(vec_type)* mkt_ask,    /* $:(clvec)-vectorized ask data */
    __global const $:(vec_type)* mkt_bid     /* $:(clvec)-vectorized bid data */
    )
{
    int param_pos = get_global_id(0);    /* current position of parameter set */
    int j;
    int i;

    if( param_pos >= param_group)
        return;


    for(j = 0; j < msg_count / $:(clvec); ++j)
    {
        int msg_pos = j*$:(clvec);
        int pm_pos = param_pos* $:(param_count/4);
        $:(vec_type) signal = ($:(vec_type)) (
$for msg_idx in range(clvec):
    $for pm_idx in range(param_count/4):
        $ delm = ''
        $if pm_idx +1 == param_count/4:
            $if msg_idx+1 == clvec:
                $ delm = ');'
            $else:
                $ delm = ','
        $else:
            $ delm = '+'
        $#
                    dot(param_data[pm_pos+$:(pm_idx)], sig_data[(msg_pos+$:(msg_idx) )*param_pos + $:(pm_idx)])$:(delm)
    $#end-for:pm-idx
$#end-for:msg-idx

        $:(vec_type) forecast = mad(mkt_mid[j], signal, mkt_mid[j]);
$ ask = 'mkt_ask[j]'
$ bid = 'mkt_bid[j]'
$ fc = 'forecast'
$if clvec == 1:
    $ ask = 'mkt_ask[j]'
    $ bid = 'mkt_bid[j]'
    $ fc = 'forecast'
$#end-if
        $:(sum_type) fc = isgreater($:(fc), $:(ask))*fone + isless($:(fc), $:(bid))*fone;
        __private float4* pfc = (__private float4*)&fc;
        output[param_pos] = output[param_pos]
$for i in range(clvec/4):
                + dot( pfc[$:(i)], fnorm4) + dot(pfc[$:(i)], fnorm4)
$#end-for:i
            ;
    }
}
