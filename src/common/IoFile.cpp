// Copyright 2025 Xenon Emulator Project. All rights reserved.

#include <vector>

#include "IoFile.h"

#include "Assert.h"
#include "Error.h"
#include "Logging/Log.h"
#include "PathUtil.h"

#ifdef _WIN32
#define ftruncate _chsize_s
#define fsync _commit

#include <Windows.h>
#include <io.h>
#include <share.h>
#else
#include <unistd.h>
#endif

#ifdef _MSC_VER
#define fileno _fileno
#define fseeko _fseeki64
#define ftello _ftelli64
#endif

namespace fs = std::filesystem;

namespace Base::FS
{

#ifdef _WIN32

[[nodiscard]] constexpr const wchar_t* AccessModeToWStr(FileAccessMode mode, FileMode type)
{
    switch (type)
    {
        case FileMode::BinaryMode:
            switch (mode)
            {
                case FileAccessMode::Read:
                    return L"rb";
                case FileAccessMode::Write:
                    return L"wb";
                case FileAccessMode::Append:
                    return L"ab";
                case FileAccessMode::ReadWrite:
                    return L"r+b";
                case FileAccessMode::ReadAppend:
                    return L"a+b";
            }
            break;
        case FileMode::TextMode:
            switch (mode)
            {
                case FileAccessMode::Read:
                    return L"r";
                case FileAccessMode::Write:
                    return L"w";
                case FileAccessMode::Append:
                    return L"a";
                case FileAccessMode::ReadWrite:
                    return L"r+";
                case FileAccessMode::ReadAppend:
                    return L"a+";
            }
            break;
    }

    return L"";
}

[[nodiscard]] constexpr int ToWindowsFileShareFlag(FileShareFlag flag)
{
    switch (flag)
    {
        case FileShareFlag::ShareNone:
        default:
            return _SH_DENYRW;
        case FileShareFlag::ShareReadOnly:
            return _SH_DENYWR;
        case FileShareFlag::ShareWriteOnly:
            return _SH_DENYRD;
        case FileShareFlag::ShareReadWrite:
            return _SH_DENYNO;
    }
}

#else

[[nodiscard]] constexpr const char* AccessModeToStr(FileAccessMode mode, FileMode type)
{
    switch (type)
    {
        case FileMode::BinaryMode:
            switch (mode)
            {
                case FileAccessMode::Read:
                    return "rb";
                case FileAccessMode::Write:
                    return "wb";
                case FileAccessMode::Append:
                    return "ab";
                case FileAccessMode::ReadWrite:
                    return "r+b";
                case FileAccessMode::ReadAppend:
                    return "a+b";
            }
            break;
        case FileMode::TextMode:
            switch (mode)
            {
                case FileAccessMode::Read:
                    return "r";
                case FileAccessMode::Write:
                    return "w";
                case FileAccessMode::Append:
                    return "a";
                case FileAccessMode::ReadWrite:
                    return "r+";
                case FileAccessMode::ReadAppend:
                    return "a+";
            }
            break;
    }

    return "";
}

#endif

[[nodiscard]] constexpr int ToSeekOrigin(SeekOrigin origin)
{
    switch (origin)
    {
        case SeekOrigin::SetOrigin:
        default:
            return SEEK_SET;
        case SeekOrigin::CurrentPosition:
            return SEEK_CUR;
        case SeekOrigin::End:
            return SEEK_END;
    }
}

IOFile::IOFile() = default;

IOFile::IOFile(const std::string& path, FileAccessMode mode, FileMode type, FileShareFlag flag)
{
    Open(path, mode, type, flag);
}

IOFile::IOFile(std::string_view path, FileAccessMode mode, FileMode type, FileShareFlag flag)
{
    Open(path, mode, type, flag);
}

IOFile::IOFile(const fs::path& path, FileAccessMode mode, FileMode type, FileShareFlag flag)
{
    Open(path, mode, type, flag);
}

IOFile::~IOFile()
{
    Close();
}

IOFile::IOFile(IOFile&& other) noexcept
{
    std::swap(filePath, other.filePath);
    std::swap(fileAccessMode, other.fileAccessMode);
    std::swap(fileType, other.fileType);
    std::swap(file, other.file);
}

IOFile& IOFile::operator=(IOFile&& other) noexcept
{
    std::swap(filePath, other.filePath);
    std::swap(fileAccessMode, other.fileAccessMode);
    std::swap(fileType, other.fileType);
    std::swap(file, other.file);
    return *this;
}

int IOFile::Open(const fs::path& path, FileAccessMode mode, FileMode type, FileShareFlag flag)
{
    Close();

    filePath = path;
    fileAccessMode = mode;
    fileType = type;

    errno = 0;
    int result = 0;

#ifdef _WIN32
    if (flag != FileShareFlag::ShareNone)
    {
        file = ::_wfsopen(path.c_str(), AccessModeToWStr(mode, type), ToWindowsFileShareFlag(flag));
        result = errno;
    }
    else
    {
        result = ::_wfopen_s(&file, path.c_str(), AccessModeToWStr(mode, type));
    }
#else
    file = std::fopen(path.c_str(), AccessModeToStr(mode, type));
    result = errno;
#endif

    if (!IsOpen())
    {
        const auto ec = std::error_code{result, std::generic_category()};
        LOG_ERROR(Base_Filesystem, "Failed to open the file at path={}, error_message={}", PathToUTF8String(filePath),
                  ec.message());
    }

    return result;
}

void IOFile::Close()
{
    if (!IsOpen())
    {
        return;
    }

    errno = 0;

    const bool closeResult = ::fclose(file) == 0;

    if (!closeResult)
    {
        const auto ec = std::error_code{errno, std::generic_category()};
        LOG_ERROR(Base_Filesystem, "Failed to close the file at path={}, ec_message={}", PathToUTF8String(filePath),
                  ec.message());
    }

    file = nullptr;

#ifdef _WIN64
    if (fileMapping && fileAccessMode == FileAccessMode::ReadWrite)
    {
        ::CloseHandle(std::bit_cast<HANDLE>(fileMapping));
    }
#endif
}

void IOFile::Unlink()
{
    if (!IsOpen())
    {
        return;
    }

    // Mark the file for deletion
    std::error_code fsError;
    fs::remove_all(filePath, fsError);
    if (fsError)
    {
        LOG_ERROR(Base_Filesystem, "Failed to remove the file at '{}'. Reason: {}", PathToUTF8String(filePath),
                  fsError.message());
    }
}

uptr IOFile::GetFileMapping()
{
    if (fileMapping)
    {
        return fileMapping;
    }
#ifdef _WIN64
    const s32 fd = fileno(file);

    HANDLE hfile = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));
    HANDLE mapping = nullptr;

    /*  if (fileAccessMode == FileAccessMode::ReadWrite) {
    mapping = CreateFileMapping2(hfile, NULL, FILE_MAP_WRITE, PAGE_READWRITE, SEC_COMMIT, 0,
                                 NULL, NULL, 0);
  } else {
    mapping = hfile;
  }*/

    mapping = hfile;

    fileMapping = std::bit_cast<uptr>(mapping);
    ASSERT_MSG(fileMapping, "{}", Base::GetLastErrorMsg());
    return fileMapping;
