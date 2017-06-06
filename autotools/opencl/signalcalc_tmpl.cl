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
//pragma OPENCL EXTENSION cl_khr_fp64 : enable

/****************************************************************************
*  Structure declaration
****************************************************************************/

/** ChinaL1Msg */
//typedef struct _ChinaL1Msg
//{
//    ulong m_inst;
//    long m_time_micro; /* time in epoch micro seconds */
//    double m_bid;
//    double m_offer;
//    int m_bid_quantity;
//    int m_offer_quantity;
//    int m_volume;
//    double m_notional;
//    double m_limit_up;
//    double m_limit_down;
//} __attribute__ ((aligned (8))) ChinaL1Msg;

typedef struct _g_status
{
    int m_max_pos;
    int m_buy_pos;
    int m_sell_pos;
    int m_multiple;
    float m_fee_rate;
    float m_fee_vol;
    float m_money;
    int m_order;
} __attribute__ ((aligned (8))) g_status;

///输入报单
typedef struct _CUstpFtdcInputOrderField
{

        ///买卖方向
        int	Direction;
        ///开平标志
        int	OffsetFlag;
        ///价格
        float	LimitPrice;
        ///数量
        int	Volume;

}__attribute__ ((aligned (8))) CUstpFtdcInputOrderField;

typedef struct _OutputMsg
{
    int m_order;
    float m_result;
}__attribute__ ((aligned (8))) OutputMsg;

void deal_order(g_status* conf, CUstpFtdcInputOrderField* g_order);

