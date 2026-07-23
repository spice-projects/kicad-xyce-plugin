#include "mapped_file.h"

#include <system_error>

#ifdef _WIN32
namespace
{
    std::size_t get_file_size(HANDLE file_handle) {
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file_handle, &size)) {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "GetFileSizeEx");
        }
        return static_cast<std::size_t>(size.QuadPart);
    }
}
#endif

MappedFile::MappedFile(const std::filesystem::path& path) {
#ifdef _WIN32
    // open file for read-only access
    m_file_handle = CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_file_handle == INVALID_HANDLE_VALUE)
        return;

    try {
        // get file size
        m_size = get_file_size(m_file_handle);
        if (m_size == 0)
            return;

        // create mapping object
        m_mapping_handle = CreateFileMappingW(m_file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!m_mapping_handle)
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "CreateFileMappingW");

        // map view of the file
        m_data = static_cast<char*>(MapViewOfFile(m_mapping_handle, FILE_MAP_READ, 0, 0, m_size));
        if (!m_data)
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "MapViewOfFile");
    }
    catch (...) {
        // reset on failure
        reset();
        throw;
    }
#else
    // open file descriptor
    m_fd = open(path.c_str(), O_RDONLY);
    if (m_fd < 0)
        return;

    // get file size
    struct stat file_info {};
    if (fstat(m_fd, &file_info) == -1) {
        reset();
        return;
    }

    // assign file size
    m_size = static_cast<std::size_t>(file_info.st_size);
    if (m_size == 0) {
        reset();
        return;
    }

    // map into memory
    m_data = mmap(nullptr, m_size, PROT_READ, MAP_SHARED, m_fd, 0);
    if (m_data == MAP_FAILED) {
        reset();
        return;
    }
#endif
}

MappedFile::~MappedFile() noexcept {
    // reset mapping
    reset();
}

MappedFile::MappedFile(MappedFile&& other) noexcept
#ifdef _WIN32
    : m_file_handle(other.m_file_handle), m_mapping_handle(other.m_mapping_handle), m_data(other.m_data), m_size(other.m_size)
#else
    : m_fd(other.m_fd), m_data(other.m_data), m_size(other.m_size)
#endif
{
    // transfer ownership
    other.reset();
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        // reset current mapping
        reset();
#ifdef _WIN32
        m_file_handle = other.m_file_handle;
        m_mapping_handle = other.m_mapping_handle;
        m_data = other.m_data;
        m_size = other.m_size;
#else
        m_fd = other.m_fd;
        m_data = other.m_data;
        m_size = other.m_size;
#endif
        // transfer ownership
        other.reset();
    }
    return *this;
}

bool MappedFile::is_valid() const noexcept {
#ifdef _WIN32
    return m_data != nullptr;
#else
    return m_data != nullptr;
#endif
}

const char* MappedFile::data() const noexcept {
    // return memory pointer
    return static_cast<const char*>(m_data);
}

std::size_t MappedFile::size() const noexcept {
    // return mapped size
    return m_size;
}

void MappedFile::reset() noexcept {
#ifdef _WIN32
    if (m_data) {
        // unmap view
        UnmapViewOfFile(m_data);
        m_data = nullptr;
    }
    if (m_mapping_handle) {
        // close mapping object
        CloseHandle(m_mapping_handle);
        m_mapping_handle = nullptr;
    }
    if (m_file_handle != INVALID_HANDLE_VALUE) {
        // close file handle
        CloseHandle(m_file_handle);
        m_file_handle = INVALID_HANDLE_VALUE;
    }
    m_size = 0;
#else
    if (m_data) {
        // unmap memory
        munmap(m_data, m_size);
        m_data = nullptr;
    }
    if (m_fd >= 0) {
        // close file descriptor
        close(m_fd);
        m_fd = -1;
    }
    m_size = 0;
#endif
}
