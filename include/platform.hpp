#ifndef CATSURF_PLATFORM_HPP
#define CATSURF_PLATFORM_HPP

#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <BaseTsd.h>

#ifndef _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#define _SSIZE_T_DEFINED
#endif

namespace platform
{
    using native_stat = struct ::_stat64;
}

#else
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace platform
{
    using native_stat = struct stat;
}

#endif

namespace platform
{

inline bool unlink_file(const std::string& path)
{
#ifdef _WIN32
    return _unlink(path.c_str()) == 0;
#else
    return ::unlink(path.c_str()) == 0;
#endif
}

inline int open_readonly(const std::string& path)
{
#ifdef _WIN32
    return _open(path.c_str(), _O_RDONLY | _O_BINARY);
#else
    int flags = O_RDONLY;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    return ::open(path.c_str(), flags);
#endif
}

inline bool fstat_fd(int fd, native_stat& st)
{
#ifdef _WIN32
    return _fstat64(fd, &st) == 0;
#else
    return ::fstat(fd, &st) == 0;
#endif
}

inline bool stat_path(const std::string& path, native_stat& st)
{
#ifdef _WIN32
    return _stat64(path.c_str(), &st) == 0;
#else
    return ::stat(path.c_str(), &st) == 0;
#endif
}

inline bool is_regular(const native_stat& st)
{
#ifdef _WIN32
    return (st.st_mode & _S_IFREG) != 0;
#else
    return S_ISREG(st.st_mode);
#endif
}

inline bool is_directory(const native_stat& st)
{
#ifdef _WIN32
    return (st.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(st.st_mode);
#endif
}

inline size_t file_size(const native_stat& st)
{
    return static_cast<size_t>(st.st_size);
}

inline void close_fd(int fd)
{
    if (fd < 0)
        return;
#ifdef _WIN32
    _close(fd);
#else
    ::close(fd);
#endif
}

inline ssize_t read_fd(int fd, char* buffer, size_t count)
{
#ifdef _WIN32
    return static_cast<ssize_t>(_read(fd, buffer, static_cast<unsigned int>(count)));
#else
    return ::read(fd, buffer, count);
#endif
}

} // namespace platform

#endif // CATSURF_PLATFORM_HPP
