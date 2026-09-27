#ifndef SCREENSEQUENCERPRESENTER_HPP
#define SCREENSEQUENCERPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ScreenSequencerView;

class ScreenSequencerPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ScreenSequencerPresenter(ScreenSequencerView& v);
    virtual void activate();
    virtual void deactivate();
    virtual ~ScreenSequencerPresenter() {}
private:
    ScreenSequencerPresenter();
    ScreenSequencerView& view;
};

#endif
