#ifndef SCREENPOWERPATHPRESENTER_HPP
#define SCREENPOWERPATHPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenPowerPathView;

class ScreenPowerPathPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenPowerPathPresenter(ScreenPowerPathView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenPowerPathPresenter() {}
private:
    ScreenPowerPathPresenter();
    ScreenPowerPathView& view;
};

#endif
