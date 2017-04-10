#include "clibapp.h"
#include "forecaster.h"
#include "guavaproto.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <string>
#include <vector>

#include <jansson.h> /** Jansson lib */

typedef struct ssh_config_s {

    int port;
    int authtype;
    std::string host;
    std::string username;
    std::string password;
    std::string root;

} ssh_config_t;

class Forecaster;
class ForecasterFactory;

json_t* obj_get(json_t* root, const char* keys) {
    json_t* o = root;
    char *key = NULL;
    size_t nsz = strlen(keys);
    char * nkeys = (char*)malloc(nsz+1);
    char* last=NULL;
    memcpy(nkeys, keys, nsz);
    nkeys[nsz] = '\0';

    for(key = strtok_r(nkeys, ".", &last);
        key;
        key = strtok_r(NULL, ".", &last))
    {
        /*printf("key:%s\n", key);*/
        o = json_object_get(o, key);


    }

    free(nkeys);
    return o;

}

static inline void get_json_string(json_t* root ,const char* key, std::string& value)
{
    json_t *obj = obj_get(root, key);
    if(json_is_string(obj)) {
        value = json_string_value(obj);
    }
}

template<class T>
static inline void get_json_integer(json_t* root, const char* key, T* value)
{
    json_t* obj = obj_get(root, key);
    if(json_is_integer(obj)) {
        *value = json_integer_value(obj);
    }
}



//int main(int argc, char* argv[]) {
//    json_t*root = NULL;
//    json_error_t err;
//    json_t *obj = NULL;
//    const char* key = NULL;
//    const char* value = NULL;
//    Forecaster *fc = NULL;
//    ssh_config_t param;


//    if (argc <= 1) {
//        printf("please input the config file\n");
//        return 0;
//    }

//    /** Load json conf */
//    root = json_load_file(argv[1], 0, &err);
//    if(root == NULL) {
//        printf("load config file error:%s at line %d, col %d\n", err.text, err.line, err.column);
//        return -1;
//    }
//    key = "conftype";
//    obj = json_object_get(root, key);
//    if(!json_is_string(obj))
//    {
//        printf("config key s[%s] is missing.\n", key);
//        return -1;
//    }

//    value = "signal_calc";
//    if( strcmp( json_string_value(obj), value ) != 0) {
//        printf("config key[%s] value[%s] is incorrect.\n", key, value);
//        return -1;
//    }


//    key="insttype";
//    obj = json_object_get(root, key);
//    if(!json_is_string(obj))
//    {
//        printf("config keys[%s] is missing.\n", key);
//        return -1;
//    }
//    value = json_string_value(obj);
//    {
//        struct tm date;
//        time_t t = time(NULL);
//        std::string type = value;
//        localtime_r(&t, &date);

//        fc  = ForecasterFactory::createForecaster( type, date );
//        std::string id = fc->get_trading_instrument();
//        printf("%p, trading instrument:%s\\n",
//               fc,
//               id.c_str());

//        std::vector<std::string> ids = fc->get_subscriptions();
//        for(size_t i=0; i< ids.size(); ++i) {
//            printf("id=%s\\n", ids[i].c_str());
//        }
//        double* value;
//        int size = 0;
//        ids = fc->get_subsignal_name();
//        printf("signal size:%d\\n", ids.size());
//        for(size_t i=0; i< ids.size(); ++i) {
//            printf("id=%s\\n", ids[i].c_str());
//        }
//        value = (double*)malloc(sizeof(double)*ids.size());
//        fc->get_all_signal(&value, &size);
//        printf("signal size:%d\\n", size);

//    }
//    /** Calculate prepare */
//    const char* begindate;
//    const char* enddate;
//    key="output.begindate";
//    obj = obj_get(root, key);
//    if(!json_is_string(obj)) {
//        printf("config key s[%s] is missing.\n", key);
//        return -1;
//    }
//    begindate = json_string_value(obj);


//    key = "output.enddate";
//    obj = obj_get(root, key);
//    if(!json_is_string(obj)) {
//        printf("config key s[%s] is missing.\n", key);
//        return -1;
//    }
//    enddate = json_string_value(obj);


//    get_json_string(root, "ssh.host", param.host);
//    get_json_string(root, "ssh.username", param.username);
//    get_json_string(root, "ssh.password", param.password);
//    get_json_string(root, "ssh.root", param.root);
//    get_json_integer<int>(root, "ssh.port", &param.port);
//    get_json_integer<int>(root, "ssh.authtype", &param.authtype);
//    if(param.host.size() > 0) {
//        printf("using ssh :\\n"
//               "\\thost:\\t%s\\n"
//               "\\tport:\\t%d\\n"
//               "\\tuser:\\t%s\\n"
//               "\\tpass:\\t%s\\n"
//               ,param.host.c_str(),
//               param.port,
//               param.username.c_str(),
//               param.password.c_str());
//    }


//    printf("calc begin date:%s to end date:%s\n", begindate, enddate);

