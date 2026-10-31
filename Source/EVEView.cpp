#include "EVEView.h"
#include "EVEProcessor.h"

EVEView::EVEView (jam::AudioModel& newModel,
                  jam::PluginEditorLayout& newLayout,
                  juce::AudioProcessor& processorToConnectTo)
    : jam::PluginEditor (processorToConnectTo, newModel, newLayout)
{
    initialise (layout.getOrCreateSettings());
    setConstrainer (this);
    setScaling();
    juce::MessageManager::callAsync ([this] { grabKeyboardFocus(); });
}

void EVEView::focusGained (FocusChangeType)
{
    if (auto traverser { createKeyboardFocusTraverser() })
    {
        if (auto* defaultComponent { traverser->getDefaultComponent (this) })
            defaultComponent->grabKeyboardFocus();
    }
}

void EVEView::resizeStart() { setResizing (true); }

void EVEView::resizeEnd() { setResizing (false); }

void EVEView::setResizing (bool isResizing)
{
    jam::Component::applyFunctionRecursively (view.get(), [this, isResizing] (juce::Component* child)
    {
        if (jam::Component::hasProperty (child->getProperties(), Id::type, Id::textEditor))
            model.getOrCreateComponentProperty (child->getComponentID(), Id::isResizing).setValue (isResizing);
    });
}

void EVEView::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (property == Id::commandCount)
        applyCommands();

    if (property == Id::preview)
        setPreview (tree.getProperty (property).toString());

    jam::PluginEditor::valueTreePropertyChanged (tree, property);
}

void EVEView::initialiseTheme (const juce::ValueTree& settings)
{
    styleManager.create (layout.fonts, settings);

    theme = std::make_unique<jam::StyleTheme> (*styleManager, model.getAppearance());
    setLookAndFeel (theme.get());
}

void EVEView::initialiseRegistry()
{
    jam::Registry::Registration registration;

    registerButtons (registration);
    registerViewComponents (registration);

    registry.create (registration);
    registerCallbacks();
}

void EVEView::initialiseView()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };
    auto& terminalProcessor { processor.getTerminalProcessor() };

    view = jam::ViewEditor::create<jam::MarkdownDocument> (model, *theme, terminalProcessor.userInterfaceGetters, terminalProcessor.chainEvents, files::viewLayout);
    addAndMakeVisible (view.get());
}

void EVEView::getAllCommands (juce::Array<juce::CommandID>& commands)
{
    for (const auto& [command, name] : map::CommandId::getInstance()->get())
        commands.add (command);
}

void EVEView::getCommandInfo (juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto name { map::CommandId::getInstance()->get (commandID).toString() };
    result.setInfo (name, name, {}, result.flags & ~ juce::ApplicationCommandInfo::isDisabled);
}

void EVEView::paintOverChildren (juce::Graphics& g) { transition.paint (g, getLocalBounds()); }

void EVEView::clear()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };

    processor.clear();
}

void EVEView::setHistoryCapacity()
{
    auto& processor { static_cast<EVEProcessor&> (*getAudioProcessor()) };

    jam::MessageBox::historyCleared (this);
    clear();
    processor.restart();
}

void EVEView::setPreview (const juce::String& path)
{
    setPreviewVisible (path.isNotEmpty());
}

void EVEView::setPreviewVisible (bool isVisible)
{
    model.getOrCreateComponentProperty (Id::toTag (Id::preview), Id::value).setValue (isVisible);
}

bool EVEView::isPreviewVisible() const
{
    return static_cast<bool> (model.getOrCreateComponentProperty (Id::toTag (Id::preview), Id::value).getValue());
}

void EVEView::setOverlays (const juce::Component& textEditor, const jam::HashMap<juce::int64, jam::Array<juce::Rectangle<int>>>& imageBounds)
{
    jam::HashMap<juce::int64, jam::Owner<jam::Preview>> told;

    for (const auto& [image, boxes] : imageBounds)
    {
        auto [toldEntry, inserted] { told.try_emplace (image, overlays.contains (image) ? std::move (overlays.at (image)) : jam::Owner<jam::Preview> {}) };
        auto& [toldImage, previews] { *toldEntry };

        while (previews.size() > boxes.size())
            previews.remove (previews.size() - 1);

        for (int index { 0 }; index < boxes.size(); ++index)
        {
            if (index == previews.size())
                applyImage (image, previews);

            if (index < previews.size())
                previews.at (static_cast<size_t> (index))->setBounds (getLocalArea (&textEditor, boxes.at (index)));
        }
    }

    overlays = std::move (told);
}

void EVEView::applyCommands()
{
    juce::String cut;

    static_cast<EVEProcessor&> (*getAudioProcessor()).getTerminalProcessor().getCommands().applyPendingData ([this, &cut] (const char* data, int size)
    {
        const char* const end { data + size };

        while (data < end)
        {
            const char* const terminator { std::find (data, end, '\0') };

            if (terminator == end)
                cut += juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (end) };
            else
            {
                const auto name { cut.isEmpty() ? juce::Identifier { juce::String::CharPointerType (data), juce::String::CharPointerType (terminator) }
                                                : juce::Identifier { cut + juce::String { juce::String::CharPointerType (data), juce::String::CharPointerType (terminator) } } };

                cut = juce::String {};

                if (const auto found { map::CommandId::getInstance()->find (name) })
                    commandManager->invokeDirectly (*found, false);
            }

            data = juce::jmin (terminator + 1, end);
        }
    });
}

void EVEView::applyImage (juce::int64 image, jam::Owner<jam::Preview>& previews)
{
    auto& swap { static_cast<EVEProcessor&> (*getAudioProcessor()).getTerminalProcessor().getSwap() };
    const auto data { swap.getImageData (image) };

    if (data.getSize() > 0)
    {
        auto& preview { *previews.add (std::make_unique<jam::Preview>()) };

        preview.setImageData (data.getData(), data.getSize(), swap.getImageEncoding (image));
        addAndMakeVisible (preview);
    }
}
