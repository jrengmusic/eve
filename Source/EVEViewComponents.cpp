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

    registration.events = [this] (jam::Registry& r)
    {
        r.bindEvent.add<juce::Component*&, jam::ViewManager::Events*, const juce::String&, const juce::String&> (
            Id::textEditor,
            [this] (juce::Component* component, jam::ViewManager::Events* chainEvents, const juce::String& eventName, const juce::String&)
            {
                enum class TerminalInputCallback { write };

                auto* textEditor { static_cast<jam::TextEditor*> (component) };
                auto& callbacks { chainEvents->at (juce::Identifier { eventName }) };
                const auto terminal { model.state.getChildWithName (Id::toType (Id::terminal)) };

                textEditor->onKeyPress = [&callbacks, applicationCursor = terminal.getPropertyAsValue (Id::applicationCursor, nullptr)] (const juce::KeyPress& key)
                {
                    const auto bytes { jam::TerminalKeyboard::map (key, static_cast<bool> (applicationCursor.getValue())) };
                    const auto isSent { not bytes.empty() };

                    if (isSent)
                        callbacks.get (static_cast<int> (TerminalInputCallback::write), bytes.data(), static_cast<int> (bytes.size()));

                    return isSent;
                };

                textEditor->onMouse = [&callbacks,
                                       mouseTracking = terminal.getPropertyAsValue (Id::mouseTracking, nullptr),
                                       mouseSgr = terminal.getPropertyAsValue (Id::mouseSgr, nullptr)] (jam::TerminalMouse::Type type,
                                                                                                         int button,
                                                                                                         jam::Cell::Point cell,
                                                                                                         const juce::ModifierKeys& mods)
                {
                    const auto bytes { jam::TerminalMouse::map (type,
                                                                button,
                                                                cell.x,
                                                                cell.y,
                                                                mods,
                                                                static_cast<int> (mouseTracking.getValue()),
                                                                static_cast<bool> (mouseSgr.getValue())) };
                    const auto isReported { bytes.getSize() > 0 };

                    if (isReported)
                        callbacks.get (static_cast<int> (TerminalInputCallback::write),
                                       static_cast<const char*> (bytes.getData()),
                                       static_cast<int> (bytes.getSize()));

                    return isReported;
                };

                textEditor->liveCursor.referTo (terminal.getPropertyAsValue (Id::cursor, nullptr));
                textEditor->liveCursorVisible.referTo (terminal.getPropertyAsValue (Id::cursorVisible, nullptr));
                textEditor->liveIsRecording.referTo (terminal.getPropertyAsValue (Id::isRecording, nullptr));
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