//    json_decref(root);
//    //delete fc;

//    return 0;

//}

std::string& replace_all(std::string& str,const std::string& old_value,const std::string&   new_value)
{
    while(true)
    {
        std::string::size_type   pos(0);
        if(   (pos=str.find(old_value))!=std::string::npos   )
            str.replace(pos,old_value.length(),new_value);
        else
            break;
    }
    return   str;
}


/** Implement clib */

typedef struct _internal_forecaster_s {
    Forecaster *fc;
    std::string stype;
    std::string trading_instrument;
    std::vector<std::string> subscriptions;
    std::vector<std::string> subsignals;
    //std::vector<FILE*> subfiles;
    FILE* instfp;
    FILE* sigsfp; //forecast X
    FILE* fcfp;   //forecast Y
    ChinaL1Msg trad_msg;
    std::vector<ChinaL1Msg> sub_msgs;
    std::vector<double> sub_sigs;
    bool is_trading;
    bool is_symbol;
    std::vector<double> bidl1;
    std::vector<double> askl1;
    std::vector<std::pair<double, int> > forecast;
    uint32_t duration;
} in_forecaster_t;

forecaster_t fc_create(const char* type)
{
    in_forecaster_t * in =  NULL;
    Forecaster *fc = NULL;
    struct tm date;
    time_t t;
    t = time(NULL);
    localtime_r(&t, &date);
    in = new in_forecaster_t;
    in->stype = type;

    fc  = ForecasterFactory::createForecaster( in->stype, date );

    printf("guava size:%ld\n", sizeof(guava_udp_head)+sizeof(guava_udp_normal));
    printf("SHFEQuotDataT size%ld\n", sizeof(SHFEQuotDataT));

    in->fc = fc;
    in->trading_instrument = fc->get_trading_instrument();
    Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&in->trad_msg;
    msg_dt.m_inst = in->trading_instrument;
    printf("trade = %s\n", in->trading_instrument.c_str());
    in->subscriptions = fc->get_subscriptions();
    for(size_t i=0; i<in->subscriptions.size(); ++i)
    {
        ChinaL1Msg msg;
        Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg;
        msg_dt.m_inst = in->subscriptions[i];
        in->sub_msgs.push_back(msg);
        printf("sub [%lu] = %s\n", i, in->subscriptions[i].c_str());
    }
    in->subsignals = fc->get_subsignal_name();
    in->sub_sigs.resize(in->subsignals.size());

    return in;
}

void fc_delete(forecaster_t fc)
{
    (void)fc;
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    delete in;
}

int fc_set_duration(forecaster_t fc, int dur)
{
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    in->duration = dur;
    return 0;
}


int fc_get_trading_instrument(forecaster_t fc, const char **instrument)
{
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    *instrument = in->trading_instrument.c_str();
    return in->trading_instrument.size();
}

int fc_get_instruments_size(forecaster_t fc)
{
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    return in->subscriptions.size();
}

int fc_get_instruments(forecaster_t fc, int pos, const char **instrument)
{
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    if(pos >=0 && pos < in->subscriptions.size())
    {
        const std::string& str = in->subscriptions[pos];
        *instrument = str.c_str();
        return str.size();
    }
    return 0;
}


int fc_get_subsignals_size(forecaster_t fc)
{
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    return in->subsignals.size();
}


int fc_get_subsignals(forecaster_t fc, int pos, const char** signame)
{
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    if(pos >=0 && pos < in->subsignals.size())
    {
        const std::string& str = in->subsignals[pos];
        *signame = str.c_str();
        return str.size();
    }
    return 0;
}


struct pcap_pkghdr {
    unsigned int tv_sec;
    unsigned int tv_usec;
    unsigned int size;
    unsigned int length;
};
#pragma pack(push, 1)
struct pcap_pkgdat {
    char udp_head[42];
    guava_udp_head head;
    union {
        guava_udp_normal normal;
        guava_udp_summary summary;
    };
};

typedef pcap_pkgdat pcap_guava1;

struct pcap_pkgguava2 {
    char udp_head[42];
    SHFEQuotDataT quot;
};

#pragma pack(pop)

#define read_in(pos, sz) fread(buf+pos, sz, 1 ,fp)
#define read_filehdr() read_in(0, 24)
#define read_pkghdr() read_in(0, 16)
#define read_pkgdat() read_in(16, hdr->size)

enum e_order_state {
    ST_ASK = 1<<0,
    ST_BID = 1<<1,
    ST_INSERT = 1<<2
};

struct st_order_state {
    int cur_order_state;
    double cur_order_px;
    double his_order_px;
    int64_t cur_order_tm;
    int ask_count;
    int bid_count;
    int sound_ask_count;
    int sound_bid_count;
};

