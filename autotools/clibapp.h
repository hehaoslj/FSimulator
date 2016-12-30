#ifndef CLIBAPP_H
#define CLIBAPP_H

typedef void* forecaster_t;

extern "C"
forecaster_t fc_create(const char* type);

extern "C"
void fc_delete(forecaster_t fc);

extern "C"
int fc_get_trading_instrument(forecaster_t fc, const char **instrument);

extern "C"
int fc_get_instruments_size(forecaster_t fc);

extern "C"
int fc_get_instruments(forecaster_t fc, int pos, const char** instrument);

extern "C"
int fc_get_subsignals_size(forecaster_t fc);

extern "C"
int fc_get_subsignals(forecaster_t fc, int pos, const char** signame);

/**
comment name pattern
%s subsignal name
%d the date of dtfile
%t the time of dtfile
*/
extern "C"
int fc_calc_subsignals(forecaster_t fc, const char* dtfile, const char* outpath, const char* namepattern);

#endif /** CLIBAPP_H */
