#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <stdint.h>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    virtual void ldoTelemetryUpdated(uint32_t inputVoltageMv,
                                     uint32_t voltageMv, int32_t currentUa,
                                     int16_t mosfetDeciC, int16_t pcbDeciC,
                                     uint8_t mode, bool connected, bool outputRequested,
                                     bool currentValid, bool currentCalibrated)
    {
        (void)inputVoltageMv; (void)voltageMv; (void)currentUa;
        (void)mosfetDeciC; (void)pcbDeciC;
        (void)mode; (void)connected; (void)outputRequested;
        (void)currentValid; (void)currentCalibrated;
    }

    void bind(Model* m)
    {
        model = m;
    }
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