inline void write_signalindex(const in_forecaster_t* in, std::string& name)
{

    printf("index file name: %s\n", name.c_str());
    FILE* fidx = fopen(name.c_str(), "w");
    fprintf(fidx, "{\n"
            "\"size\":%ld,\n"
            "\"names\":[\n",
            in->subsignals.size());
    size_t i;
    if(in->subsignals.size()>0)
    {
        fprintf(fidx, "\t\"%s\"\n",
                in->subsignals[0].c_str());
    }
    for(i=1;i<in->subsignals.size();++i)
    {
        fprintf(fidx, "\t,\"%s\"\n",
                in->subsignals[i].c_str());
    }
    fprintf(fidx, "]\n}\n");
    fclose(fidx);
}

inline void write_instrument(FILE* fp, const ChinaL1Msg& msg_data)
{
    /* Total 68 bytes:  instrument -- 8 bytes  data -- 60 bytes
    */
    const Dummy_ChinaL1Msg& msg_dt =
            *reinterpret_cast<const Dummy_ChinaL1Msg*>(&msg_data);
    int64_t inst = 0;
    memcpy(&inst, msg_dt.m_inst.c_str(), msg_dt.m_inst.size());
    fwrite(&inst, 8,1, fp);                   //    8 bytes
    fwrite(&(msg_dt.m_time_micro), 8, 8, fp); //    64bytes
}

inline void write_subsignals(in_forecaster_t* in, const ChinaL1Msg& msg_data)
{
    double *rdata = &in->sub_sigs[0];
    int rsize = 0;
    int i;
    double signal;
    FILE* fp = in->sigsfp;
    const Dummy_ChinaL1Msg& msg_dt =
            *reinterpret_cast<const Dummy_ChinaL1Msg*>(&msg_data);

    signal = in->fc->get_forecast();
    in->fc->get_all_signal(&rdata, &rsize);



    double mid = 0.5 * ( msg_dt.m_offer + msg_dt.m_bid );
    double forecast = mid * ( 1 + signal);
    int pos = (1<<0);
    if(forecast > msg_dt.m_offer) pos = (1<<1);
    else if(forecast < msg_dt.m_bid) pos = (1<<2);

    if(signal >= 0)
        pos += (1<<3);

    //Store bid/ask
    in->askl1.push_back(msg_dt.m_offer);
    in->bidl1.push_back(msg_dt.m_bid);
    in->forecast.push_back(
                std::pair<double,int>(mid, pos));

    //Write subsignals
    fprintf(fp, "\"%lld\", %016.014lf",
            msg_dt.m_time_micro,
            signal);

    for(i=0; i< rsize; ++i)
    {
        fprintf(fp, ",%016.014lf", *(rdata+i) );
    }
    fprintf(fp, "\n");

    //Write forecast
    fwrite(&in->sub_sigs[0], 1, sizeof(double)*rsize, in->fcfp);

//    if(in->forecast.size() > in->duration)
//    {
//        double maxmid =0;
//        double minmid = 0;
//        for(int i = in->duration; i>0;--i)
//        {
//            auto a = (in->askl1.end() -i);
//            auto b = (in->bidl1.end() -i);
//            double mid = (*a + *b)/2;
//            if(maxmid < mid || maxmid == 0)
//                maxmid = mid;
//            if(minmid > mid || minmid == 0)
//                minmid = mid;
//        }
//        auto f = in->forecast.end() - in->duration;
//        double dis1 = maxmid - f->first;
//        double dis2 = f->first - minmid;
//        if(dis1 > dis2)
//            fprintf(in->fcfp, "%16.014lf\n", (maxmid / f->first) - 1.0 );
//        else
//            fprintf(in->fcfp, "%16.014lf\n", (minmid / f->first) - 1.0 );
//        //printf("%16.014lf %16.014lf %16.014lf %16.014lf\n",
//        //       dis1,dis2, *(in->askl1.end()-1), *(in->bidl1.end()-1) );
////        if(f->second & (1<<3))
////        {
////            //Max mid
////            fprintf(in->fcfp, "%16.014lf\n", (maxmid / f->first) - 1.0 );
////            //if(f->first - maxmid > 0)
////                printf("MAX:MID, FC CRI\n%16.014lf %16.014lf\n",
////                       f->first, maxmid);
////        } else {
////            //Min mid
////            fprintf(in->fcfp, "%16.014lf\n", (minmid / f->first) - 1.0 );
////            //if(f->first - minmid < 0)
////                printf("MIN:MID, FC CRI\n%16.014lf %16.014lf\n",
////                       f->first, minmid);
////        }

//    }


}

inline void proc_l1msg(in_forecaster_t* in, const ChinaL1Msg& msg_data)
{

    //Calc signals
    in->fc->update(msg_data);

    if(!in->is_trading)
        return;


    //Save ChinaL1Msg
    write_instrument(in->instfp, msg_data);

    //Save signals
    write_subsignals(in, msg_data);
}

