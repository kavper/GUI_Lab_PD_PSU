#ifndef SCREENMEASUREMENTSPRESENTER_HPP
#define SCREENMEASUREMENTSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenMeasurementsView;

class ScreenMeasurementsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenMeasurementsPresenter(ScreenMeasurementsView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenMeasurementsPresenter() {}
private:
    ScreenMeasurementsPresenter();
    ScreenMeasurementsView& view;
};

#endif
