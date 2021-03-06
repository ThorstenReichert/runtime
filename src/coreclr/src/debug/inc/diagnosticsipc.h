// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#ifndef __DIAGNOSTICS_IPC_H__
#define __DIAGNOSTICS_IPC_H__

#include <stdint.h>

#ifdef TARGET_UNIX
  struct sockaddr_un;
#else
  #include <Windows.h>
#endif /* TARGET_UNIX */

typedef void (*ErrorCallback)(const char *szMessage, uint32_t code);

class IpcStream final
{
public:
    static constexpr int32_t InfiniteTimeout = -1;
    ~IpcStream();
    bool Read(void *lpBuffer, const uint32_t nBytesToRead, uint32_t &nBytesRead, const int32_t timeoutMs = IpcStream::InfiniteTimeout);
    bool Write(const void *lpBuffer, const uint32_t nBytesToWrite, uint32_t &nBytesWritten, const int32_t timeoutMs = IpcStream::InfiniteTimeout);
    bool Flush() const;
    void Close(ErrorCallback callback = nullptr);

    class DiagnosticsIpc final
    {
    public:
        enum ConnectionMode
        {
            CLIENT,
            SERVER
        };

        enum class PollEvents : uint8_t
        {
            TIMEOUT  = 0x00, // implies timeout
            SIGNALED = 0x01, // ready for use
            HANGUP   = 0x02, // connection remotely closed
            ERR      = 0x04  // other error
        };

        // The bookeeping struct used for polling on server and client structs
        struct IpcPollHandle
        {
            // Only one of these will be non-null, treat as a union
            DiagnosticsIpc *pIpc;
            IpcStream *pStream;

            // contains some set of PollEvents
            // will be set by Poll
            // Any values here are ignored by Poll
            uint8_t revents;

            // a cookie assignable by upstream users for additional bookkeeping
            void *pUserData;
        };

        // Poll
        // Paramters:
        // - IpcPollHandle * rgpIpcPollHandles: Array of IpcPollHandles to poll
        // - uint32_t nHandles: The number of handles to poll
        // - int32_t timeoutMs: The timeout in milliseconds for the poll (-1 == infinite)
        // Returns:
        // int32_t: -1 on error, 0 on timeout, >0 on successful poll
        // Remarks:
        // Check the events returned in revents for each IpcPollHandle to find the signaled handle.
        // Signaled DiagnosticsIpcs can call Accept() without blocking.
        // Signaled IpcStreams can call Read(...) without blocking.
        // The caller is responsible for cleaning up "hung up" connections.
        static int32_t Poll(IpcPollHandle *rgIpcPollHandles, uint32_t nHandles, int32_t timeoutMs, ErrorCallback callback = nullptr);

        ConnectionMode mode;

        ~DiagnosticsIpc();

        // Creates an IPC object
        static DiagnosticsIpc *Create(const char *const pIpcName, ConnectionMode mode, ErrorCallback callback = nullptr);

        // puts the DiagnosticsIpc into Listening Mode
        // Re-entrant safe
        bool Listen(ErrorCallback callback = nullptr);

        // produces a connected stream from a server-mode DiagnosticsIpc.  Blocks until a connection is available.
        IpcStream *Accept(ErrorCallback callback = nullptr);

        // Connect to a server and returns a connected stream
        IpcStream *Connect(ErrorCallback callback = nullptr);

        //!Closes an open IPC.
        void Close(ErrorCallback callback = nullptr);

    private:

#ifdef TARGET_UNIX
        const int _serverSocket;
        sockaddr_un *const _pServerAddress;
        bool _isClosed;

        DiagnosticsIpc(const int serverSocket, sockaddr_un *const pServerAddress, ConnectionMode mode = ConnectionMode::SERVER);

        // Used to unlink the socket so it can be removed from the filesystem
        // when the last reference to it is closed.
        void Unlink(ErrorCallback callback = nullptr);
#else
        static const uint32_t MaxNamedPipeNameLength = 256;
        char _pNamedPipeName[MaxNamedPipeNameLength]; // https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-createnamedpipea
        HANDLE _hPipe = INVALID_HANDLE_VALUE;
        OVERLAPPED _oOverlap = {};

        DiagnosticsIpc(const char(&namedPipeName)[MaxNamedPipeNameLength], ConnectionMode mode = ConnectionMode::SERVER);
#endif /* TARGET_UNIX */

        bool _isListening;

        DiagnosticsIpc() = delete;
        DiagnosticsIpc(const DiagnosticsIpc &src) = delete;
        DiagnosticsIpc(DiagnosticsIpc &&src) = delete;
        DiagnosticsIpc &operator=(const DiagnosticsIpc &rhs) = delete;
        DiagnosticsIpc &&operator=(DiagnosticsIpc &&rhs) = delete;
    };

private:
#ifdef TARGET_UNIX
    int _clientSocket = -1;
    IpcStream(int clientSocket, int serverSocket, DiagnosticsIpc::ConnectionMode mode = DiagnosticsIpc::ConnectionMode::SERVER)
        : _clientSocket(clientSocket), _mode(mode) {}
#else
    HANDLE _hPipe = INVALID_HANDLE_VALUE;
    OVERLAPPED _oOverlap = {};
    BOOL _isTestReading = false; // used to check whether we are already doing a 0-byte read to test for data
    IpcStream(HANDLE hPipe, DiagnosticsIpc::ConnectionMode mode = DiagnosticsIpc::ConnectionMode::SERVER);
#endif /* TARGET_UNIX */

    DiagnosticsIpc::ConnectionMode _mode;

    IpcStream() = delete;
    IpcStream(const IpcStream &src) = delete;
    IpcStream(IpcStream &&src) = delete;
    IpcStream &operator=(const IpcStream &rhs) = delete;
    IpcStream &&operator=(IpcStream &&rhs) = delete;
};

#endif // __DIAGNOSTICS_IPC_H__
