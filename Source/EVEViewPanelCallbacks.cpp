#include "EVEView.h"

void EVEView::attachPanelCallbacks()
{
    registry->callbacks.add<juce::Component&> (Id::toType (Id::UIScale),
                                                [] (juce::Component& c)
                                                {
                                                    static_cast<EVEView&> (c).resized();
                                                });

    jam::ButtonDialog* aboutDialog { nullptr };

    jam::Function::Map<juce::String, void> acquisitions;

    acquisitions.add<juce::Component*&> (
        Id::buttonDialog.toString(),
        [&aboutDialog] (juce::Component*& child)
        {
            if (jam::Component::hasProperty (child->getProperties(), Id::parameter, Id::toType (Id::aboutBox)))
                aboutDialog = static_cast<jam::ButtonDialog*> (child);
        });

    jam::Component::applyFunctionRecursively (
        panel.get(),
        [&acquisitions] (juce::Component* child)
        {
            const auto componentType { child->getProperties()[Id::type].toString() };

            if (acquisitions.contains (componentType))
                acquisitions.get (componentType, child);
        });

    if (aboutDialog != nullptr) attachAboutDialog (*aboutDialog);
}

void EVEView::attachAboutDialog (jam::ButtonDialog& aboutDialog)
{
    aboutDialog.onDismiss = [&aboutDialog]
    {
        jam::Component::applyFunctionRecursively (
            &aboutDialog,
            [] (juce::Component* child)
            {
                if (auto* anim { dynamic_cast<jam::AnimationBase*> (child) })
                    anim->stop();
            });
    };
}
