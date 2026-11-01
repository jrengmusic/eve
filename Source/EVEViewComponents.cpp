#include "EVEView.h"

void EVEView::registerViewComponents (jam::Registry::Registration& registration)
{
    registration.viewComponents = [] (jam::Registry& r)
    {
        r.registerComponent<juce::Label> (Id::label);
        r.registerComponent<jam::Selector> (Id::selector);
        r.registerComponent<jam::TextEditor> (Id::textEditor);

        r.registerConfig<jam::Selector> (Id::selector,
                                         Id::toTag (Id::appearance),
                                         [] (jam::Selector* selector)
                                         {
                                             selector->addItemList (map::Appearance::getInstance()->get());
                                         });

        r.registerConfig<jam::Selector> (Id::selector,
                                         Id::toTag (Id::UIScale),
                                         [] (jam::Selector* selector)
                                         {
                                             selector->addItemList (map::UIScaleMap::getInstance()->get());
                                         });
    };

    registration.events = [] (jam::Registry& r)
    {
        r.bindEvent.add<juce::Component*&, jam::ViewManager::Events*, const juce::String&, const juce::String&> (
            Id::textEditor,
            [] (juce::Component* component, jam::ViewManager::Events* chainEvents, const juce::String& eventName, const juce::String&)
            {
                auto* textEditor { static_cast<jam::TextEditor*> (component) };

                textEditor->bindToEvents (chainEvents->at (juce::Identifier { eventName }));
                textEditor->setMode (map::EditorMode::terminal);
            });
    };

    registration.styles = [] (jam::Registry& r)
    {
        r.registerStyle<jam::StyleCustom> (Id::textEditor);
    };

    registration.makeContent = [] (jam::Registry& r)
    {
        r.makeContent.add<jam::AudioModel&, juce::LookAndFeel&> (
            Id::settings,
            [] (jam::AudioModel& modelToUse, juce::LookAndFeel& lookAndFeel)
            {
                return jam::ViewSettings::create (modelToUse, lookAndFeel, jam::HtmlDocument::getOrCreate (juce::Identifier { files::settingsLayout }));
            });
    };
}