/****************************************************************************
*  Signal calcuation fallback to non-vectorized version
****************************************************************************/
inline float calc_signal(
    const __global float* prop,
    const __global float* sig,
    const unsigned int pc,
    const unsigned int dtype)
{
    int j;
    float rt = 0;
    int prop_count = pc;
    if(dtype)
    {
        do{
           if(prop_count >= 8)
            {
                float8 pp = vload8(0, prop);
                float8 ps = vload8(0, sig);
                rt += dot(pp.hi, ps.hi) + dot(pp.lo, ps.lo);
                prop += 8;
                sig += 8;
                prop_count -=8;
            }
            else if(prop_count >=4 )
            {
                float4 pp = vload4(0, prop);
                float4 ps = vload4(0, sig);
                rt += dot(pp, ps);
                prop += 4;
                sig += 4;
                prop_count -=4;
            }
            else if(prop_count == 3)
            {
                float3 pp = vload3(0, prop);
                float3 ps = vload3(0, sig);
                rt += dot(pp, ps);
                prop += 3;
                sig += 3;
                prop_count -=3;
            }
            else if(prop_count == 2)
            {
                float2 pp = vload2(0, prop);
                float2 ps = vload2(0, sig);
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
        const __global float$:(vec)* prop,
        const __global float$:(vec)* sig,
        const unsigned int   prop_count,
        const unsigned int dtype)
    {
        int j;
        float rt = 0;
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


__constant float4 fnorm4 = (float4)(1.0f,1.0f,1.0f,1.0f);
__constant int fone = 0x3f800000; /* float 1.0f */

$ vec_type = 'float'+str(clvec) if clvec > 1 else 'float'
$ sum_type = 'int' + str(clvec) if clvec > 1 else 'int'
$ msg_range = param_count/4
/****************************************************************************
*  Kernal for signal's multiple
*  param_count: 4-aligned
****************************************************************************/
__kernel void signal_multiple_simulation (
    __global const float4* param_data, /* param_count/4 * param_group */
    __global const float4* sig_data,   /* param_count/4 * msg_count */
    //__global const ChinaL1Msg* msg_data,/* msg_count */
    //__global const float3* mkt_data,    /* msg_count */
    __global OutputMsg* output,             /* param_group */
    //const float g_signal_multiple,      /* initialied multiple */
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

    output[param_pos].m_result = 0;
    output[param_pos].m_order = 0;
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
                    dot(param_data[pm_pos+$:(pm_idx)], sig_data[(msg_pos+$:(msg_idx) )*$:(msg_range)+$:(pm_idx)])$:(delm)
    $#end-for:pm-idx
$#end-for:msg-idx

        //signal = signal * 2.2f;
        //$:(vec_type) forecast = mkt_mid[j]*(1+signal);
        $:(vec_type) forecast = mad(mkt_mid[j], signal, mkt_mid[j]);
        int rt = 0;
        float* pfc = (float*)&forecast;
        __global float* pask = (__global float*)&mkt_ask[j];
        __global float* pbid = (__global float*)&mkt_bid[j];
$ ask = 'pask[i]'
$ bid = 'pbid[i]'
$ fc = 'pfc[i]'
$if clvec == 1:
    $ ask = '*pask'
    $ bid = '*pbid'
    $ fc = '*pfc'
$#end-if


        for(i=0; i<$:(clvec);++i)
        {
            if($(fc) > $(ask))
                rt += 1;
            else if($(fc) < $(bid))
                rt += 1;
        }


        output[param_pos].m_result = output[param_pos].m_result + rt;
        output[param_pos].m_order = output[param_pos].m_order + rt;
    }
}

void flatten(g_status* conf, float ask, float bid)
{
    CUstpFtdcInputOrderField g_order;
    g_order.OffsetFlag = 3;//平
    if(conf->m_buy_pos >0 )
    {
        g_order.Direction = 1;//卖
        g_order.LimitPrice = ask;
        g_order.Volume = conf->m_buy_pos;
        deal_order(conf, &g_order);
    }

    if(conf->m_sell_pos >0 )
    {
        g_order.Direction = 0;//买
        g_order.LimitPrice = bid;
        g_order.Volume = conf->m_sell_pos;
        deal_order(conf, &g_order);
    }

//printf("ask %d=%f bid %d=%f\\n",  conf->m_buy_pos, ask, conf->m_sell_pos, bid);
}

void deal_order(g_status* conf, CUstpFtdcInputOrderField* g_order)
{
    float money = g_order->LimitPrice * g_order->Volume * conf->m_multiple;

    float fee = 0;

    if(conf->m_fee_rate == 0)
    {
        fee = conf->m_fee_vol * g_order->Volume;
    }
    else
    {
        fee = conf->m_fee_rate * money;
    }

    //维护持仓
    if(g_order->Direction == 0)//买
    {
        if(g_order->OffsetFlag == 3)//平
        {
            conf->m_sell_pos -= g_order->Volume;
        }
        else //开
        {
            conf->m_buy_pos += g_order->Volume;
        }

//printf("buy %f, %f, fee: %f  pl:%f\\n", conf->m_money, money, fee, conf->m_money - money);
        //维护PL
        conf->m_money = conf->m_money - money - fee;
        conf->m_order += 1;

    }
    else    //卖
    {
        if(g_order->OffsetFlag == 3)//平
        {
            conf->m_buy_pos -= g_order->Volume;
        }
        else //开
        {
            conf->m_sell_pos += g_order->Volume;
        }
//printf("sel %f, %f, fee: %f  pl:%f\\n", conf->m_money, money, fee, conf->m_money + money);
        //维护PL
        conf->m_money = conf->m_money + money - fee;
        conf->m_order += 1;


    }
}


void do_bid(g_status* conf, CUstpFtdcInputOrderField* g_order)
{
    int left_pos_size = conf->m_max_pos - (conf->m_sell_pos - conf->m_buy_pos);
    if(left_pos_size <=0)
        return;

    g_order->Volume = left_pos_size;
    if(conf->m_buy_pos >= left_pos_size)
    {
        g_order->OffsetFlag = 3; //平
    }
    else
        g_order->OffsetFlag = 0; //开

     g_order->Direction =1; //卖

    deal_order(conf, g_order);
}

void do_ask(g_status* conf, CUstpFtdcInputOrderField* g_order)
{
    int left_pos_size = conf->m_max_pos - (conf->m_buy_pos - conf->m_sell_pos);
    if(left_pos_size <=0)
        return;

    g_order->Volume = left_pos_size;
    if(conf->m_sell_pos >= left_pos_size)
    {
        g_order->OffsetFlag = 3; //平
    }
    else
        g_order->OffsetFlag = 0; //开

     g_order->Direction = 0; //买

    deal_order(conf, g_order);

}

/****************************************************************************
*  Kernal for signal's parameters
*  param_count: 4-aligned
****************************************************************************/
__kernel void signal_parameters_simulation (
    __global const float4* param_data, /* param_count/4 * param_group */
    __global const float4* sig_data,   /* param_count/4 * msg_count */
    //__global const ChinaL1Msg* msg_data,/* msg_count */
    //__global const float3* mkt_data,    /* msg_count */
    __global OutputMsg* output,         /* param_group */
    __global float* debug_log,
    //const float g_signal_multiple,      /* initialied multiple */
    const int msg_count,                /* sizeof msg */
    const int param_count,              /* sizeof parameter */
    const int param_group,              /* sizeof param_data */
    const int device_type,              /* 0-CPU    1-GPU */
    __global const $:(vec_type)* mkt_mid,    /* $:(clvec)-vectorized mid data */
    __global const $:(vec_type)* mkt_ask,    /* $:(clvec)-vectorized ask data */
    __global const $:(vec_type)* mkt_bid,     /* $:(clvec)-vectorized bid data */
    __global const float* last_ask,    /* last ask price every session */
    __global const float* last_bid,    /* last bid price every session */
    __global const int* trd_lv,
    __global const int* session_list,
    const int session_count
    )
{
    int param_pos = get_global_id(0);    /* current position of parameter set */
    int j;
    int i;
    int scur;
    int sstart;
    int send;

    CUstpFtdcInputOrderField g_order;
    g_status conf;

    if( param_pos >= param_group)
        return;
//    if(param_pos != 0)
//    return;
    //printf("session_cont=%d msg_count=%d\\n", session_count, msg_count);


    output[param_pos].m_result=0.0f;
    output[param_pos].m_order=0;
    sstart = 0;
    send = 0;
    
    for(scur = 0; scur < session_count; ++scur)
    {
        conf.m_max_pos = 1;
        conf.m_buy_pos = 0;
        conf.m_sell_pos = 0;
        conf.m_multiple= 5;
        conf.m_fee_rate= 0.0f;//0.0001f;
        conf.m_fee_vol = 3.03f;
        conf.m_money = 0.0f;
        conf.m_order = 0;
        sstart += send;
        send = session_list[scur];
        //printf("start:%d\\tsend:%d\\n", sstart, send);
        //continue;

        for(j = sstart / $:(clvec); j < (sstart+send) / $:(clvec); ++j)
        {
            int msg_pos = j*$:(clvec);
            int pm_pos = param_pos* $:(param_count/4);

            $:(vec_type) signal = ($:(vec_type)) (
                    $:str("\\n\\t\\t").join("dot(param_data[pm_pos+{pm_idx}], sig_data[(msg_pos + {msg_idx} )*{msg_range} + {pm_idx}]){seps}".format(msg_range=msg_range, pm_idx=pm_idx, msg_idx=msg_idx, seps='+' if pm_idx < msg_range-1 else "" if msg_idx == clvec-1 else ",\\n" ) for msg_idx in range(clvec) for pm_idx in range(msg_range) ) );

            //signal = signal * 2.2f;
            $:(vec_type) forecast = mad(mkt_mid[j], signal, mkt_mid[j]);
            float* pfc = (float*)&forecast;
            __global float* pask = (__global float*)&(mkt_ask[j]);
            __global float* pbid = (__global float*)&(mkt_bid[j]);
    $ ask = 'pask[i]'
    $ bid = 'pbid[i]'
    $ fc = 'pfc[i]'
    $if clvec == 1:
        $ ask = '*pask'
        $ bid = '*pbid'
        $ fc = '*pfc'
    $#end-if
            for(i=0; i<$:(clvec);++i)
            {
//                if(trd_lv[msg_pos+i] == 0)
//                    continue;

                if($(fc) > $(ask))
                {
                    g_order.LimitPrice = $(ask);
                    do_ask(&conf, &g_order);
                }
                else if($(fc) < $(bid))
                {
                    g_order.LimitPrice = $(bid);
                    do_bid(&conf, &g_order);
                }
            }

//            if(j > 26110 / $:(clvec)) {
//            printf("sig: %f\\t%f, mid:%f\\t%f\\t%f\\n",
//                forecast[0],signal[0],
//                mkt_mid[j][0], mkt_ask[j][0], mkt_bid[j][0]
//                );
//            }
        }//end for-j
        
        flatten(&conf,//2400.0f,2400.0f
            last_ask[scur],last_bid[scur]
            );
 
        //printf("mon %f ord:%d\\n", conf.m_money, conf.m_order);
        output[param_pos].m_result += conf.m_money;
        output[param_pos].m_order += conf.m_order;
    }//end for-scur
    //printf("mon %f ord:%d\\n", output[param_pos].m_result, output[param_pos].m_order);
}


