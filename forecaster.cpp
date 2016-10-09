#include <stdio.h>
#include <stdlib.h>

#include "forecaster.h"

HC_0_Forecaster::HC_0_Forecaster(struct tm& date)
{
    (void)date;
    m_trading_instrument = "aaa2008";
}


void Forecaster::update(const ChinaL1Msg& msg)
{
    (void)msg;

}

double Forecaster::get_forecast()
{
    return rand()*1.1/RAND_MAX*2008;
}

void Forecaster::get_all_signal( double array[] )
{
    (void)array;
}

void Forecaster::reset()
{

}

std::vector<std::string> Forecaster::get_subscriptions() const
{
    std::vector<std::string> vec;
    vec.push_back("hc1609");
    vec.push_back("hc1610");
    vec.push_back("hc1611");

    return vec;
}

Forecaster* ForecasterFactory::createForecaster(std::string& fcName, struct tm& date)
{
    (void)fcName;
    HC_0_Forecaster* fc = new HC_0_Forecaster(date);
    return fc;
}
