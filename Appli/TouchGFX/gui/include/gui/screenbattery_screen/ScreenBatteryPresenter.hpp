#ifndef SCREENBATTERYPRESENTER_HPP
#define SCREENBATTERYPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenBatteryView;

class ScreenBatteryPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenBatteryPresenter(ScreenBatteryView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenBatteryPresenter() {}
private:
    ScreenBatteryPresenter();
    ScreenBatteryView& view;
};

#endif
