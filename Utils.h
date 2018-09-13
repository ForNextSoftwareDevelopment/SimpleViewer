#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdio>
#include <ctime>
#include <string>
#include <limits>
#include <csetjmp>
#include <jpeglib.h>
#include "Error.h"

namespace Utils
{
    // Create a font name from a scalable font given a desired fontsize
    XFontStruct *LoadQueryScalableFont(Display *display, int screen, char *fontName, int fontSize);

    // Convert integer to a string
    std::string IntToStr (int i);

    // Convert floating point to a string
    std::string FloatToStr (float f);

    // Convert a string to an integer
    int StrToInt (char *pStrInt);

    // Convert a string to a floating point
    float StrToFloat (char *pStrFloat);

    // Show all window property values
    bool showWindowProperties(Display* display, Window window, std::string propname);

    // Load image
    uint32_t* LoadImage(std::string path, unsigned int *width, unsigned int *height, bool resizing, bool deforming);

    // Bilinear resize image
    uint32_t* ScaleImage(uint32_t *pBitmap, unsigned int w, unsigned int h, unsigned int w2, unsigned int h2);
}

