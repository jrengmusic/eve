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

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::clear),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).clear();
                                                });

    registry->callbacks.add<juce::Component&> (Id::toType (Id::historyCapacity),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).setHistoryCapacity();
                                                });
}
