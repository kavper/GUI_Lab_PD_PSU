#ifndef SCREENNETWORKPRESENTER_HPP
#define SCREENNETWORKPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenNetworkView;

class ScreenNetworkPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenNetworkPresenter(ScreenNetworkView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenNetworkPresenter() {}
private:
    ScreenNetworkPresenter();
    ScreenNetworkView& view;
};

#endif
