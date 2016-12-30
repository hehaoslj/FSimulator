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

typedef struct signal_calc_s {

} signal_calc_t;

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
        printf("key:%s\n", key);
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



int main(int argc, char* argv[]) {
    json_t*root = NULL;
    json_error_t err;
    json_t *obj = NULL;
    const char* key = NULL;
    const char* value = NULL;
    Forecaster *fc = NULL;
    ssh_config_t param;


    if (argc <= 1) {
        printf("please input the config file\n");
        return 0;
    }

    /** Load json conf */
    root = json_load_file(argv[1], 0, &err);
    if(root == NULL) {
        printf("load config file error:%s at line %d, col %d\n", err.text, err.line, err.column);
        return -1;
    }
    key = "conftype";
    obj = json_object_get(root, key);
    if(!json_is_string(obj))
    {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }

    value = "signal_calc";
    if( strcmp( json_string_value(obj), value ) != 0) {
        printf("config key[%s] value[%s] is incorrect.\n", key, value);
        return -1;
    }


    key="insttype";
    obj = json_object_get(root, key);
    if(!json_is_string(obj))
    {
        printf("config keys[%s] is missing.\n", key);
        return -1;
    }
    value = json_string_value(obj);
    {
        struct tm date;
        time_t t = time(NULL);
        std::string type = value;
        localtime_r(&t, &date);

        fc  = ForecasterFactory::createForecaster( type, date );
        std::string id = fc->get_trading_instrument();
        printf("%p, trading instrument:%s\\n",
               fc,
               id.c_str());

        std::vector<std::string> ids = fc->get_subscriptions();
        for(size_t i=0; i< ids.size(); ++i) {
            printf("id=%s\\n", ids[i].c_str());
        }
        double* value;
        int size = 0;
        ids = fc->get_subsignal_name();
        printf("signal size:%d\\n", ids.size());
        for(size_t i=0; i< ids.size(); ++i) {
            printf("id=%s\\n", ids[i].c_str());
        }
        value = (double*)malloc(sizeof(double)*ids.size());
        fc->get_all_signal(&value, &size);
        printf("signal size:%d\\n", size);

    }
    /** Calculate prepare */
    const char* begindate;
    const char* enddate;
    key="output.begindate";
    obj = obj_get(root, key);
    if(!json_is_string(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    begindate = json_string_value(obj);


    key = "output.enddate";
    obj = obj_get(root, key);
    if(!json_is_string(obj)) {
        printf("config key s[%s] is missing.\n", key);
        return -1;
    }
    enddate = json_string_value(obj);


    get_json_string(root, "ssh.host", param.host);
    get_json_string(root, "ssh.username", param.username);
    get_json_string(root, "ssh.password", param.password);
    get_json_string(root, "ssh.root", param.root);
    get_json_integer<int>(root, "ssh.port", &param.port);
    get_json_integer<int>(root, "ssh.authtype", &param.authtype);
    if(param.host.size() > 0) {
        printf("using ssh :\\n"
               "\\thost:\\t%s\\n"
               "\\tport:\\t%d\\n"
               "\\tuser:\\t%s\\n"
               "\\tpass:\\t%s\\n"
               ,param.host.c_str(),
               param.port,
               param.username.c_str(),
               param.password.c_str());
    }


    printf("calc begin date:%s to end date:%s\n", begindate, enddate);

    json_decref(root);
    //delete fc;

    return 0;

}

std::string&   replace_all(std::string&   str,const   std::string&   old_value,const   std::string&   new_value)
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
    ChinaL1Msg trad_msg;
    std::vector<ChinaL1Msg> sub_msgs;
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
    in->subscriptions = fc->get_subscriptions();
    for(size_t i=0; i<in->subscriptions.size(); ++i)
    {
        ChinaL1Msg msg;
        Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg;
        msg_dt.m_inst = in->subscriptions[i];
        in->sub_msgs.push_back(msg);
    }
    in->subsignals = fc->get_subsignal_name();

    return in;
}

void fc_delete(forecaster_t fc)
{
    (void)fc;
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    delete in;
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
#pragma pack(pop)

#define read_in(pos, sz) fread(buf+pos, sz, 1 ,fp)
#define read_filehdr() read_in(0, 24)
#define read_pkghdr() read_in(0, 16)
#define read_pkgdat() read_in(16, hdr->size)

int fc_calc_subsignals(forecaster_t fc, const char* dtfile, const char* outpath, const char* namepattern)
{
    char buf[256];
    pcap_pkghdr* hdr=reinterpret_cast<pcap_pkghdr*>(buf);
    pcap_pkgdat* data = reinterpret_cast<pcap_pkgdat*>(buf+16);
    in_forecaster_t* in = reinterpret_cast<in_forecaster_t*>(fc);
    FILE* fp;
    printf("fc_calc_subsignals data:\n file %s\noutpath:%s\npattern:%s\nsz:%ld\nhdr:%ld\n",
           dtfile, outpath, namepattern,
           sizeof(pcap_pkgdat), sizeof(pcap_pkghdr));
    fp = fopen(dtfile, "rb");
    if(!fp)
        return EBADF;
    std::string sdate;
    std::string stime;
    {
        std::string dfile = dtfile;
        std::string::size_type pos(0), pos_dt(0);
        pos = dfile.find("__", 0);
        if(pos == std::string::npos)
        {
            printf("no __\n");
            return 0;
        }
        sdate = dfile.substr(pos-10, 10);
        stime = dfile.substr(pos+2, 8);
    }

    FILE* fpsigs = 0;
    {
        std::string name = namepattern;
        name = replace_all(name, "%type", in->stype);

        name = replace_all(name, "%date", sdate);
        name = replace_all(name, "%time", stime);
        name = "/" + name;
        name = outpath + name;
        printf("sig file name :%s\n", name.c_str());
        fpsigs = fopen(name.c_str(), "w");
        fprintf(fpsigs, "\"Date\",\"Index\",\"Signal\"");
        for(size_t i=0; i< in->subsignals.size(); ++i)
        {
            fprintf(fpsigs, ",\"%s\"",
                    in->subsignals[i].c_str());
        }
        fprintf(fpsigs, "\n");
    }

    int cnt =0;
    ChinaL1Msg *pmsg_data;
    double *rdata = new double[in->subsignals.size()+1];
    int rsize = in->subsignals.size();
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
        if(hdr->size == 42+sizeof(guava_udp_head)+sizeof(guava_udp_normal))
        {
            bool is_symbol = false;
            bool is_trading=false;
            if(strcmp(in->trading_instrument.c_str(), data->head.m_symbol) == 0
                    )
            {
               is_symbol = true;
               is_trading = true;
               pmsg_data = &in->trad_msg;
            }
            for(size_t i=0; i< in->subscriptions.size(); ++i) {
                if(strcmp(in->subscriptions[i].c_str(), data->head.m_symbol) == 0)
                {
                    is_symbol = true;
                    pmsg_data = &in->sub_msgs[i];
                    break;
                }
            }

            if(!is_symbol)
            {
                continue;
            }
            //printf("%s. %s\n", in->trading_instrument.c_str(), data->head.m_symbol);

            const uint64_t dmax = 0x7fefffffffffffff;
            if( (*(const uint64_t*)(&data->normal.m_ask_px) != dmax) &&
                    (*(const uint64_t*)(&data->normal.m_bid_px) != dmax) &&
                    (*(const uint64_t*)(&data->normal.m_last_px) != dmax) )
            {
                /** Calculate */
                ChinaL1Msg &msg_data = *pmsg_data;
                Dummy_ChinaL1Msg& msg_dt = *(Dummy_ChinaL1Msg*)&msg_data;
                int64_t micro_time = (int64_t)hdr->tv_sec*1000000+hdr->tv_usec;

                //msg_dt.m_inst = data->head.m_symbol;
                //msg_dt.m_time_micro = dtime*1000000+data->head.m_millisecond*1000;
                msg_dt.m_time_micro = micro_time;
//                msg_dt.m_bid = data->normal.m_bid_px;
//                msg_dt.m_offer = data->normal.m_ask_px;
//                msg_dt.m_bid_quantity = data->normal.m_bid_share;
//                msg_dt.m_offer_quantity = data->normal.m_ask_share;
//                msg_dt.m_volume = 0;//guava1 no-data md_data->PriVol.Volume ;
//                msg_dt.m_notional = data->normal.m_total_pos;//md_data->PriVol.Turnover;
                msg_dt.m_limit_up = 0;
                msg_dt.m_limit_down = 0;

                switch((int)data->head.m_quote_flag)
                {
                case 1:
                    //msg_dt.m_bid = data->normal.m_bid_px;
                    //msg_dt.m_offer = data->normal.m_ask_px;
                    //msg_dt.m_bid_quantity = data->normal.m_bid_share;
                    //msg_dt.m_offer_quantity = data->normal.m_ask_share;
                    msg_dt.m_volume = data->normal.m_total_pos;//guava1 no-data md_data->PriVol.Volume ;
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
                    msg_dt.m_volume = data->normal.m_total_pos;//guava1 no-data md_data->PriVol.Volume ;
                    msg_dt.m_notional = data->normal.m_total_value;//md_data->PriVol.Turnover;
                    break;
                case 0:
                    break;
                case 4:
                    continue;

                }

                double signal = 0;

                in->fc->update(msg_data);
                if(!is_trading)
                    continue;
                signal = in->fc->get_forecast();
                rsize = 0;
                //printf("call get_all_signal %d\n", rsize);
                in->fc->get_all_signal(&rdata, &rsize);
                //printf("called get_all_signal %d\n", rsize);
                FILE* fp = fpsigs;
                fprintf(fp, "\"%s %03d\",%d,%016.014lf",
                        data->head.m_update_time,
                        data->head.m_millisecond,
                        cnt,
                        signal);

                for(size_t i=0; i< rsize; ++i)
                {

                    fprintf(fp, ",%016.014lf",

                            data->head.m_update_time,
                            data->head.m_millisecond,
                            *(rdata+i) );
                }
                fprintf(fp, "\n");
                if(signal > 1) {
                printf("data [quote=%d][date=%s : %d]\n"
                       "%lf %d \n"
                       "%lf %lf %lf\n"
                       "%d %lf %d\n",
                       (int)data->head.m_quote_flag,
                       data->head.m_update_time,
                       data->head.m_millisecond,

                       data->normal.m_last_px,
                       data->normal.m_last_share,
                       data->normal.m_total_value,
                       data->normal.m_total_pos,
                       data->normal.m_bid_px,
                       data->normal.m_bid_share,
                       data->normal.m_ask_px,
                       data->normal.m_ask_share);
                printf("%d:\tsymbol:%s\tsignal = %lf\n",cnt, data->head.m_symbol, signal);
                }
                ++cnt;
//                if(cnt >= 1824) {

//                    //break;
//                }
//                if(cnt >=4800)
//                    break;

            }
        }

    } while(feof(fp) == 0);
    fclose(fp);
    printf("calc count %d\n", cnt);
    return 0;

}