inline void proc_guava1(in_forecaster_t* in, char* buf, FILE* fp) {
    pcap_pkghdr* hdr=reinterpret_cast<pcap_pkghdr*>(buf);
    pcap_guava1* data = reinterpret_cast<pcap_guava1*>(buf+16);
    ChinaL1Msg *pmsg_data;
    const uint64_t dmax = 0x7fefffffffffffff;
    /* file header */
    read_filehdr();
    do {
        /* package header */
        read_pkghdr();

        /* package data */
        read_pkgdat();

        if(hdr->size == 42+sizeof(guava_udp_head)+sizeof(guava_udp_normal))
        {
            in->is_symbol = false;
            in->is_trading=false;
            if(strcmp(in->trading_instrument.c_str(), data->head.m_symbol) == 0)
            {
                in->is_symbol = true;
                in->is_trading = true;
                pmsg_data = &in->trad_msg;
            }
            for(size_t i=0; i< in->subscriptions.size(); ++i) {
                if(strcmp(in->subscriptions[i].c_str(), data->head.m_symbol) == 0)
                {
                    in->is_symbol = true;
                    pmsg_data = &in->sub_msgs[i];
                    break;
                }
            }

            if(!in->is_symbol)
            {
                continue;
            }
            //printf("%s. %s\n", in->trading_instrument.c_str(), data->head.m_symbol);

            if( (*(const uint64_t*)(&data->normal.m_ask_px) != dmax) &&
                    (*(const uint64_t*)(&data->normal.m_bid_px) != dmax) &&
                    (*(const uint64_t*)(&data->normal.m_last_px) != dmax) )
            {
                /** Calculate */
                ChinaL1Msg &msg_data = *pmsg_data;
                Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg_data;
                int64_t micro_time = (int64_t)hdr->tv_sec*1000000LL+data->head.m_millisecond*1000;


                //msg_dt.m_inst = data->head.m_symbol;
                //msg_dt.m_time_micro = dtime*1000000+data->head.m_millisecond*1000;
                msg_dt.m_time_micro = micro_time;

                msg_dt.m_limit_up = 0;
                msg_dt.m_limit_down = 0;

                switch((int)data->head.m_quote_flag)
                {
                case 1:
                    //msg_dt.m_bid = data->normal.m_bid_px;
                    //msg_dt.m_offer = data->normal.m_ask_px;
                    //msg_dt.m_bid_quantity = data->normal.m_bid_share;
                    //msg_dt.m_offer_quantity = data->normal.m_ask_share;
                    msg_dt.m_volume = data->normal.m_last_share;//guava1 no-data md_data->PriVol.Volume ;
                    msg_dt.m_notional = data->normal.m_total_value;//md_data->PriVol.Turnover;
                    break;
                case 2:
                    msg_dt.m_bid = data->normal.m_bid_px;
                    msg_dt.m_offer = data->normal.m_ask_px;
                    msg_dt.m_bid_quantity = data->normal.m_bid_share;
                    msg_dt.m_offer_quantity = data->normal.m_ask_share;
                    //msg_dt.m_volume = 0;//guava1 no-data md_data->PriVol.Volume ;
                    //msg_dt.m_notional = data->normal.m_total_pos;//md_data->PriVol.Turnover;
                    break;
                case 3:
                    msg_dt.m_bid = data->normal.m_bid_px;
                    msg_dt.m_offer = data->normal.m_ask_px;
                    msg_dt.m_bid_quantity = data->normal.m_bid_share;
                    msg_dt.m_offer_quantity = data->normal.m_ask_share;
                    msg_dt.m_volume = data->normal.m_last_share;//guava1 no-data md_data->PriVol.Volume ;
                    msg_dt.m_notional = data->normal.m_total_value;//md_data->PriVol.Turnover;
                    break;
                case 0:
                    break;
                case 4:
                    continue;

                }

                proc_l1msg(in, msg_data);
            }
        }

    } while(feof(fp) == 0);
}

