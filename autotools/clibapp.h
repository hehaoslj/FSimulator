#ifndef CLIBAPP_H
#define CLIBAPP_H

typedef void* forecaster_t;
#ifdef __cplusplus

extern "C" {
#endif

forecaster_t fc_create(const char* type);


void fc_delete(forecaster_t fc);

int fc_set_duration(forecaster_t fc, int dur);

int fc_get_trading_instrument(forecaster_t fc, const char **instrument);


int fc_get_instruments_size(forecaster_t fc);


int fc_get_instruments(forecaster_t fc, int pos, const char** instrument);


int fc_get_subsignals_size(forecaster_t fc);


int fc_get_subsignals(forecaster_t fc, int pos, const char** signame);

int fc_calc_subsignals(forecaster_t fc, const char* dtfile, const char* outpath, const char* namepattern);

int fc_calc(const char* cfg);


/**
comment name pattern
%s subsignal name
%d the date of dtfile
%t the time of dtfile
*/



#ifdef __cplusplus
}
#endif

#endif /** CLIBAPP_H */
