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

    openpty = library.getFunction<std::remove_pointer_t<decltype (openpty)>> ("openpty");
    ioctl = library.getFunction<std::remove_pointer_t<decltype (ioctl)>> ("ioctl");
    read = library.getFunction<std::remove_pointer_t<decltype (read)>> ("read");
    write = library.getFunction<std::remove_pointer_t<decltype (write)>> ("write");
    close = library.getFunction<std::remove_pointer_t<decltype (close)>> ("close");
    dup = library.getFunction<std::remove_pointer_t<decltype (dup)>> ("dup");
    dup2 = library.getFunction<std::remove_pointer_t<decltype (dup2)>> ("dup2");
    spawnv = library.getFunction<std::remove_pointer_t<decltype (spawnv)>> ("spawnv");
    kill = library.getFunction<std::remove_pointer_t<decltype (kill)>> ("kill");
    signal = library.getFunction<std::remove_pointer_t<decltype (signal)>> ("signal");
    setpgid = library.getFunction<std::remove_pointer_t<decltype (setpgid)>> ("setpgid");
    tcsetpgrp = library.getFunction<std::remove_pointer_t<decltype (tcsetpgrp)>> ("tcsetpgrp");
    setsid = library.getFunction<std::remove_pointer_t<decltype (setsid)>> ("setsid");
    cygwinInternal = library.getFunction<std::remove_pointer_t<decltype (cygwinInternal)>> ("cygwin_internal");
    pthreadCreate = library.getFunction<std::remove_pointer_t<decltype (pthreadCreate)>> ("pthread_create");
    pthreadJoin = library.getFunction<std::remove_pointer_t<decltype (pthreadJoin)>> ("pthread_join");
    poll = library.getFunction<std::remove_pointer_t<decltype (poll)>> ("poll");
    errorNumber = library.getFunction<std::remove_pointer_t<decltype (errorNumber)>> ("__errno");
    getOsfhandle = library.getFunction<std::remove_pointer_t<decltype (getOsfhandle)>> ("_get_osfhandle");
}

bool Runtime::isLoaded() const noexcept
{
    return openpty != nullptr and ioctl != nullptr and read != nullptr and write != nullptr and close != nullptr
           and dup != nullptr and dup2 != nullptr and spawnv != nullptr and kill != nullptr and signal != nullptr and setpgid != nullptr and tcsetpgrp != nullptr and setsid != nullptr and cygwinInternal != nullptr and pthreadCreate != nullptr
           and pthreadJoin != nullptr and poll != nullptr and errorNumber != nullptr and getOsfhandle != nullptr;
}