#else
    fileMapping = fileno(file);
    return fileMapping;
#endif
}

std::string IOFile::ReadString(size_t length) const
{
    std::vector<char> string_buffer(length);

    const u64 charsRead = ReadSpan<char>(string_buffer);
    const auto stringSize = charsRead != length ? charsRead : length;

    return std::string{string_buffer.data(), stringSize};
}

bool IOFile::Flush() const
{
    if (!IsOpen())
    {
        return false;
    }

    errno = 0;

    const bool flushResult = ::fflush(file) == 0;

    if (!flushResult)
    {
        const std::error_code ec = {errno, std::generic_category()};
        LOG_ERROR(Base_Filesystem, "Failed to flush the file at path={}, ec_message={}", PathToUTF8String(filePath),
                  ec.message());
    }

    return flushResult;
}

bool IOFile::Commit() const
{
    if (!IsOpen())
    {
        return false;
    }

    errno = 0;

    bool commitResult = ::fflush(file) == 0 && ::fsync(::fileno(file)) == 0;

    if (!commitResult)
    {
        const std::error_code ec = {errno, std::generic_category()};
        LOG_ERROR(Base_Filesystem, "Failed to commit the file at path={}, ec_message={}", PathToUTF8String(filePath),
                  ec.message());
    }

    return commitResult;
}

