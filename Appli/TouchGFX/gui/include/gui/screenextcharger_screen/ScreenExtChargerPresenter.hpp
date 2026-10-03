#ifndef SCREENEXTCHARGERPRESENTER_HPP
#define SCREENEXTCHARGERPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenExtChargerView;

class ScreenExtChargerPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenExtChargerPresenter(ScreenExtChargerView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenExtChargerPresenter() {}
private:
    ScreenExtChargerPresenter();
    ScreenExtChargerView& view;
};

#endif
