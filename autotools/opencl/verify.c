#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
}OutputMsg;

void flatten(g_status* conf, float ask, float bid);
void deal_order(g_status* conf, CUstpFtdcInputOrderField* g_order);
void do_bid(g_status* conf, CUstpFtdcInputOrderField* g_order);
void do_ask(g_status* conf, CUstpFtdcInputOrderField* g_order);

void flatten(g_status* conf, float ask, float bid)
{
    CUstpFtdcInputOrderField g_order;
    g_order.OffsetFlag = 3;//平
    printf("call flatten\n");
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

printf("buy \t%d\tprice:%f\t%d\n", g_order->OffsetFlag, g_order->LimitPrice, g_order->Volume);
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
        printf("sel \t%d\tprice:%f\t%d\n", g_order->OffsetFlag, g_order->LimitPrice, g_order->Volume);
//printf("sel %f, %f, fee: %f  pl:%f\n", conf->m_money, money, fee, conf->m_money + money);
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
        const int session_count)
{
    int param_pos = 0;
    int j;
    int i;
    int scur;
    int sstart;
    int send;

    CUstpFtdcInputOrderField g_order;
    g_status conf;

    output[param_pos].m_result=0;
    output[param_pos].m_order=0;
    sstart = 0;
    send = 0;
    for(scur = 0; scur < session_count; ++scur)
    {
        conf.m_max_pos = 2;
        conf.m_buy_pos = 0;
        conf.m_sell_pos = 0;
        conf.m_multiple=10;
        conf.m_fee_rate=0.0001f;
        conf.m_fee_vol = 0.0f;
        conf.m_money = 0.0f;
        conf.m_order = 0;
        sstart += send;
        send = session_list[scur];

        //continue;

        for(j = sstart; j < (sstart+send); ++j)
        {
            //printf("j=%d  %f\n", j, mkt_ask[j]);
            float signal = 0;
            for(i=0; i<param_count; ++i)
            {
                signal += sig_data[j*param_count + i] * param_data[i];
            }

            float forecast = (signal +1)*mkt_mid[j];
            float* pfc = (float*)&forecast;
            float* pask = (float*)&(mkt_ask[j]);
            float* pbid = (float*)&(mkt_bid[j]);


//            if(trd_lv[j] == 0)
//                continue;

            //
            if(*pfc > *pask)
            {
                //printf("ask fc:%f\t ask %f \tbid %f\n", forecast, *pask, *pbid);
                g_order.LimitPrice = *pask;
                do_ask(&conf, &g_order);
            }
            else if(*pfc < *pbid)
            {
                //printf("bid fc:%f\t ask %f \tbid %f\n", forecast, *pask, *pbid);
                g_order.LimitPrice = *pbid;
                do_bid(&conf, &g_order);
            }

        }//end for-j

        flatten(&conf,//2400.0f,2400.0f
            last_ask[scur],last_bid[scur]
            );
        //printf("mon %f ord:%d\\n", conf.m_money, conf.m_order);
        output[param_pos].m_result += conf.m_money;
        output[param_pos].m_order += conf.m_order;

        printf("start:%d\tsend:%d\torder:%d money:%f\t%d \t%d\n", sstart, send,
               conf.m_order, conf.m_money, conf.m_buy_pos, conf.m_sell_pos);
    }//end for-scur
}


