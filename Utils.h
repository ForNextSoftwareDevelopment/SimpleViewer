#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdio>
#include <ctime>
#include <string>
#include <limits>
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

    // Create a random float
    float Random(void);

    // Show all window property values
    bool showWindowProperties(Display* display, Window window, std::string propname);
}

