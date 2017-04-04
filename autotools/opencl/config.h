#ifndef CONFIG_H
#define CONFIG_H




#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

typedef void* config_t;

config_t cfg_init_file(const char*);

void cfg_close(config_t);

const char* cfg_get_string(config_t cfg, const char* keys);
long cfg_get_integer(config_t cfg, const char* keys);
double cfg_get_real(config_t cfg, const char* keys);
bool cfg_get_bool(config_t cfg, const char* keys);
int cfg_get_list_size(config_t cfg, const char* keys);
const char* cfg_get_list_string(config_t cfg, const char* keys, int idx);
long cfg_get_list_integer(config_t cfg, const char* keys, int idx);
double cfg_get_list_real(config_t cfg, const char* keys, int idx);
bool cfg_get_list_bool(config_t cfg, const char* keys, int idx);

#ifdef __cplusplus
}
#endif

#endif /** CONFIG_H */
