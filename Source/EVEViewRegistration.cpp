#include "EVEView.h"

void EVEView::registerButtons (jam::Registry::Registration& registration)
{
    registration.buttons = [] (jam::Registry& r)
    {
        r.registerComponent<jam::ButtonSVG> (Id::button);
        r.registerComponent<jam::ButtonSVG> (Id::toTag (Id::settings),
                                             juce::StringArray { BinaryData::getString (files::settingsNormal),
                                                                 BinaryData::getString (files::settingsOver),
                                                                 BinaryData::getString (files::settingsDown),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String() });
        r.registerComponent<jam::ButtonSVG> (Id::up,
                                             juce::StringArray { BinaryData::getString (files::upNormal),
                                                                 BinaryData::getString (files::upOver),
                                                                 BinaryData::getString (files::upDown),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String() });
        r.registerComponent<jam::ButtonSVG> (Id::down,
                                             juce::StringArray { BinaryData::getString (files::downNormal),
                                                                 BinaryData::getString (files::downOver),
                                                                 BinaryData::getString (files::downDown),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String(),
                                                                 juce::String() });
        r.registerComponent<jam::ButtonLogo> (Id::toTag (Id::aboutBox));
        r.registerComponent<jam::ButtonDialog> (Id::buttonDialog);
    };

    registration.config = [] (jam::Registry& r)
    {
        r.registerConfig<jam::Selector> (Id::selector,
                                         [] (jam::Selector* s)
                                         {
                                             auto* reg { jam::Registry::getInstance() };
                                             jassert (reg != nullptr);

                                             s->setNavigationButtons (jam::Registry::toButton (reg->make.get (Id::up)),
                                                                      jam::Registry::toButton (reg->make.get (Id::down)));
                                         });

        r.registerConfig<jam::ButtonDialog> (Id::buttonDialog,
                                             Id::toTag (Id::settings),
                                             [] (jam::ButtonDialog* d)
                                             {
                                                 d->setShouldBeDismissedWhenOutOfFocus (false);
                                             });
    };
}
