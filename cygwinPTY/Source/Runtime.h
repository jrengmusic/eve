#pragma once

/**
 * @brief Window size that the session fills from jam::TerminalWinsize.
 *
 * The session passes it to openpty and to the window size ioctl.
 * The members follow the order of Session::getWinsize.
 */
struct Winsize
{
    unsigned short rows;        ///< Terminal height in cell rows.
    unsigned short cols;        ///< Terminal width in cell columns.
    unsigned short pixelWidth;  ///< Viewport width in physical pixels.
    unsigned short pixelHeight; ///< Viewport height in physical pixels.
};

/**
 * @brief Loads a Cygwin or MSYS2 runtime DLL and binds the POSIX functions that the helper calls.
 *
 * Each function pointer stays null when the library does not open or does not export the function.
 * isLoaded() tells whether every pointer is bound.
 */
class Runtime
{
public:
    /**
     * @brief Opens the runtime file as a library, and binds its functions.
     *
     * The call runs the msys or cygwin initialiser of the library first, when the library has one.
     *
     * @param file  Runtime DLL to load.
     */
    explicit Runtime (const juce::File& file);

    /**
     * @brief Tells whether every function pointer is bound.
     *
     * @return `true` when all functions of the runtime are bound.
     */
    bool isLoaded() const noexcept;

    /**
     * @brief Opens a pseudo terminal pair, sized by the start size of the session.
     *
     * The session passes the size that it received at construction as the start size of the pair.
     */
    int (*openpty) (int*, int*, char*, const void*, const Winsize*) { nullptr };   ///< Binds `openpty`, which opens a pseudo terminal pair with a start size.
    int (*ioctl) (int, int, ...) { nullptr };   ///< Binds `ioctl`, which sets the controlling terminal and the window size of a pseudo terminal.
    long (*read) (int, void*, size_t) { nullptr };   ///< Binds `read`, which reads from a file descriptor.
    long (*write) (int, const void*, size_t) { nullptr };   ///< Binds `write`, which writes to a file descriptor.
    int (*close) (int) { nullptr };   ///< Binds `close`, which closes a file descriptor.
    int (*dup) (int) { nullptr };   ///< Binds `dup`, which duplicates a file descriptor.
    int (*dup2) (int, int) { nullptr };   ///< Binds `dup2`, which duplicates a file descriptor onto a given number.
    int (*spawnv) (int, const char*, const char* const*) { nullptr };   ///< Binds `spawnv`, which starts a program from an argument vector.
    int (*kill) (int, int) { nullptr };   ///< Binds `kill`, which sends a signal to a process.
    void (*(*signal) (int, void (*) (int))) (int) { nullptr };   ///< Binds `signal`, which sets the action for a signal.
    int (*setpgid) (int, int) { nullptr };   ///< Binds `setpgid`, which sets the process group of a process.
    int (*tcsetpgrp) (int, int) { nullptr };   ///< Binds `tcsetpgrp`, which sets the foreground process group of a terminal.
    int (*setsid) () { nullptr };   ///< Binds `setsid`, which creates a session for the calling process.
    uintptr_t (*cygwinInternal) (int, ...) { nullptr };   ///< Binds `cygwin_internal`, the Cygwin query call. The session uses it to get the Windows process id of a process.
    int (*pthreadCreate) (void**, const void*, void* (*) (void*), void*) { nullptr };   ///< Binds `pthread_create`, which starts a thread.
    int (*pthreadJoin) (void*, void**) { nullptr };   ///< Binds `pthread_join`, which waits for a thread to end.

private:
    /** @brief The loaded runtime DLL. It stays open while the object lives. */
    juce::DynamicLibrary library;
};