inline void proc_guava2(in_forecaster_t* in, char* buf, FILE* fp) {
    pcap_pkghdr* hdr=reinterpret_cast<pcap_pkghdr*>(buf);
    pcap_pkgguava2* data = reinterpret_cast<pcap_pkgguava2*>(buf+16);
    ChinaL1Msg *pmsg_data;
    const uint64_t dmax = 0x7fefffffffffffff;
    /* file header */
    read_filehdr();
    do {
        /* package header */
        read_pkghdr();
        //break;

        //        printf("sec :%u, usec:%u\n", hdr->tv_sec, hdr->tv_usec);
        //        if(cnt > 100)
        //            break;
        read_pkgdat();

        //SHFEQuotDataTag* s = reinterpret_cast<SHFEQuotDataTag*>(buf);
        //printf("%s, %s\t%d:\tsz:%d\t in1 : %s\t in2: %s\n", cnt, sz, data->head.m_symbol, s->InstrumentID);
        if(hdr->size != sizeof(pcap_pkgguava2))
            continue;

        in->is_symbol = false;
        in->is_trading=false;
        if(strcmp(in->trading_instrument.c_str(), data->quot.InstrumentID) == 0)
        {
            in->is_symbol = true;
            in->is_trading = true;
            pmsg_data = &in->trad_msg;
        } else {
            for(size_t i=0; i< in->subscriptions.size(); ++i) {
                if(strcmp(in->subscriptions[i].c_str(), data->quot.InstrumentID) == 0)
                {
                    in->is_symbol = true;
                    pmsg_data = &in->sub_msgs[i];
                    break;
                }
            }
        }

        if(!in->is_symbol)
        {
            continue;
        }

        if( (*(const uint64_t*)(&data->quot.AskPrice1) != dmax) &&
                (*(const uint64_t*)(&data->quot.BidPrice1) != dmax) &&
                (*(const uint64_t*)(&data->quot.LastPrice) != dmax) )
        {
            /** Calculate */
            ChinaL1Msg &msg_data = *pmsg_data;
            Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg_data;
            int64_t micro_time = (int64_t)hdr->tv_sec*1000000+data->quot.UpdateMillisec;//hdr->tv_usec;

            //Update ChinaL1Msg
            msg_dt.m_time_micro = micro_time;

            msg_dt.m_limit_up = 0;
            msg_dt.m_limit_down = 0;

            msg_dt.m_bid = data->quot.BidPrice1;
            msg_dt.m_offer = data->quot.AskPrice1;
            msg_dt.m_bid_quantity = data->quot.BidVolume1;
            msg_dt.m_offer_quantity = data->quot.AskVolume1;
            msg_dt.m_volume = data->quot.Volume;//Volume ;
            msg_dt.m_notional = data->quot.Turnover;//Turnover;


            proc_l1msg(in, msg_data);
        }

    } while(feof(fp) == 0);

}
int fc_calc_subsignals(forecaster_t fc, const char* dtfile, const char* outpath, const char* namepattern)
{
    char buf[256];
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    FILE* fp;

    fp = fopen(dtfile, "rb");
    if(!fp)
        return EBADF;

    /** Clear in*/
    in->askl1.clear();
    in->bidl1.clear();
    in->forecast.clear();

    std::string sdate;
    std::string stime;
    {
        std::string dfile = dtfile;
        std::string::size_type pos(0);
        pos = dfile.find("__", 0);
        if(pos == std::string::npos)
        {
            printf("filename havn't __ splited date and time [%s]\n", dtfile);
            return 0;
        }
        sdate = dfile.substr(pos-10, 10);
        stime = dfile.substr(pos+2, 8);
    }

    {
        std::string name = namepattern;
        name = replace_all(name, "%type", in->stype);

        name = replace_all(name, "%date", sdate);
        name = replace_all(name, "%time", stime);
        name = "/" + name;
        name = outpath + name+".csv";

        printf("sig file name :%s\n", name.c_str());
        in->sigsfp = fopen(name.c_str(), "w");
        fprintf(in->sigsfp, "\"Date\",\"Signal\"");
        for(size_t i=0; i< in->subsignals.size(); ++i)
        {
            fprintf(in->sigsfp, ",\"%s\"",
                    in->subsignals[i].c_str());
        }
        fprintf(in->sigsfp, "\n");

        name.replace(name.end() -4, name.end(), ".dat");
        printf("inst file name: %s\n", name.c_str());
        in->instfp = fopen(name.c_str(), "wb");

        name.replace(name.end() -4, name.end(), ".fc");
        printf("forecast file name: %s\n", name.c_str());
        in->fcfp = fopen(name.c_str(), "w");

        /// subsignal index file
        name.replace(name.end()-3, name.end(), ".idx");
        write_signalindex(in, name);

    }

    const char* p = strstr(dtfile, "guava2");
    if( !p )
    {
        proc_guava1(in, buf, fp);
    }
    else
    {
        proc_guava2(in, buf, fp);
    }

    fclose(fp);
    fclose(in->sigsfp);
    fclose(in->instfp);
    fclose(in->fcfp);
    return 0;
}

//int fc_calc_subsignals2(forecaster_t fc, const char* dtfile, const char* outpath, const char* namepattern)
//{
//    char buf[256];
//    pcap_pkghdr* hdr=reinterpret_cast<pcap_pkghdr*>(buf);
//    pcap_pkgdat* data = reinterpret_cast<pcap_pkgdat*>(buf+16);
//    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
//    FILE* fp;
//    printf("fc_calc_subsignals data:\n file %s\noutpath:%s\npattern:%s\nsz:%ld\nhdr:%ld\n",
//           dtfile, outpath, namepattern,
//           sizeof(pcap_pkgdat), sizeof(pcap_pkghdr));
//    fp = fopen(dtfile, "rb");
//    if(!fp)
//        return EBADF;
//    std::string sdate;
//    std::string stime;
//    {
//        std::string dfile = dtfile;
//        std::string::size_type pos(0), pos_dt(0);
//        pos = dfile.find("__", 0);
//        if(pos == std::string::npos)
//        {
//            printf("no __\n");
//            return 0;
//        }
//        sdate = dfile.substr(pos-10, 10);
//        stime = dfile.substr(pos+2, 8);
//    }

