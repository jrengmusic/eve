#include "EVEView.h"

void EVEView::attachPanelCallbacks()
{
    registry->callbacks.add<juce::Component&> (Id::toType (Id::UIScale),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    registry->callbacks.add<juce::Component&> (Id::toType (Id::winsize),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).setWinsize();
                                                });
}
