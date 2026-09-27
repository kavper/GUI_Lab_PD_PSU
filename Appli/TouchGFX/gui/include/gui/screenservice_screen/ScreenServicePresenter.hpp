#ifndef SCREENSERVICEPRESENTER_HPP
#define SCREENSERVICEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenServiceView;

class ScreenServicePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenServicePresenter(ScreenServiceView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenServicePresenter() {}
private:
    ScreenServicePresenter();
    ScreenServiceView& view;
};

#endif
