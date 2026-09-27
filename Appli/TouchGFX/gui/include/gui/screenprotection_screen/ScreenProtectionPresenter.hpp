#ifndef SCREENPROTECTIONPRESENTER_HPP
#define SCREENPROTECTIONPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenProtectionView;

class ScreenProtectionPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenProtectionPresenter(ScreenProtectionView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenProtectionPresenter() {}
private:
    ScreenProtectionPresenter();
    ScreenProtectionView& view;
};

#endif
