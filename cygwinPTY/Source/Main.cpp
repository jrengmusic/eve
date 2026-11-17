#include "Session.h"

static constexpr int paddingSize { 32768 };

class RuntimePadding
{
public:
    RuntimePadding()
    {
        SecureZeroMemory (padding.data(), padding.size());
    }

private:
    std::array<char, paddingSize> padding;
};

extern "C" DWORD mainCRTStartup (LPVOID environmentBlock);

extern "C" DWORD cygwinPTYCRTStartup (LPVOID environmentBlock)
{
    RuntimePadding runtimePadding;
    return mainCRTStartup (environmentBlock);
}

int main (int argc, char* argv[])
{
    jassert (argc > 4);
    auto runtime { std::make_unique<const Runtime> (juce::File (argv[1])) };

    if (runtime->isLoaded())
    {
        std::function<void()> messageLoop { [&runtime, argv]
        {
            juce::ScopedJuceInitialiser_GUI juceInitialiser;
            Session session { *runtime, reinterpret_cast<HANDLE> (static_cast<uintptr_t> (juce::String (argv[2]).getLargeIntValue())), juce::String (argv[3]).getLargeIntValue(), juce::StringArray (argv + 4) };
            juce::MessageManager::getInstance()->runDispatchLoop();
        } };
        void* messageThread { nullptr };
        [[maybe_unused]] const auto isCreated { runtime->pthreadCreate (&messageThread, nullptr, [] (void* loop) -> void*
        {
            (*static_cast<std::function<void()>*> (loop)) ();
            return nullptr;
        }, &messageLoop) };
        jassert (isCreated == 0);
        runtime->pthreadJoin (messageThread, nullptr);
    }

    return 0;
}
