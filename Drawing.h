#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <csetjmp>
#include <jpeglib.h>
#include "Error.h"
#include "Utils.h"

#undef DEBUGIMAGE

class Drawing
{
    public:

        // Window handle
        Window window;

        // Standard colors to use
        XColor white;
        XColor blue;
        XColor red;
        XColor grey;
        XColor black;

        // Coordinates of the start of the control
        unsigned int x, y;

        // Size of the control
        unsigned int width, height;

        // Color depth of the jpeg
        unsigned int pixSize;

        // Size of the image
        unsigned int imgWidth, imgHeight;

        // Constructor
        Drawing(unsigned short x, unsigned short y, unsigned short width, unsigned short height);

        // Destructor
        ~Drawing();

        // Create a window for the filelist and attach it to a parent window
        void CreateWindow(Display *display, Window parentWindow);

        // Attach this window
        void Attach(void);

        // Detach this window
        void Detach(void);

        // Load new image
        bool LoadImage(std::string path);

        // Paint image in window
        void Paint();

        // Clear image
        void Clear();

        // Resize window
        void SetSize (unsigned int width, unsigned int height);

        // Position window
        void SetPosition (unsigned int x, unsigned int y);

    protected:

    private:

        // Display handle
        Display* display;

        // Parent window
        Window parentWindow;

        // Graphical context
        GC gc;

        // Bitmap raw data
        uint32_t *pBitmap;

        // pixmap to draw
        Pixmap pixmap;
};

