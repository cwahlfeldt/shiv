#ifndef SHIV_COLOR_H
#define SHIV_COLOR_H

// Color conversion functions
#include "shiv_sdl.h"

static float hue_to_rgb(float p, float q, float t) {
  if (t < 0.0f)
    t += 1.0f;
  if (t > 1.0f)
    t -= 1.0f;
  if (t < 1.0f / 6.0f)
    return p + (q - p) * 6.0f * t;
  if (t < 1.0f / 2.0f)
    return q;
  if (t < 2.0f / 3.0f)
    return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
  return p;
}

struct color color_from_hsl(float hue, float saturation, float lightness) {
  // Map these to HSL standard ranges. 0-360 for h, 0-100 for s and l
  hue = hue / 360.0f;
  saturation = saturation / 100.0f;
  lightness = lightness / 100.0f;

  if (saturation == 0.0f) {
    return (struct color){lightness, lightness, lightness, 1.0f};
  }

  const float q = lightness < 0.5f
                      ? lightness * (1.0f + saturation)
                      : lightness + saturation - lightness * saturation;
  const float p = 2.0f * lightness - q;

  return (struct color){hue_to_rgb(p, q, hue + 1.0f / 3.0f),
                        hue_to_rgb(p, q, hue),
                        hue_to_rgb(p, q, hue - 1.0f / 3.0f), 1.0f};
}

#endif // SHIV_COLOR_H
