#include "EVEView.h"

static juce::MemoryBlock getCapturedBytes (const std::function<void (FILE&)>& writeToFile)
{
    const std::unique_ptr<FILE, decltype (&std::fclose)> file { std::tmpfile(), &std::fclose };
    juce::MemoryBlock capturedBytes;

    jassert (file != nullptr);

    writeToFile (*file);

    if (std::fseek (file.get(), 0, SEEK_END) == 0)
    {
        const auto byteCount { std::ftell (file.get()) };

        if (byteCount > 0)
        {
            std::rewind (file.get());
            capturedBytes = juce::MemoryBlock { static_cast<size_t> (byteCount) };
            std::fread (capturedBytes.getData(), sizeof (char), capturedBytes.getSize(), file.get());
        }
    }

    return capturedBytes;
}

static juce::MemoryBlock getTerminalFrame (const juce::File& harness, int columns, const juce::String& appearance, jam::GlyphAtlas::Metrics monoMetrics, int colorMode, float pixelScale)
{
    return getCapturedBytes ([&harness, columns, &appearance, monoMetrics, colorMode, pixelScale] (FILE& file)
    {
        static constexpr bool shouldUseInlineImages { true };

        jam::terminal::GraphicsEngine engine { file, shouldUseInlineImages };

        jam::StyleMarkdown styleMarkdown { Id::toType (Id::style), appearance };

        const auto lineHeight { styleMarkdown.getMetrics<float> (map::MarkdownStyleSheet::lineHeight) };
        styleMarkdown.setCellSize (static_cast<float> (jam::AttributedChar::narrowColumns), lineHeight);
        styleMarkdown.setPixelCellSize (monoMetrics.cellWidth, monoMetrics.cellHeight);
        styleMarkdown.setPixelScale (pixelScale);
        engine.setColorMode (colorMode);

        const auto cellRows { juce::AffineTransform::scale (1.0f, lineHeight).inverted() };

        jam::MarkdownComponent component;
        component.setLookAndFeel (&styleMarkdown);
        component.setBounds (0, 0, columns, 0);
        component.setDocument (jam::MarkdownDocument::parse (harness.loadFileAsString(), harness.getFullPathName()));

        const auto rows { juce::Rectangle<int> { columns, component.getViewedComponent()->getHeight() }.toFloat().transformedBy (cellRows).getSmallestIntegerContainer().getHeight() };

        {
            engine.resize (columns, rows, cellRows.mat11);

            jam::terminal::GraphicsContext context { engine };
            juce::Graphics g { context };
            component.getViewedComponent()->paintEntireComponent (g, false);
        }
    });
}

void EVEView::registerViewComponents (jam::Registry::Registration& registration)
{
    registration.viewComponents = [this] (jam::Registry& r)
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

        r.registerConfig<jam::TextEditor> (Id::textEditor,
                                           [this] (jam::TextEditor* textEditor)
                                           {
                                               static const juce::File harness { juce::File { __FILE__ }.getParentDirectory().getSiblingFile (Id::markdown.toString()).getChildFile (files::harness) };

                                               jassert (dynamic_cast<const jam::StyleCustom*> (&textEditor->getLookAndFeel()) != nullptr);

                                               const auto monoMetrics { static_cast<const jam::StyleCustom&> (textEditor->getLookAndFeel()).getOrCreateMonoMetrics() };
                                               const auto colorMode { jam::StyleManager::getInstance()->getMetrics<int> (Id::toType (Id::style), Id::colorMode) };

                                               const auto* display { juce::Desktop::getInstance().getDisplays().getDisplayForRect (textEditor->getScreenBounds()) };

                                               jassert (display != nullptr);

                                               const auto pixelScale { juce::Component::getApproximateScaleFactorForComponent (textEditor) * static_cast<float> (display->scale) };
                                               const auto bytes { getTerminalFrame (harness, textEditor->getNumColumns(), model.getAppearance(), monoMetrics, colorMode, pixelScale) };
                                               auto document { std::make_unique<jam::AnsiDocument> (jam::AnsiDocument::parse (static_cast<const char*> (bytes.getData()), static_cast<int> (bytes.getSize()))) };

                                               for (const auto& [colourId, argb] : document->getTrueColours())
                                                   textEditor->getLookAndFeel().setColour (colourId, juce::Colour { argb });

                                               textEditor->setDocument (std::move (document));
                                           });
    };

    registration.styles = [] (jam::Registry& r)
    {
        r.registerStyle<jam::StyleCustom> (Id::terminal);
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