//    FILE* fpsigs = 0;
//    {
//        std::string name = namepattern;
//        name = replace_all(name, "%type", in->stype);

//        name = replace_all(name, "%date", sdate);
//        name = replace_all(name, "%time", stime);
//        name = "/" + name;
//        name = outpath + name;
//        printf("sig file name :%s\n", name.c_str());
//        fpsigs = fopen(name.c_str(), "w");
//        fprintf(fpsigs, "\"Date\",\"Index\",\"Signal\"");
//        for(size_t i=0; i< in->subsignals.size(); ++i)
//        {
//            fprintf(fpsigs, ",\"%s\"",
//                    in->subsignals[i].c_str());
//        }
//        fprintf(fpsigs, "\n");
//    }

//    st_order_state cur_st;//current order state
//    memset(&cur_st, 0, sizeof(cur_st));
//    cur_st.cur_order_state = -1;

//    int cnt =0;
//    ChinaL1Msg *pmsg_data;
//    double *rdata = new double[in->subsignals.size()+1];
//    int rsize = in->subsignals.size();
//    /* file header */
//    read_filehdr();
//    do {
//        /* package header */
//        read_pkghdr();
//        //break;

//        //        printf("sec :%u, usec:%u\n", hdr->tv_sec, hdr->tv_usec);
//        //        if(cnt > 100)
//        //            break;
//        read_pkgdat();

//        //SHFEQuotDataTag* s = reinterpret_cast<SHFEQuotDataTag*>(buf);
//        //printf("%s, %s\t%d:\tsz:%d\t in1 : %s\t in2: %s\n", cnt, sz, data->head.m_symbol, s->InstrumentID);
//        if(hdr->size == 42+sizeof(guava_udp_head)+sizeof(guava_udp_normal))
//        {
//            bool is_symbol = false;
//            bool is_trading=false;
//            if(strcmp(in->trading_instrument.c_str(), data->head.m_symbol) == 0)
//            {
//                is_symbol = true;
//                is_trading = true;
//                pmsg_data = &in->trad_msg;
//            }
//            for(size_t i=0; i< in->subscriptions.size(); ++i) {
//                if(strcmp(in->subscriptions[i].c_str(), data->head.m_symbol) == 0)
//                {
//                    is_symbol = true;
//                    pmsg_data = &in->sub_msgs[i];
//                    break;
//                }
//            }

//            if(!is_symbol)
//            {
//                continue;
//            }
//            //printf("%s. %s\n", in->trading_instrument.c_str(), data->head.m_symbol);

//            const uint64_t dmax = 0x7fefffffffffffff;
//            if( (*(const uint64_t*)(&data->normal.m_ask_px) != dmax) &&
//                    (*(const uint64_t*)(&data->normal.m_bid_px) != dmax) &&
//                    (*(const uint64_t*)(&data->normal.m_last_px) != dmax) )
//            {
//                /** Calculate */
//                ChinaL1Msg &msg_data = *pmsg_data;
//                Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg_data;
//                int64_t micro_time = (int64_t)hdr->tv_sec*1000000+hdr->tv_usec;

//                //msg_dt.m_inst = data->head.m_symbol;
//                //msg_dt.m_time_micro = dtime*1000000+data->head.m_millisecond*1000;
//                msg_dt.m_time_micro = micro_time;
//                //                msg_dt.m_bid = data->normal.m_bid_px;
//                //                msg_dt.m_offer = data->normal.m_ask_px;
//                //                msg_dt.m_bid_quantity = data->normal.m_bid_share;
//                //                msg_dt.m_offer_quantity = data->normal.m_ask_share;
//                //                msg_dt.m_volume = 0;//guava1 no-data md_data->PriVol.Volume ;
//                //                msg_dt.m_notional = data->normal.m_total_pos;//md_data->PriVol.Turnover;
//                msg_dt.m_limit_up = 0;
//                msg_dt.m_limit_down = 0;

