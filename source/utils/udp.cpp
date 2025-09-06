/**
 * Simple UDP socket interface for cross-platform use.
 * Distributed under MIT Software License
 */
#include "udp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h> // ssize_t
#include <errno.h>
#include <stdarg.h>

#if _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include <WinSock2.h>
    #include <ws2tcpip.h>           // winsock2 and TCP/IP functions
    #include <mstcpip.h>            // WSAIoctl Options
    #ifdef _MSC_VER
        #pragma comment(lib, "Ws2_32.lib") // link against winsock libraries
        #pragma comment(lib, "Iphlpapi.lib")
    #endif
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <poll.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////////

static int get_oserror() noexcept
{
#if _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

static void set_oserror(int err) noexcept
{
#if _WIN32
    WSASetLastError(err);
#else
    errno = err;
#endif
}

IpAddress::IpAddress(const char* addr_string, uint16_t port) noexcept
    : port{port}
{
    if (!addr_string || *addr_string == '\0')
    {
        addr = INADDR_ANY; // bind to any interface
    }
    else if (!inet_pton(AF_INET, addr_string, &addr_parts))
    {
        addr = 0;
        UdpSocket::print_error(get_oserror(), "failed to parse address: %s", addr_string);
    }
}

std::string IpAddress::to_string() const noexcept
{
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%u.%u.%u.%u:%u",
                       addr_parts[0], addr_parts[1], addr_parts[2], addr_parts[3], port);

    if (len < 0 || len >= static_cast<int>(sizeof(buf)))
        len = static_cast<int>(sizeof(buf)) - 1;
    buf[len] = '\0';

    return std::string{buf, buf + len};
}

////////////////////////////////////////////////////////////////////////////////////////////

UdpSocket::UdpSocket() noexcept
{
}

UdpSocket::~UdpSocket() noexcept
{
    close();
}

bool UdpSocket::create(const IpAddress& local_addr, bool blocking) noexcept
{
#if _WIN32
    static WSADATA wsaInit;
    if (wsaInit.wVersion == 0)
    {
        WSAStartup(MAKEWORD(2, 2), &wsaInit);
    }
#endif

    // only recreate handle if needed (useful for port scanning)
    if (!is_valid())
    {
        if ((socket = (int)::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            print_error(get_oserror(), "socket creation failed");
            return false;
        }

        int reuse = 1;
        if (!set_opt(SOL_SOCKET, SO_REUSEADDR, reuse))
        {
            print_error(get_oserror(), "set SO_REUSEADDR failed");
            // it's not a fatal error
        }

        // set blocking mode only once after socket creation
        set_blocking(blocking);
    }

    // bind the socket so it has a local address and port which others can reply back to
    struct sockaddr_in sa_in;
    memset(&sa_in, 0, sizeof(sa_in));
    sa_in.sin_family      = AF_INET;
    sa_in.sin_addr.s_addr = local_addr.addr;
    sa_in.sin_port        = htons(local_addr.port);
    if (::bind(socket, (struct sockaddr *)&sa_in, sizeof(sa_in)) < 0)
    {
        print_error(get_oserror(), "bind failed to %s", local_addr.to_string().c_str());
        close();
        return false;
    }

    this->addr = local_addr;
    return true;
}

void UdpSocket::close() noexcept
{
    if (socket < 0)
        return; // already closed
#if __linux__
    shutdown(socket, SHUT_RDWR);
    ::close(socket);
#else
    shutdown(socket, SD_BOTH);
    closesocket(socket);
#endif
    socket = -1;
}

int UdpSocket::available() const noexcept
{
    if (socket < 0) return -1; // invalid socket
    int bytes_available = 0;
#if _WIN32
    if (ioctlsocket(socket, FIONREAD, (u_long*)&bytes_available) != 0)
    {
        print_error(get_oserror(), "ioctlsocket FIONREAD failed");
        return -1;
    }
#else
    if (ioctl(socket, FIONREAD, &bytes_available) != 0)
    {
        print_error(get_oserror(), "ioctl FIONREAD failed");
        return -1;
    }
#endif
    return bytes_available;
}

void UdpSocket::set_blocking(bool is_blocking) noexcept
{
#if _WIN32
    u_long val = is_blocking?0:1; // FIONBIO: !=0 nonblock, 0 block
    if (ioctlsocket(socket, FIONBIO, &val) != 0)
    {
        print_error(get_oserror(), "failed to set socket to %sblocking", is_blocking ? "" : "non-");
        return;
    }
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) flags = 0;
    flags = is_blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    if (fcntl(socket, F_SETFL, flags) != 0)
    {
        print_error(get_oserror(), "failed to set socket to %sblocking", is_blocking ? "" : "non-");
        return;
    }
#endif
    this->blocking_io = is_blocking;
}

