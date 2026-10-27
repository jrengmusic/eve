#if JucePlugin_Build_Standalone
#include "EVEStandaloneApp.h"

juce::PropertySet* EVEStandaloneApp::getUserSettings() { return &settings; }

juce::JUCEApplicationBase* juce_CreateApplication() { return new EVEStandaloneApp(); }

#endif
