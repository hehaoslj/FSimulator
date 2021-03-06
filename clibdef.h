typedef void* forecaster_t;


forecaster_t fc_create(const char* type);


void fc_delete(forecaster_t fc);


int fc_get_trading_instrument(forecaster_t fc, const char **instrument);


int fc_get_instruments_size(forecaster_t fc);


int fc_get_instruments(forecaster_t fc, int pos, const char** instrument);


int fc_get_subsignals_size(forecaster_t fc);

int fc_get_subsignals(forecaster_t fc, int pos, const char** signame);
