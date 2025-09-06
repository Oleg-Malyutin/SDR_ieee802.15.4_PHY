/**
 * Simple UDP socket interface for cross-platform use.
 * Distributed under MIT Software License
 */
#pragma once
#include <stdint.h>
#include <stddef.h> // size_t
#include <string>

/**
 * @brief A simple IpAddress structure, example usage: IpAddress{"127.0.0.1", 5600}
 */
struct IpAddress
{
    union {
        uint32_t addr; // IPv4 address
        uint8_t addr_parts[4]; // IPv4 address as 4 bytes
    };
    
    uint16_t port; // Port number in local byte order

    IpAddress() noexcept : addr{0}, port{0} {}

    /**
     * @brief Binds the socket to INADDR_ANY(0) and the specified local port.
     */
    IpAddress(uint16_t local_port) noexcept : addr{0}, port{local_port} {}

    /**
     * @brief Specifies u32 IP address and port number.
     */
    IpAddress(uint32_t addr, uint16_t port) noexcept : addr{addr}, port{port} {}

    /** 
     * @brief Initializes IP address from a string eg "127.0.0.1" and port number.
     */
    IpAddress(const char* addr_string, uint16_t port) noexcept;

    /** @returns TRUE if at least port is set, 0.0.0.0 address is used for listener sockets */
    bool is_valid() const noexcept { return port != 0; }
    explicit operator bool() const noexcept { return port != 0; }

    /**
     * @returns IP address in human-readable string format, e.g. "127.0.0.1:12345"
     */
    std::string to_string() const noexcept;
};

/**
 * A very minimalistic UDP socket interface for cross-platform use.
 * Works on most Linux and Windows systems.
 */
class UdpSocket
{
    int socket = -1;
    IpAddress addr {}; // Local address and port
    bool blocking_io = false; // Blocking mode flag

public:
    /** @brief default initializes the socket, but does not create the OS handle */
    UdpSocket() noexcept;
    ~UdpSocket() noexcept;
    
    /** @returns TRUE if socket handle appears to be valid */
    bool is_valid() const noexcept { return socket >= 0; }
    explicit operator bool() const noexcept { return socket >= 0; }

    /** @returns Local address+port of this socket */
    const IpAddress& address() const noexcept { return addr; }

    /**
     * @brief Creates a UDP socket and configures default options.
     * @param local_port Binds this UDP socket to 0.0.0.0:local_port
     * @param blocking If true, the socket will be created in blocking mode.
     *                 If false, it will be created in non-blocking mode.
     * @return true if the socket was created successfully, false otherwise.
     */
    bool create(uint16_t local_port, bool blocking = false) noexcept
    {
        return create(IpAddress{local_port}, blocking);
    }
    bool create(const IpAddress& local_addr, bool blocking = false) noexcept;

    /**
     * @brief Closes the UDP socket.
     * @note After calling this function, the socket will no longer be usable.
     *       It is safe to call this function multiple times.
     */
    void close() noexcept;

    /**
     * @returns Number of bytes available for reading on the socket.
     *          This is a non-blocking call that checks how many bytes can be read
     *          without blocking.
     */
    int available() const noexcept;

    /**
     * @brief Sets the socket blocking or non-blocking mode.
     *        Blocking mode socket are slightly faster, but can complicate development.
     * @param is_blocking If true, the socket will be set to blocking mode.
     *                    If false, it will be set to non-blocking mode.
     * @note Non-blocking mode means that operations like recvfrom will return immediately
     *       if no data is available, rather than waiting indefinitely.
     *       This is useful for applications that need to handle multiple sockets or events
     *       without blocking the main thread.
     *       In blocking mode, recvfrom will wait until data is available and needs to be
     *       unblocked by closing the socket or only using poll() to check for data.
     */
    void set_blocking(bool is_blocking) noexcept;
    bool is_blocking() const noexcept { return blocking_io; }

    /**
     * @brief Sets the Send or Receiver buffer sizes (SO_RCVBUF and SO_SNDBUF).
     * @param rcv_buf If true, sets the receive buffer size (SO_RCVBUF).
     *                If false, sets the send buffer size (SO_SNDBUF).
     * @param buf_size The size of the buffer in bytes.
     * @note The buffer size is set using setsockopt() with SO_RCVBUF or SO_SNDBUF.
     *       If the buffer size cannot be set to the requested value, it may be adjusted
     *       to the nearest value supported by the system.
     *       This function can be used to optimize the performance of the socket for
     *       high-throughput applications by increasing the buffer size.
     *       Note that setting a very large buffer may fail.
     */
    bool set_buf_size(bool rcv_buf, int buf_size) noexcept;
    int get_buf_size(bool rcv_buf) const noexcept;

    /**
     * @brief Sends data to a specific remote address.
     * @param data Pointer to the data to be sent.
     * @param size Size of the data in bytes.
     * @param to The destination address and port.
     * @return The number of bytes sent, or -1 on error.
     */
    int sendto(const void* data, size_t size, const IpAddress& to) noexcept;

    /**
     * @brief Receives data from a remote sender.
     * @param buffer Pointer to the buffer where received data will be stored.
     * @param maxsize Maximum size of the buffer.
     * @param from [out] The address of the sender.
     * @return The number of bytes received, or -1 on error.
     */
    int recvfrom(void* buffer, size_t maxsize, IpAddress& from) noexcept;

    /**
     * @brief Polls the socket to check if data is available for reading.
     * @param timeout_ms Timeout in milliseconds. If -1, it will block indefinitely.
     */
    bool poll_read(int timeout_ms) const noexcept;

    /**
     * @brief Low-level wrapper around setsockopt() to set integer options.
     */
    bool set_opt(int level, int option, int value) noexcept;

    /**
     * @brief Low-level wrapper around getsockopt() to get integer options.
     * @return The value of the option, or -1 on error.
     */
    int get_opt(int level, int option) const noexcept;

    /**
     * @brief Utility for printing OS-specific error messages.
     */
    static void print_error(int err, const char* fmt, ...) noexcept;
};
