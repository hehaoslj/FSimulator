#pragma OPENCL EXTENSION cl_amd_printf : enable
#pragma OPENCL EXTENSION cl_intel_printf : enable
#pragma OPENCL EXTENSION cl_nv_printf : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
struct ChinaL1Msg
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
} __attribute__ ((aligned (8))) ;


inline float get_forecast(const __global double* prop, const __global double* sig, const unsigned int pc, const unsigned int dtype)
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
                rt += (*prop) * (*sig);
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


inline float get_forecast_d4(const __global double4* prop, const __global double4* sig, const unsigned int prop_count, const unsigned int dtype)
{
    int j;
    double rt = 0;
    int pc = prop_count/4;
    for(j=0; j<pc; ++j)
    {
    rt += dot(*prop, *sig);

    ++prop;
    ++sig;
    }
    return rt;
//    if(dtype)
//    {
//        do{
//           if(prop_count >= 8)
//            {
//                double8 pp = convert_double8(prop);
//                double8 ps = convert_double8(sig);
//                rt += dot(pp.hi, ps.hi) + dot(pp.lo, ps.lo);
//                prop += 8;
//                sig += 8;
//                prop_count -=8;
//            }
//            else if(prop_count >=4 )
//            {
//                double4 pp = as_double4(*prop);
//                double4 ps = as_double4(*sig);
//                rt += dot(*prop, *sig);
//                prop += 1;
//                sig += 1;
//                prop_count -=4;
//            }
//            else if(prop_count == 3)
//            {
//                //double3 pp = convert_double3(prop);
//                //double3 ps = convert_double3(sig);
//                //rt += dot(pp, ps);
//                prop += 3;
//                sig += 3;
//                prop_count -=3;
//            }
//            else if(prop_count == 2)
//            {
////                double2 pp = convert_double2_sat(prop);
////                double2 ps = convert_double2_sat(sig);
////                rt += dot(pp, ps);
//                prop += 2;
//                sig += 2;
//                prop_count -=2;
//            }
//            else if(prop_count == 1)
//            {
//                rt += (*prop) * (*sig);
//                prop += 1;
//                sig += 1;
//                prop_count -=1;
//            }
//        } while(prop_count >=4);
//    }
//    else
//    {
//        for(j=0; j<prop_count; ++j)
//        {
//            rt += prop[j]*sig[j];
//        }
//    }
//    return (float)rt;

}

inline float4 calc_forecast4(__global const double* prop_data,
    __global const double* sig_data,
    __global const float3* mkt_data,
    int group,
    int msg_pos,
    unsigned int prop_count,
    const unsigned int device_type)
{
    float4 sig4;
    sig4.x = get_forecast(prop_data + group*prop_count, sig_data+prop_count*(msg_pos*4), prop_count, device_type);
    sig4.y = get_forecast(prop_data + group*prop_count, sig_data+prop_count*(msg_pos*4+1), prop_count, device_type);
    sig4.z = get_forecast(prop_data + group*prop_count, sig_data+prop_count*(msg_pos*4+2), prop_count, device_type);
    sig4.w = get_forecast(prop_data + group*prop_count, sig_data+prop_count*(msg_pos*4+3), prop_count, device_type);



    float4 mid4 = (mkt_data[msg_pos*4].x, mkt_data[msg_pos*4+1].x, mkt_data[msg_pos*4+2].x, mkt_data[msg_pos*4+3].x);
    //float4 ask4 = (mkt_data[j*4].y, mkt_data[j*4+1].y, mkt_data[j*4+2].y, mkt_data[j*4+3].y);
    //float4 bid4 = (mkt_data[j*4].z, mkt_data[j*4+1].z, mkt_data[j*4+2].z, mkt_data[j*4+3].z);
    float4 forecast4 = mad(mid4, sig4, mid4);

    return forecast4;
}


__constant float4 norm=(float4)(1.0f,1.0f,1.0f,1.0f);
__constant int8 cvtfloat=(int8)(1065353215,1065353215,1065353215,1065353215,
1065353215,1065353215,1065353215,1065353215);

