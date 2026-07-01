/**
 * @file socket_compat.h
 * @brief Astrazione socket multipiattaforma (Windows/macOS/Linux).
 *
 * Su Windows include winsock2.h e definisce le API native.
 * Su macOS/Linux include le socket POSIX e mappa le funzioni
 * Windows-specifiche sulle equivalenti BSD/macOS:
 *   - SOCKET -> int
 *   - closesocket -> close
 *   - ioctlsocket -> ioctl
 *   - WSAGetLastError() -> errno (circa)
 *   - INVALID_SOCKET -> -1
 *   - WSAStartup/WSACleanup -> noop
 */
#ifndef SOCKET_COMPAT_H
#define SOCKET_COMPAT_H

#ifdef _WIN32
    #define NOGDI
    #define NOUSER
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    /* macOS / Linux */
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>

    /* Mappatura tipi e costanti */
    typedef int SOCKET;
    #define INVALID_SOCKET  (-1)
    #define SOCKET_ERROR    (-1)

    /* Mappatura funzioni */
    #define closesocket(x)      close(x)
    #define ioctlsocket(s,c,v)  ioctl(s,c,(void*)(v))
    #define WSAGetLastError()   (errno)

    /* Winsock errori -> POSIX (mappatura minima per codice progetto) */
    #define WSAEWOULDBLOCK      EWOULDBLOCK
    #define WSAEINPROGRESS      EINPROGRESS
    #define WSAEALREADY         EALREADY
    #define WSAENOTSOCK         ENOTSOCK
    #define WSAEDESTADDRREQ     EDESTADDRREQ
    #define WSAEMSGSIZE         EMSGSIZE
    #define WSAEPROTOTYPE       EPROTOTYPE
    #define WSAENOPROTOOPT      ENOPROTOOPT
    #define WSAEPROTONOSUPPORT  EPROTONOSUPPORT
    #define WSAESOCKTNOSUPPORT  ESOCKTNOSUPPORT
    #define WSAEOPNOTSUPP       EOPNOTSUPP
    #define WSAEPFNOSUPPORT     EPFNOSUPPORT
    #define WSAEAFNOSUPPORT     EAFNOSUPPORT
    #define WSAEADDRINUSE       EADDRINUSE
    #define WSAEADDRNOTAVAIL    EADDRNOTAVAIL
    #define WSAENETDOWN         ENETDOWN
    #define WSAENETUNREACH      ENETUNREACH
    #define WSAENETRESET        ENETRESET
    #define WSAECONNABORTED     ECONNABORTED
    #define WSAECONNRESET       ECONNRESET
    #define WSAENOBUFS          ENOBUFS
    #define WSAEISCONN          EISCONN
    #define WSAENOTCONN         ENOTCONN
    #define WSAESHUTDOWN        ESHUTDOWN
    #define WSAETOOMANYREFS     ETOOMANYREFS
    #define WSAETIMEDOUT        ETIMEDOUT
    #define WSAELOOP            ELOOP
    #define WSAENAMETOOLONG     ENAMETOOLONG
    #define WSAHOST_NOT_FOUND   HOST_NOT_FOUND
    #define WSATRY_AGAIN        TRY_AGAIN
    #define WSANO_RECOVERY      NO_RECOVERY
    #define WSANOTINITIALISED   ENOTCONN /*近似 */
    #define WSAENOTEMPTY        ENOTEMPTY

    /* Inizializzazione socket: su macOS/Linux non serve WSAStartup */
    static inline int WSASafeToCancel(void) { return 1; }
    #define WSACleanup()        ((void)0)
    #define WSAStartup(a,b)     (0)
#endif

#endif /* SOCKET_COMPAT_H */