bool UdpSocket::set_buf_size(bool rcv_buf, int buf_size) noexcept
{
    int so_buf = (rcv_buf ? SO_RCVBUF : SO_SNDBUF);
#if __linux__
    // NOTE: on linux the kernel doubles buffsize for internal bookkeeping
    //       so to keep things consistent between platforms, we divide by 2 on linux:
    int size_cmd = static_cast<int>(buf_size / 2);
    int so_buf_force = (so_buf == SO_RCVBUF ? SO_RCVBUFFORCE : SO_SNDBUFFORCE);
#else
    int size_cmd = static_cast<int>(buf_size);
    int so_buf_force = 0;
#endif
    bool ok = set_opt(SOL_SOCKET, so_buf, size_cmd);
    if (!ok && so_buf_force != 0)
    {
        ok = set_opt(SOL_SOCKET, so_buf_force, size_cmd);
    }
    return ok;
}

int UdpSocket::get_buf_size(bool rcv_buf) const noexcept
{
    if (socket < 0) return -1; // invalid socket
    int so_buf = (rcv_buf ? SO_RCVBUF : SO_SNDBUF);
    int buf_size = 0;
    socklen_t len = sizeof(int);
    getsockopt(socket, SOL_SOCKET, so_buf, (char*)&buf_size, &len);
    return buf_size;
}

int UdpSocket::sendto(const void* data, size_t size, const IpAddress& to) noexcept
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = to.addr;
    addr.sin_port        = htons(to.port);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));
    return ::sendto(socket, (const char*)data, (int)size, 0, (struct sockaddr*)&addr, sizeof(addr));
}

int UdpSocket::recvfrom(void* buffer, size_t maxsize, IpAddress& from) noexcept
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int size = ::recvfrom(socket, (char*)buffer, (int)maxsize, 0, (struct sockaddr*)&addr, &addr_len);
    if (size > 0)
    {
        from.addr = addr.sin_addr.s_addr;
        from.port = ntohs(addr.sin_port);
    }
    return size;
}

bool UdpSocket::poll_read(int timeout_ms) const noexcept
{
    if (socket < 0) return false; // invalid socket

    struct pollfd pfd;
    pfd.fd = socket;
    pfd.events = POLLIN;
    pfd.revents = 0;

#if _WIN32 || _WIN64
    int r = WSAPoll(&pfd, 1, timeout_ms);
#else
    int r = ::poll(&pfd, 1, timeout_ms);
#endif

    if (r == 0)
        return false; // no data available (timeout)
    if (r < 0)
    {
        print_error(get_oserror(), "poll failed");
        return false; // error occurred
    }
    return (pfd.revents & POLLIN) != 0;
}

bool UdpSocket::set_opt(int level, int option, int value) noexcept
{
    if (socket < 0) return false;
    if (setsockopt(socket, level, option, (const char*)&value, sizeof(value)) < 0)
    {
        int err = get_oserror();
        print_error(err, "setsockopt %d:%d failed", option, value);
        set_oserror(err); // restore os error code
        return false;
    }
    return true;
}

int UdpSocket::get_opt(int level, int option) const noexcept
{
    if (socket < 0) return -1;
    int value = 0;
    socklen_t len = sizeof(value);
    if (getsockopt(socket, level, option, (char*)&value, &len) < 0)
    {
        int err = get_oserror();
        print_error(err, "getsockopt %d failed", option);
        set_oserror(err); // restore os error code
        return -1;
    }
    return value;
}

void UdpSocket::print_error(int err, const char* fmt, ...) noexcept
{
    int errcode = err ? err : get_oserror();

    // user formatted message
    char fmt_buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(fmt_buf, sizeof(fmt_buf), fmt, args);
    va_end(args);

    // format system error message
#if _WIN32
    char errbuf[512];
    int len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errcode, 0, errbuf, sizeof(errbuf), nullptr);
    if (errbuf[len - 2] == '\r') errbuf[len -= 2] = '\0';
    const char* errmsg = errbuf;
#else
    char errbuf[512];
    const char* errmsg = strerror_r(errcode, errbuf, sizeof(errbuf));
#endif

    fprintf(stderr, "UdpSocket: %s (%s)\n", fmt_buf, errmsg);
}
