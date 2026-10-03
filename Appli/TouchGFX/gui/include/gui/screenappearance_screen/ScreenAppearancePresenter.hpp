#ifndef SCREENAPPEARANCEPRESENTER_HPP
#define SCREENAPPEARANCEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenAppearanceView;

class ScreenAppearancePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenAppearancePresenter(ScreenAppearanceView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenAppearancePresenter() {}
private:
    ScreenAppearancePresenter();
    ScreenAppearanceView& view;
};

#endif
