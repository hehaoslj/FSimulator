
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <jansson.h>

#include "config.h"

typedef struct _config_s {
    FILE* fp;
    json_t* root;
    json_error_t err;
}config_s;

static inline json_t* obj_get(json_t* root, const char* keys)
{
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


const char* cfg_get_string(void* cfg, const char* keys)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_string(obj))
        return NULL;
    return json_string_value(obj);
}

config_t cfg_init_file(const char * name)
{
    config_s *cfg = NULL;
    if(name)
    {
        cfg = (config_s*)malloc(sizeof(config_s) );
        cfg->fp = fopen(name, "r");
        cfg->root = json_loadf(cfg->fp, 0, &cfg->err);
    }
    return cfg;

}

long cfg_get_integer(config_t cfg, const char *keys)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_integer(obj))
    {
        printf("obj is not integer %p\n", obj);
        return 0;
    }
    return json_integer_value(obj);
}

double cfg_get_real(config_t cfg, const char *keys)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_real(obj))
        return 0;
    return json_real_value(obj);
}

bool cfg_get_bool(config_t cfg, const char *keys)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_boolean(obj))
        return false;
    return true;
}

int cfg_get_list_size(config_t cfg, const char *keys)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_array(obj))
        return 0;
    return json_array_size(obj);
}

const char *cfg_get_list_string(config_t cfg, const char *keys, int idx)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_array(obj))
        return NULL;
    return json_string_value( json_array_get(obj, idx) );
}

long cfg_get_list_integer(config_t cfg, const char *keys, int idx)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_array(obj))
        return 0;
    return json_integer_value( json_array_get(obj, idx) );
}

double cfg_get_list_real(config_t cfg, const char *keys, int idx)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_array(obj))
        return 0;
    return json_real_value( json_array_get(obj, idx) );
}

bool cfg_get_list_bool(config_t cfg, const char *keys, int idx)
{
    config_s *pv = (config_s *)cfg;
    json_t* obj;
    obj = obj_get(pv->root, keys);
    if(!json_is_array(obj))
        return NULL;
    return json_is_boolean( json_array_get(obj, idx) );
}

void cfg_close(config_t cfg)
{
    config_s *pv = (config_s *)cfg;
    json_decref(pv->root);
    fclose(pv->fp);
}
