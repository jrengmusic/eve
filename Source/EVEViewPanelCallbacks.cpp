#include "EVEView.h"

void EVEView::registerCallbacks()
{
    registry->callbacks.add<juce::Component&> (Id::toType (Id::UIScale),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::clear),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).clear();
                                                });

    registry->callbacks.add<juce::Component&> (map::CommandId::getInstance()->get (map::CommandId::preview),
                                                [] (juce::Component& c)
                                                {
                                                    auto& editor { static_cast<EVEView&> (c) };
                                                    editor.setPreviewVisible (not editor.isPreviewVisible());
                                                });

    registry->callbacks.add<juce::Component&> (Id::toType (Id::preview),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    registry->callbacks.add<juce::Component&> (Id::toType (Id::historyCapacity),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).setHistoryCapacity();
                                                });
}
