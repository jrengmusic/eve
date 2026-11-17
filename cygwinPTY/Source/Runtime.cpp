#include "Runtime.h"

Runtime::Runtime (const juce::File& file)
{
    if (library.open (file.getFullPathName()))
    {
        auto* initialise { library.getFunction<void ()> ("msys_dll_init") };

        if (initialise == nullptr)
            initialise = library.getFunction<void ()> ("cygwin_dll_init");

        if (initialise != nullptr)
            initialise();
    }

    openpty = library.getFunction<int (int*, int*, char*, const void*, const Winsize*)> ("openpty");
    ioctl = library.getFunction<int (int, int, ...)> ("ioctl");
    read = library.getFunction<long (int, void*, size_t)> ("read");
    write = library.getFunction<long (int, const void*, size_t)> ("write");
    close = library.getFunction<int (int)> ("close");
    dup = library.getFunction<int (int)> ("dup");
    dup2 = library.getFunction<int (int, int)> ("dup2");
    spawnv = library.getFunction<int (int, const char*, const char* const*)> ("spawnv");
    kill = library.getFunction<int (int, int)> ("kill");
    signal = library.getFunction<void (*(int, void (*) (int))) (int)> ("signal");
    setpgid = library.getFunction<int (int, int)> ("setpgid");
    tcsetpgrp = library.getFunction<int (int, int)> ("tcsetpgrp");
    setsid = library.getFunction<int ()> ("setsid");
    cygwinInternal = library.getFunction<uintptr_t (int, ...)> ("cygwin_internal");
    pthreadCreate = library.getFunction<int (void**, const void*, void* (*) (void*), void*)> ("pthread_create");
    pthreadJoin = library.getFunction<int (void*, void**)> ("pthread_join");
}

bool Runtime::isLoaded() const noexcept
{
    return openpty != nullptr and ioctl != nullptr and read != nullptr and write != nullptr and close != nullptr
           and dup != nullptr and dup2 != nullptr and spawnv != nullptr and kill != nullptr and signal != nullptr and setpgid != nullptr and tcsetpgrp != nullptr and setsid != nullptr and cygwinInternal != nullptr and pthreadCreate != nullptr
           and pthreadJoin != nullptr;
}
