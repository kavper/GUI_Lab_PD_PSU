#ifndef SCREENUSBPDPRESENTER_HPP
#define SCREENUSBPDPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenUsbPdView;

class ScreenUsbPdPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenUsbPdPresenter(ScreenUsbPdView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenUsbPdPresenter() {}
private:
    ScreenUsbPdPresenter();
    ScreenUsbPdView& view;
};

#endif