//                switch((int)data->head.m_quote_flag)
//                {
//                case 1:
//                    //msg_dt.m_bid = data->normal.m_bid_px;
//                    //msg_dt.m_offer = data->normal.m_ask_px;
//                    //msg_dt.m_bid_quantity = data->normal.m_bid_share;
//                    //msg_dt.m_offer_quantity = data->normal.m_ask_share;
//                    msg_dt.m_volume = data->normal.m_total_pos;//guava1 no-data md_data->PriVol.Volume ;
//                    msg_dt.m_notional = data->normal.m_total_value;//md_data->PriVol.Turnover;
//                    break;
//                case 2:
//                    msg_dt.m_bid = data->normal.m_bid_px;
//                    msg_dt.m_offer = data->normal.m_ask_px;
//                    msg_dt.m_bid_quantity = data->normal.m_bid_share;
//                    msg_dt.m_offer_quantity = data->normal.m_ask_share;
//                    //msg_dt.m_volume = 0;//guava1 no-data md_data->PriVol.Volume ;
//                    //msg_dt.m_notional = data->normal.m_total_pos;//md_data->PriVol.Turnover;
//                    break;
//                case 3:
//                    msg_dt.m_bid = data->normal.m_bid_px;
//                    msg_dt.m_offer = data->normal.m_ask_px;
//                    msg_dt.m_bid_quantity = data->normal.m_bid_share;
//                    msg_dt.m_offer_quantity = data->normal.m_ask_share;
//                    msg_dt.m_volume = data->normal.m_total_pos;//guava1 no-data md_data->PriVol.Volume ;
//                    msg_dt.m_notional = data->normal.m_total_value;//md_data->PriVol.Turnover;
//                    break;
//                case 0:
//                    break;
//                case 4:
//                    continue;

//                }

//                double signal = 0;

//                in->fc->update(msg_data);
//                if(!is_trading)
//                    continue;
//                signal = in->fc->get_forecast();
//                double mid = 0.5 * ( msg_dt.m_offer + msg_dt.m_bid );
//                double forecast = mid * ( 1 + signal*1.2);

//                rsize = 0;
//                //printf("call get_all_signal %d\n", rsize);
//                in->fc->get_all_signal(&rdata, &rsize);
//                //printf("called get_all_signal %d\n", rsize);
//                FILE* fp = fpsigs;
//                fprintf(fp, "\"%s %03d\",%d,%016.014lf",
//                        data->head.m_update_time,
//                        data->head.m_millisecond,
//                        cnt,
//                        signal);

//                for(size_t i=0; i< rsize; ++i)
//                {
//                    fprintf(fp, ",%016.014lf", *(rdata+i) );
//                }
//                fprintf(fp, "\n");
//                if( cur_st.cur_order_state == (ST_ASK|ST_INSERT) )
//                {
//                    //update the high px
//                    if(cur_st.his_order_px < msg_dt.m_offer)
//                    {
//                        cur_st.his_order_px = msg_dt.m_offer;
//                    }

//                    //time check
//                    if(cur_st.cur_order_tm+1000*15000LL > msg_dt.m_time_micro)
//                    {
//                        if(cur_st.cur_order_px*(1+0.0008) < msg_dt.m_offer)
//                        {
//                            printf("ASK [date=%s : %d] [dur=%ld] [ins=%lf, his=%lf]!!!!\n",data->head.m_update_time,
//                                   data->head.m_millisecond,
//                                   msg_dt.m_time_micro - cur_st.cur_order_tm,
//                                   cur_st.cur_order_px, cur_st.his_order_px);
//                            cur_st.cur_order_state = ST_ASK;
//                            ++cur_st.sound_ask_count;
//                        }
//                    }
//                    else
//                    {
//                        printf("FASK [date=%s : %d] [dur=%ld] [ins=%lf, his=%lf]!!!!\n",data->head.m_update_time,
//                               data->head.m_millisecond,
//                               msg_dt.m_time_micro - cur_st.cur_order_tm,
//                               cur_st.cur_order_px, cur_st.his_order_px);
//                        cur_st.cur_order_state = ST_ASK;
//                    }


//                }

//                if( cur_st.cur_order_state == (ST_BID|ST_INSERT) )
//                {
//                    //update the high px
//                    if(cur_st.his_order_px > msg_dt.m_bid)
//                    {
//                        cur_st.his_order_px = msg_dt.m_bid;
//                    }

//                    if(cur_st.cur_order_tm+1000*15000LL > msg_dt.m_time_micro)
//                    {
//                        if(cur_st.cur_order_px*(1-0.0008) > msg_dt.m_bid)
//                        {
//                            printf("BID [date=%s : %d] [dur=%ld] [ins=%lf, his=%lf]!!!!\n",data->head.m_update_time,
//                                   data->head.m_millisecond,
//                                   msg_dt.m_time_micro - cur_st.cur_order_tm,
//                                   cur_st.cur_order_px, cur_st.his_order_px);
//                            cur_st.cur_order_state = ST_BID;
//                            ++cur_st.sound_bid_count;
//                        }
//                    }
//                    else
//                    {
//                        printf("FBID [date=%s : %d] [dur=%ld] [ins=%lf, his=%lf]!!!!\n",data->head.m_update_time,
//                               data->head.m_millisecond,
//                               msg_dt.m_time_micro - cur_st.cur_order_tm,
//                               cur_st.cur_order_px, cur_st.his_order_px);
//                        cur_st.cur_order_state = ST_BID;
//                    }
//                }

//                if(data->normal.m_ask_px != 0 && forecast > data->normal.m_ask_px) {// &&
//                    //( (cur_st.cur_order_state & ST_BID) || (cur_st.cur_order_state == -1) ) ) {
//                    cur_st.cur_order_state = ST_ASK|ST_INSERT;
//                    cur_st.cur_order_px = data->normal.m_ask_px;
//                    ++cur_st.ask_count;
//                    cur_st.cur_order_tm = msg_dt.m_time_micro;
//                    cur_st.his_order_px = cur_st.cur_order_px;

