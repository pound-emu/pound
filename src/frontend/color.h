#ifndef POUND_COLORS_H
#define POUND_COLORS_H

#include "imgui.h"
#include <algorithm>
#include <cstdint>

namespace gui::color
{
constexpr ImVec4 primary = ImVec4(0.0f, 0.765f, 0.890f, 1.0f);         // #00c3e3
constexpr ImVec4 primary_hover = ImVec4(0.0f, 0.865f, 0.990f, 1.0f);   // Lighter
constexpr ImVec4 primary_active = ImVec4(0.0f, 0.665f, 0.790f, 1.0f);  // Darker

// Secondary colors
constexpr ImVec4 secondary = ImVec4(1.0f, 0.271f, 0.329f, 1.0f);         // #ff4554
constexpr ImVec4 secondary_hover = ImVec4(1.0f, 0.371f, 0.429f, 1.0f);   // Lighter
constexpr ImVec4 secondary_active = ImVec4(0.9f, 0.171f, 0.229f, 1.0f);  // Darker

// Background colors
constexpr ImVec4 background = ImVec4(0.255f, 0.271f, 0.282f, 1.0f);
constexpr ImVec4 background_dark = ImVec4(0.155f, 0.171f, 0.182f, 1.0f);
constexpr ImVec4 background_light = ImVec4(0.355f, 0.371f, 0.382f, 1.0f);

// Text colors
constexpr ImVec4 text = ImVec4(0.95f, 0.96f, 0.98f, 1.0f);
constexpr ImVec4 text_disable = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);

// UI element colors
constexpr ImVec4 border = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
constexpr ImVec4 frame = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
constexpr ImVec4 frame_hover = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
constexpr ImVec4 frame_active = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);

// Special colors
constexpr ImVec4 sucess = ImVec4(0.0f, 0.8f, 0.0f, 1.0f);
constexpr ImVec4 warning = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
constexpr ImVec4 error = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
constexpr ImVec4 info = ImVec4(0.0f, 0.765f, 0.890f, 1.0f);

/*
 *  NAME
 *      with_alpha - Create a new color with adjusted alpha channel.
 *
 *  SYNOPSIS
 *      ImVec4 gui::color::with_alpha(const ImVec4& color, float alpha channel)
 *
 *  DESCRIPTION
 *      Returns a copy of the input color with the alpha component replaced by the specified value. The original color's
 *      RGB values remain unchanged.
 *
 *      This should be used when the transparency level needs to be adjusted without modifying the original color value.
 *
 *  RETURN VALUE
 *      A new ImVec4 instance with RGB values from `color` amd alpha value from `alpha`.
 *
 *  EXAMPLES
 *      ImVec4 red = ImVec4(1.0F, 0.0F, 0.0F, 1.0F);
 *      ImVec4 semi_transparent_red = gui::color::with_alpha(red, 0.5F);
 *      // semi_transparent_red = (1.0, 0.0, 0.0, 0.5)
 */
ImVec4 with_alpha(const ImVec4& color, float alpha);

/*
 *  NAME
 *      lighten - Create a new color with increased brightness.
 *
 *  SYNOPSIS
 *      ImVec4 gui::color::lighten(const ImVec& color, float amount);
 *
 *  DESCRIPTION
 *      Returns a copy of the input color with each RGB channel brightened by the specified amount.
 *
 *      Brightening works by adding the amount to each RGB component and clamping the result between 0.0F and 1.0F.
 *      This is useful for creating lighter color variations, highlights, or glow effects while maintaining the original
 *      transparency.
 *
 *  RETURN VALUE
 *      A new ImVec4 instance with brightened RGB values and unchanged alpha.
 *
 *  NOTES
 *      - Negative amounts will result in darkening instead of lightening.
 *
 *  EXAMPLES
 *      // Create a brighter version of a base color.
 *      ImVec4 blue = ImVec4(0.0F, 0.0F, 1.0F, 1.0F);
 *      ImVec4 lighter_blue = gui::color::lighten(blue, 0.3F); // (0.3, 0.3 1.0, 1.0)
 */
ImVec4 lighten(const ImVec4& color, float amount);

/*
 *  NAME
 *      darken - Create a new color ith decreased brightness.
 *
 *  SYNOPSIS
 *      ImVec4 - gui::color::darken(const ImVec4& color, float amount);
 *
 *  DESCRIPTION
 *      Return a copy of the input color with each RGB channel darkened by the specified amount.
 *
 *      Darkening works by subtracting the amount from each RGB component and ensuring the result does not go below 0.0F.
 *      This operation is useful for creating darker color variation, shadows, or dimmed effects while maintaining the
 *      original transparency.
 *
 *  RETURNS
 *      A new ImVec4 instance with darkened RGB values and unchanged alpha.
 *
 *  NOTES
 *      - Negative amounts will result in lightening instead of darkening.
 *
 *  EXAMPLES
 *      // Create a darker version of a base color.
 *      ImVec4 yellow = ImVec4(1.0F, 1.0F, 0.0F, 1.0F);
 *      ImVec4 dark_yellow = gui::color::darken(yellow, 0.5F); // (0.5, 0.5, 0.0, 1.0)
 */
ImVec4 darken(const ImVec4& color, float amount);

/*
 *  NAME
 *      from_hex - Convert a hex color value to an ImVec4 color with specified alpha.
 *
 *  SYNOPSIS
 *      ImVec4 gui::color::from_hex(uint32_t hex, float alpha);
 *
 *  DESCRIPTION
 *      Converts a 24-bit hexadecimal color value (RRGGBB format) to an ImVec4 color structure. The input hex value is
 *      interpreted as having three bytes: red, green, and blue components in that order. Each component ranges from
 *      0x00 to 0xFF and is normalized to the ranged [0.0F, 1.0F] for the output.
 *
 *  RETURN VALUE
 *      A new ImVec4 instance with RGB values from hex specified alpha.
 *
 *  EXAMPLES
 *      // Convert pure red with full opacity.
 *      ImVec4 red = gui::color::from_hex(0xFF0000, 1.0F); // (1.0, 0.0, 0.0, 1.0)
 */
ImVec4 from_hex(uint32_t hex, float alpha);
}  // namespace gui::color

#endif  //POUND_COLORS_H
