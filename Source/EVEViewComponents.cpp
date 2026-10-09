#include "EVEView.h"

void EVEView::registerViewComponents (jam::Registry::Registration& registration)
{
    registration.viewComponents = [this] (jam::Registry& r)
    {
        r.registerComponent<juce::Label> (Id::label);
        r.registerComponent<jam::AnimationStrip> (Id::animationStrip);
        r.registerComponent<jam::AnimationScrambledText> (Id::scrambledText);
        r.registerComponent<jam::Selector> (Id::selector);
        r.registerComponent<jam::MarkdownComponent> (Id::markdown);

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

        r.registerConfig<jam::MarkdownComponent> (Id::markdown,
                                                  [this] (jam::MarkdownComponent* markdown)
                                                  {
                                                      static const juce::File harness { juce::File { __FILE__ }.getParentDirectory().getSiblingFile ("markdown").getChildFile ("markdown.md") };

                                                      markdown->setDocument (jam::MarkdownDocument::parse (harness.loadFileAsString(), harness.getFullPathName()));
                                                      markdown->setAppearance (model.getAppearance());
                                                  });

        {
            static constexpr int animationStripFrameCount { 125 };

            r.registerConfig<jam::AnimationStrip> (Id::animationStrip,
                                                   Id::toTag (Id::jreng),
                                                   [] (jam::AnimationStrip* animationStrip)
                                                   {
                                                       static const juce::String jrengWebsite { "https://jrengmusic.com" };

                                                       animationStrip->setImageStrip (jam::ImageLoader::getFromBinary (Id::jrengLogo), animationStripFrameCount);
                                                       animationStrip->setURL (jrengWebsite);
                                                       animationStrip->start();
                                                   });
        }

        r.registerConfig<jam::AnimationScrambledText> (
            Id::scrambledText,
            Id::toTag (Id::developer),
            [] (jam::AnimationScrambledText* scrambledText)
            {
                juce::StringArray lines;

                for (const auto& name : map::credits)
                    lines.add (name);

                scrambledText->setText (lines);
                scrambledText->start();
            });
    };

    registration.styles = [] (jam::Registry& r)
    {
        r.registerStyle<jam::StyleMarkdown> (Id::markdown);
    };

    registration.makeContent = [] (jam::Registry& r)
    {
        r.makeContent.add<jam::AudioModel&> (jam::Format::toCamelCase (Id::aboutBox),
                                             [] (jam::AudioModel& modelToUse)
                                             {
                                                 return jam::ViewContent::create (modelToUse, jam::HtmlDocument::getOrCreate (juce::Identifier { files::aboutLayout }));
                                             });

        r.makeContent.add<jam::AudioModel&> (
            Id::settings,
            [] (jam::AudioModel& modelToUse)
            {
                return jam::ViewSettings::create<jam::ConfigDocument> (modelToUse, jam::HtmlDocument::getOrCreate (juce::Identifier { files::settingsLayout }));
            });
    };
}
