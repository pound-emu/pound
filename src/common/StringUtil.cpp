// Copyright 2025 Xenon Emulator Project. All rights reserved.

#include "StringUtil.h"
#include "Types.h"

namespace Base
{

#ifdef _WIN32
static std::wstring CPToUTF16(u32 code_page, std::string_view input)
{
    const s32 size = ::MultiByteToWideChar(code_page, 0, input.data(), static_cast<s32>(input.size()), nullptr, 0);

    if (size == 0)
    {
        return {};
    }

    std::wstring output(size, L'\0');

    if (size != ::MultiByteToWideChar(code_page, 0, input.data(), static_cast<s32>(input.size()), &output[0],
                                      static_cast<s32>(output.size())))
    {
        output.clear();
    }

    return output;
}
#endif  // ifdef _WIN32

std::string UTF16ToUTF8(const std::wstring_view& input)
{
#ifdef _WIN32
    const s32 size =
        ::WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<s32>(input.size()), nullptr, 0, nullptr, nullptr);
    if (size == 0)
    {
        return {};
    }

    std::string output(size, '\0');

    if (size != ::WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<s32>(input.size()), &output[0],
                                      static_cast<s32>(output.size()), nullptr, nullptr))
    {
        output.clear();
    }
    return output;
#else
    // Very hacky way to get cross-platform wstring conversion
    return std::filesystem::path(input).string();
#endif  // ifdef _WIN32
}

std::wstring UTF8ToUTF16W(const std::string_view& input)
{
#ifdef _WIN32
    return CPToUTF16(CP_UTF8, input);
#else
    // Very hacky way to get cross-platform wstring conversion
    return std::filesystem::path(input).wstring();
#endif  // ifdef _WIN32
}

std::string ToLower(const std::string& str)
{
    std::string out = str;
    std::transform(str.begin(), str.end(), out.data(), ::tolower);
    return out;
}

}  // namespace Base
