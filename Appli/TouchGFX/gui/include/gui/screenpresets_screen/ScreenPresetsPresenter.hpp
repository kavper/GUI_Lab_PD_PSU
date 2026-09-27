#ifndef SCREENPRESETSPRESENTER_HPP
#define SCREENPRESETSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenPresetsView;

class ScreenPresetsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenPresetsPresenter(ScreenPresetsView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenPresetsPresenter() {}
private:
    ScreenPresetsPresenter();
    ScreenPresetsView& view;
};

#endif
