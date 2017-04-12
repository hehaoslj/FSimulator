#include "forecaster.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <time.h>

#include <jansson.h> /** Jansson lib */

class Forecaster;
class ForecasterFactory;

int main(int argc, char* argv[]) {
    std::string type="hc_0";
    struct tm date;
    time_t t = time(NULL);
    json_t*root = NULL;
    json_error_t err;
    json_t *obj = NULL;
    const char* key = NULL;
    const char* value = NULL;
    Forecaster *fc = NULL;

    localtime_r(&t, &date);

    fc  = ForecasterFactory::createForecaster( type, date );
    printf("trading instrument:%s\\\n", fc->get_trading_instrument().c_str());

    if (argc <= 1) {
        printf("please input the config file\\n");
        return 0;
    }

    /** Load json conf */
    root = json_load_file(argv[1], 0, &err);
    if(root == NULL) {
        printf("load config file error:%s at line %d, col %d\\n", err.text, err.line, err.column);
        return -1;
    }
    key = "conf_type";
    obj = json_object_get(root, key);
    if(!json_is_string(obj))
    {
        printf("config key s[%s] is missing.\\n", key);
        return -1;
    }

    value = "signal_calc";
    if( strcmp( json_string_value(obj), value ) != 0) {
        printf("config key[%s] value[%s] is incorrect.\\n", key, value);
        return -1;
    }

    key="output.begindate";
    obj = json_object_get(root, key);
    if(!json_is_string(obj)) {
        printf("config key s[%s] is missing.\\n", key);
        return -1;
    }


    return 0;

}

