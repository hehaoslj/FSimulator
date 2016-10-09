
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>
#include <string.h>

#include <jansson.h> /** Jansson lib */

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


    do {
        key = strtok_r(nkeys, ".", &last);
        if(key) {
            o = json_object_get(o, key);
        }

    } while(key != NULL && o != NULL);

    free(nkeys);
    return o;

}

int main(int argc, char* argv[]) {
    json_t*root = NULL;
    json_error_t err;
    json_t *obj = NULL;
    const char* key = NULL;
    const char* value = NULL;
    Forecaster *fc = NULL;



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
        printf("trading instrument:%s\\n", fc->get_trading_instrument().c_str());

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

    printf("calc begin date:%s to end date:%s\n", begindate, enddate);

    json_decref(root);
    delete fc;

    return 0;

}