__kernel void signal_simulation(
    __global const double4* prop_data,   /* prop_count * prop_group */
    __global const double4* sig_data,    /* prop_count * msg_count*/
    /*__global const struct ChinaL1Msg* msg,  msg_count*/
    __global const float3* mkt_data,    /*msg_count */
    __global float* output,            /* prop_group*/
    const float g_signal_multiplier,
    const unsigned int msg_count,
    const unsigned int prop_count,
    const unsigned int prop_group,
    const unsigned int device_type,/* 0-CPU    1-GPU */
    __global const float8* mkt_mid,
    __global const float8* mkt_ask,
    __global const float8* mkt_bid)
{
    unsigned int group = get_global_id(0);
    unsigned int j;
    int i;

    if(group >= prop_group)
        return;
//    for(j=0; j<msg_count/4; ++j)
//    {
//        float4 sig4;
//        sig4.x = get_forecast_d4(prop_data + group*prop_count/4, sig_data+j*4*prop_count/4, prop_count, device_type);
//        sig4.y = get_forecast_d4(prop_data + group*prop_count/4, sig_data+j*4*prop_count/4+1, prop_count, device_type);
//        sig4.z = get_forecast_d4(prop_data + group*prop_count/4, sig_data+j*4*prop_count/4+2, prop_count, device_type);
//        sig4.w = get_forecast_d4(prop_data + group*prop_count/4, sig_data+j*4*prop_count/4+3, prop_count, device_type);



//        float4 mid4 = (mkt_data[j*4].x, mkt_data[j*4+1].x, mkt_data[j*4+2].x, mkt_data[j*4+3].x);
//        float4 ask4 = (mkt_data[j*4].y, mkt_data[j*4+1].y, mkt_data[j*4+2].y, mkt_data[j*4+3].y);
//        float4 bid4 = (mkt_data[j*4].z, mkt_data[j*4+1].z, mkt_data[j*4+2].z, mkt_data[j*4+3].z);
//        float4 forecast4 = mad(mid4, sig4, mid4);

//        for(i=0; i<4; ++i)
//        {
//            if( forecast4[i] > ask4[i])
//            {
//                output[group] = forecast4[i];
//            }
//            else if (forecast4[i] < bid4[i])
//            {
//                output[group] = forecast4[i];
//            }
//            else
//            {
//                //Do nothing
//                //printf("aaa\n");
//            }
//        }
//    }

//    for(j=0; j<msg_count; ++j)
//    {
//        float signal = get_forecast(prop_data + group*prop_count, sig_data+j*prop_count, prop_count, device_type);
////        double mid = (msg[j].m_bid+msg[j].m_offer) * 0.5;
////        double ask = msg[j].m_offer;
////        double bid = msg[j].m_bid;
////        double forecast = mid *(1+signal*g_signal_multiplier);
//        float mid = mkt_data[j].x;
//        float ask = mkt_data[j].y;
//        float bid = mkt_data[j].z;
//        float forecast = mad(mid, signal, mid);//mid *(1+signal);

//        if( isnan(signal) )
//        {
//            printf("error nan\n");
//        }
//        if(forecast > ask)
//        {
//            output[group] = forecast;
//        }
//        else if (forecast < bid)
//        {
//            output[group] = forecast;
//        }
//        else
//        {
//            //Do nothing
//            //printf("aaa\n");
//        }
//    }

//    for(j=0; j<msg_count/4; ++j)
//    {
//        float4 forecast4 = calc_forecast4(prop_data,
//        sig_data,
//        mkt_data,
//        group,
//        j,
//        prop_count,
//        device_type);
//        for(i=0; i<4; ++i)
//        {
//            float ask = mkt_data[j*4+i].y;
//            float bid = mkt_data[j*4+i].z;
//            float forecast = forecast4[i];
//            if(forecast > ask)
//            {
//                output[group] = forecast;
//            }
//            else if (forecast < bid)
//            {
//                output[group] = forecast;
//            }
//            else
//            {
//                //Do nothing
//                //printf("aaa\n");
//            }
//        }
//    }

//    for(j=0; j<msg_count; ++j)
//    {
//        float signal = get_forecast_d4(prop_data + group*prop_count/4, sig_data+j*prop_count/4, prop_count, device_type);
////        double mid = (msg[j].m_bid+msg[j].m_offer) * 0.5;
////        double ask = msg[j].m_offer;
////        double bid = msg[j].m_bid;
////        double forecast = mid *(1+signal*g_signal_multiplier);
//        float mid = mkt_data[j].x;
//        float ask = mkt_data[j].y;
//        float bid = mkt_data[j].z;
//        float forecast = mad(mid, signal, mid);//mid *(1+signal);

//        if( isnan(signal) )
//        {
//            printf("error nan\n");
//        }
//        if(forecast > ask)
//        {
//            output[group] = forecast;
//        }
//        else if (forecast < bid)
//        {
//            output[group] = forecast;
//        }
//        else
//        {
//            //Do nothing
//            //printf("aaa\n");
//        }
//    }
    output[group] = 0;
    __private int8 fc;
    float8 fcc;
    float8 forecast;
    float8 signal8;
    for(j=0; j<msg_count/8; ++j)
    {
        int msg_pos = j*8;
        int prop_pos = prop_count/4;
        signal8 =(float8)(
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+0)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+1)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+2)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+3)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+4)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+5)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+6)*prop_pos, prop_count, device_type),
//        get_forecast_d4(prop_data+group*prop_pos, sig_data+ (msg_pos+7)*prop_pos, prop_count, device_type)
                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+0)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+0)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+0)*prop_pos+2]) ,

                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+1)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+1)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+1)*prop_pos+2]) ,

                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+2)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+2)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+2)*prop_pos+2]) ,

                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+3)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+3)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+3)*prop_pos+2]) ,


                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+4)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+4)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+4)*prop_pos+2]) ,

                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+5)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+5)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+5)*prop_pos+2]) ,

                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+6)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+6)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+6)*prop_pos+2]) ,

                        dot(prop_data[group*prop_pos+0], sig_data[(msg_pos+7)*prop_pos+0]) +
                        dot(prop_data[group*prop_pos+1], sig_data[(msg_pos+7)*prop_pos+1]) +
                        dot(prop_data[group*prop_pos+2], sig_data[(msg_pos+7)*prop_pos+2])
        );
        forecast = mad(mkt_mid[j], signal8*g_signal_multiplier, mkt_mid[j]);
        //int8 ask_cnt = isgreater(forecast, mkt_ask[j]);
        //int8 bid_cnt = isless(forecast, mkt_bid[j]);
        //int8 fc = ask_cnt + bid_cnt;

        fc = (isgreater(forecast, mkt_ask[j]) + isless(forecast, mkt_bid[j]) ) * 1065353216;
        //fcc = convert_float8(fc);
        //output[group] += dot( fcc.hi, norm) + dot(fcc.lo, norm);


        //output[group] = fc[0]+fc[1]+fc[2]+fc[3]+fc[4]+fc[5]+fc[6]+fc[7];
        //fc *= 1065353216;
        //fc = mad_sat(fc, cvtfloat, fc);
        __private float4* pfc = (__private float4*)&fc;

        output[group] += dot( pfc[0], norm) + dot(pfc[1], norm);



//    uint2 tmp;
//    tmp.x = fc.hi
//        - ((fc.hi >> 1) & 033333333333)
//        - ((fc.hi >> 2) & 011111111111);
//    tmp.x = (tmp.x + (tmp.x >> 3)) & 030707070707;
//    tmp.y = fc.lo
//        - ((fc.lo >> 1) & 033333333333)
//        - ((fc.lo >> 2) & 011111111111);
//    tmp.y = (tmp.y + (tmp.y >> 3)) & 030707070707;
//    output[group] =  (tmp.x%63+tmp.y%63);

    }

}
