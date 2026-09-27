#ifndef SCREENBMSPRESENTER_HPP
#define SCREENBMSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenBmsView;

class ScreenBmsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenBmsPresenter(ScreenBmsView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenBmsPresenter() {}
private:
    ScreenBmsPresenter();
    ScreenBmsView& view;
};

#endif