bool IOFile::SetSize(u64 size) const
{
    if (!IsOpen())
    {
        return false;
    }

    errno = 0;

    const bool setSizeResult = ::ftruncate(::fileno(file), static_cast<s64>(size)) == 0;

    if (!setSizeResult)
    {
        const std::error_code ec = {errno, std::generic_category()};
        LOG_ERROR(Base_Filesystem, "Failed to resize the file at path={}, size={}, ec_message={}",
                  PathToUTF8String(filePath), size, ec.message());
    }

    return setSizeResult;
}

u64 IOFile::GetSize() const
{
    if (!IsOpen())
    {
        return 0;
    }

    // Flush any unwritten buffered data into the file prior to retrieving the file size.
    std::fflush(file);

    u64 fSize = 0;
    // fs::file_size can cause a exception if it is not a valid file
    try
    {
        std::error_code ec{};
        fSize = fs::file_size(filePath, ec);
        if (fSize == -1 || !fSize)
        {
            LOG_ERROR(Base_Filesystem, "Failed to retrieve the file size of path={}, ec_message={}",
                      PathToUTF8String(filePath), ec.message());
            return 0;
        }
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(Base_Filesystem, "Exception trying to get file size. Exception: {}", ex.what());
        return 0;
    }

    return fSize;
}

bool IOFile::Seek(s64 offset, SeekOrigin origin) const
{
    if (!IsOpen())
    {
        return false;
    }

    if (False(fileAccessMode & (FileAccessMode::Write | FileAccessMode::Append)))
    {
        u64 size = GetSize();
        if (origin == SeekOrigin::CurrentPosition && Tell() + offset > size)
        {
            LOG_ERROR(Base_Filesystem, "Seeking past the end of the file");
            return false;
        }
        else if (origin == SeekOrigin::SetOrigin && static_cast<u64>(offset) > size)
        {
            LOG_ERROR(Base_Filesystem, "Seeking past the end of the file");
            return false;
        }
        else if (origin == SeekOrigin::End && offset > 0)
        {
            LOG_ERROR(Base_Filesystem, "Seeking past the end of the file");
            return false;
        }
    }

    errno = 0;

    const s32 seekResult = fseeko(file, offset, ToSeekOrigin(origin)) == 0;

    if (!seekResult)
    {
        const std::error_code ec = {errno, std::generic_category()};
        LOG_ERROR(Base_Filesystem, "Failed to seek the file at path={}, offset={}, origin={}, ec_message={}",
                  PathToUTF8String(filePath), offset, static_cast<u32>(origin), ec.message());
    }

    return seekResult;
}

s64 IOFile::Tell() const
{
    if (!IsOpen())
    {
        return 0;
    }

    errno = 0;

    return ftello(file);
}

u64 GetDirectorySize(const std::filesystem::path& path)
{
    if (!fs::exists(path))
    {
        return 0;
    }

    u64 total = 0;
    for (const auto& entry : fs::recursive_directory_iterator(path))
    {
        if (fs::is_regular_file(entry.path()))
        {
            // fs::file_size can cause a exception if it is not a valid file
            try
            {
                std::error_code ec{};
                u64 fileSize = fs::file_size(entry.path(), ec);
                if (fileSize != -1 && fileSize)
                {
                    total += fileSize;
                }
                else
                {
                    LOG_ERROR(Base_Filesystem, "Failed to retrieve the file size of path={}, ec_message={}",
                              PathToUTF8String(entry.path()), ec.message());
                }
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR(Base_Filesystem, "Exception trying to get file size. Exception: {}", ex.what());
            }
        }
    }
    return total;
}

}  // namespace Base::FS
