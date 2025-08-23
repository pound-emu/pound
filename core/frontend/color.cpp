#include "color.h"

ImVec4 gui::color::with_alpha(const ImVec4& color, float alpha)
{
    auto vec = ImVec4(color.x, color.y, color.z, alpha);
    return vec;
}

ImVec4 gui::color::lighten(const ImVec4& color, float amount)
{
    float x = std::min(1.0F, color.x + amount);
    float y = std::min(1.0F, color.y + amount);
    float z = std::min(1.0F, color.z + amount);
    auto vec = ImVec4(x, y, z, color.w);
    return vec;
}

ImVec4 gui::color::darken(const ImVec4& color, float amount)
{
    float x = std::max(0.0F, color.x - amount);
    float y = std::max(0.0F, color.y - amount);
    float z = std::max(0.0F, color.z - amount);
    auto vec = ImVec4(x, y, z, color.w);
    return vec;
}

ImVec4 gui::color::from_hex(uint32_t hex, float alpha)
{
    float r = static_cast<float>(((hex >> 16) & 0xFF)) / 255.0f;
    float g = static_cast<float>(((hex >> 8) & 0xFF)) / 255.0f;
    float b = static_cast<float>((hex & 0xFF)) / 255.0f;
    auto vec = ImVec4(r, g, b, alpha);
    return vec;
}