//                    printf("ASK data [quote=%d][date=%s : %d]\n"
//                           "%lf %d \n"
//                           "%lf %lf %lf\n"
//                           "%d %lf %d\n",
//                           (int)data->head.m_quote_flag,
//                           data->head.m_update_time,
//                           data->head.m_millisecond,

//                           data->normal.m_last_px,
//                           data->normal.m_last_share,
//                           data->normal.m_total_value,
//                           data->normal.m_total_pos,
//                           data->normal.m_bid_px,
//                           data->normal.m_bid_share,
//                           data->normal.m_ask_px,
//                           data->normal.m_ask_share);
//                    printf("%d:\tsymbol:%s\tsignal = %lf\tforecast:%lf\t\n",cnt, data->head.m_symbol, signal, forecast);
//                } else if(data->normal.m_bid_px != 0 && forecast < data->normal.m_bid_px){// &&
//                    //( (cur_st.cur_order_state & ST_ASK) || (cur_st.cur_order_state == -1) ) ) {
//                    cur_st.cur_order_state = ST_BID|ST_INSERT;
//                    cur_st.cur_order_px = data->normal.m_bid_px;
//                    cur_st.cur_order_tm = msg_dt.m_time_micro;
//                    ++cur_st.bid_count;
//                    cur_st.his_order_px = cur_st.cur_order_px;
//                    printf("BID data [quote=%d][date=%s : %d]\n"
//                           "%lf %d \n"
//                           "%lf %lf %lf\n"
//                           "%d %lf %d\n",
//                           (int)data->head.m_quote_flag,
//                           data->head.m_update_time,
//                           data->head.m_millisecond,

//                           data->normal.m_last_px,
//                           data->normal.m_last_share,
//                           data->normal.m_total_value,
//                           data->normal.m_total_pos,
//                           data->normal.m_bid_px,
//                           data->normal.m_bid_share,
//                           data->normal.m_ask_px,
//                           data->normal.m_ask_share);
//                    printf("%d:\tsymbol:%s\tsignal = %lf\tforecast:%lf\t\n",cnt, data->head.m_symbol, signal, forecast);
//                }
//                ++cnt;
//                //                if(cnt >= 1824) {

//                //                    //break;
//                //                }
//                //                if(cnt >=4800)
//                //                    break;

//            }
//        }

//    } while(feof(fp) == 0);
//    fclose(fp);
//    printf("calc count %d\n", cnt);
//    printf("ask count:%d\t sound ask count:%d\n"
//           "bid count:%d\t sound bid count:%d\n",
//           cur_st.ask_count,cur_st.sound_ask_count,
//           cur_st.bid_count,cur_st.sound_bid_count);
//    return 0;

//}


int fc_calc(const char* cfg)
{
    json_t*root = NULL;
    json_error_t err;
    json_t *obj = NULL;
    const char* key = NULL;
    const char* value = NULL;
    forecaster_t fc = NULL;

    Dummy_ChinaL1Msg* pd=0;
    printf("sizeof Dummy_ChinaL1Msg = %llu, %llu\n",sizeof(Dummy_ChinaL1Msg), &(pd->m_limit_down));

    /** Load json conf */
    root = json_load_file(cfg, 0, &err);
    if(root == NULL) {
        printf("load config file error:%s at line %d, col %d\n", err.text, err.line, err.column);
        return -1;
    }
    key = "insttype";
    obj = json_object_get(root, key);
    if(!json_is_string(obj))
    {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    value = json_string_value(obj);

    fc = fc_create(value);

    key="signal_calc.duration";
    obj = obj_get(root, key);
    if(!json_is_integer(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    int dur = json_integer_value(obj);
    fc_set_duration(fc, dur);

    key="signal_calc.outpath";
    obj = obj_get(root, key);
    if(!json_is_string(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    const char* outpath = json_string_value(obj);

    key="signal_calc.pattern";
    obj = obj_get(root, key);
    if(!json_is_string(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    const char* namepattern = json_string_value(obj);

    key="signal_calc.datpath";
    obj = obj_get(root, key);
    if(!json_is_string(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    const char* datapath = json_string_value(obj);

    key="signal_calc.files";
    obj = obj_get(root, key);
    if(!json_is_array(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    json_t* arr = obj;
    int i;
    for(i = 0;
        i < json_array_size(arr) && (obj = json_array_get(arr, i));
        i++)
    {
        if(!json_is_string(obj)) {
            printf("config key s[%s] is misstype.\n", key);
            return -1;
        }
        value = json_string_value(obj);

        std::string file = datapath;
        file.append("/");
        file.append(value);

        fc_calc_subsignals(fc, file.c_str(), outpath, namepattern);
    }

    return 0;
}



