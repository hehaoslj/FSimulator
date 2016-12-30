#include "clibapp.h"
#include "forecaster.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>
#include <string.h>

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

/** Implement clib */

typedef struct _internal_forecaster_s {
    Forecaster *fc;
    std::string trading_instrument;
    std::vector<std::string> subscriptions;
    std::vector<std::string> subsignals;
} in_forecaster_t;

forecaster_t fc_create(const char* type)
{
    in_forecaster_t * in =  NULL;
    Forecaster *fc = NULL;
    struct tm date;
    time_t t = time(NULL);
    std::string stype = type;
    localtime_r(&t, &date);
    fc  = ForecasterFactory::createForecaster( stype, date );

    in = new in_forecaster_t;
    in->fc = fc;
    in->trading_instrument = fc->get_trading_instrument();
    in->subscriptions = fc->get_subscriptions();
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

extern "C"
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
