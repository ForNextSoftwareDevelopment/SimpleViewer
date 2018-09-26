#include "Drawing.h"

/*********************************************************************
* Constructor
*********************************************************************/
Drawing::Drawing(unsigned short x, unsigned short y, unsigned short width, unsigned short height)
{
    this->x        = x;
    this->y        = y;
    this->width    = width;
    this->height   = height;
    this->imgWidth  = 0;
    this->imgHeight = 0;
    this->pBitmap  = NULL;
}

/*********************************************************************
* Destructor
*********************************************************************/
Drawing::~Drawing()
{
    XFreePixmap(display, pixmap);
    XFreeGC(display, gc);
}

/*********************************************************************
* Create a window for the drawing and attach it to a parent window
*********************************************************************/
void Drawing::CreateWindow(Display *display, Window parentWindow)
{
    this->display = display;
    this->parentWindow = parentWindow;

    int screen = DefaultScreen(display);

    // Define used colors
    Colormap colormap = DefaultColormap(display, screen);
    XAllocNamedColor(display, colormap, "white", &white, &white);
    XAllocNamedColor(display, colormap, "red", &red, &red);
    XAllocNamedColor(display, colormap, "blue", &blue, &blue);
    XAllocNamedColor(display, colormap, "grey", &grey, &grey);
    XAllocNamedColor(display, colormap, "black", &black, &black);

    // Initialize window attributes
    XSetWindowAttributes attrs;

    // Set event masks
    attrs.event_mask = ButtonPressMask | KeyPressMask;

    // Do not hide events from child window
    attrs.do_not_propagate_mask = 0;

    // Background color
    attrs.background_pixel = white.pixel;

    // Border color
    attrs.border_pixel = black.pixel;

    unsigned long attrs_mask = CWEventMask | CWBackPixel | CWBorderPixel;

    // Create and map container window
    window = XCreateWindow(display, parentWindow, x, y, width, height, 1, CopyFromParent, InputOutput, CopyFromParent, attrs_mask, &attrs);

    // Get graphical context
    gc = XCreateGC(display, window, 0, 0);

    // define the fill style for the GC. to be 'solid filling'
    XSetFillStyle(display, gc, FillSolid);

    // Set background color
    XSetBackground(display, gc, white.pixel);

    // Make window visible
    Attach();

    // Dummy
    int depth = DefaultDepth(display, screen);
    pixmap = XCreatePixmap(display, window, width, height, depth);
}

/*********************************************************************
* Attach this window
*********************************************************************/
void Drawing::Attach()
{
    XMapWindow(display, window);
}

/*********************************************************************
* Detach this window
*********************************************************************/
void Drawing::Detach()
{
    XUnmapWindow(display, window);
}

/*********************************************************************
* Resize window
*********************************************************************/
void Drawing::SetSize (unsigned int width, unsigned int height)
{
    if (width  == 0) width =this->width;
    if (height == 0) height=this->height;

    XResizeWindow(display, window, width, height);

    this->width  = width;
    this->height = height;
}

/*********************************************************************
* Position window
*********************************************************************/
void Drawing::SetPosition (unsigned int x, unsigned int y)
{
    XMoveWindow(display, window, x, y);

    this->x = x;
    this->y = y;
}


/*********************************************************************
* Jpeg error handler
*********************************************************************/
struct jpegErrorManager
{
    // "public" fields
    struct jpeg_error_mgr pub;

    // for return to caller
    jmp_buf setjmp_buffer;
};

char jpegLastErrorMsg[JMSG_LENGTH_MAX];

void jpegErrorExit (j_common_ptr cinfo)
{
    // cinfo->err actually points to a jpegErrorManager struct
    jpegErrorManager* pJpegErrorManager = (jpegErrorManager*) cinfo->err;

    // Create the message
    ( *(cinfo->err->format_message) ) (cinfo, jpegLastErrorMsg);

    // Jump to the setjmp point
    longjmp(pJpegErrorManager->setjmp_buffer, 1);
}

/*********************************************************************
* Load new image
*********************************************************************/
bool Drawing::LoadImage(std::string path)
{
    // Load image data
    imgWidth  = width;
    imgHeight = height;

    int screen = DefaultScreen(display);
    int depth = DefaultDepth(display, screen);

    uint32_t *pImageData = Utils::LoadImage(path, &imgWidth, &imgHeight, true, false);

    if (pImageData != NULL)
    {
        // Free memory from original bitmap
        delete (pBitmap);

        // 32 for 24 and 32 bpp, 16, for 15&16
        int bitmap_pad = 32;

        // number of bytes in the client image between the start of one scanline and the start of the next
        int bytes_per_line = 0;

        XImage *pImg = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, (char*)pImageData, imgWidth, imgHeight, bitmap_pad, bytes_per_line);
        if (!pImg)
        {
            Error::WriteLog("ERROR", "Drawing::LoadImage", "Can't create image");
            imgWidth = 0;
            imgHeight = 0;
            return(false);
        }

        // Free old pixmap
        if (pixmap) XFreePixmap(display, pixmap);

        // Create new pixmap and put image in it
        pixmap = XCreatePixmap(display, window, imgWidth, imgHeight, depth);
        XPutImage(display, pixmap, gc, pImg, 0, 0, 0, 0, imgWidth, imgHeight);

        // Free image
        XDestroyImage(pImg);

        // Draw image
        Paint();
    } else
    {
        XClearArea(display, window, 0, 0, width, height, false);
        XSetForeground(display, gc, black.pixel);
        XDrawString (display, window, gc, width / 3, height / 3, "ERROR READING FILE", strlen("ERROR READING FILE"));

        return (false);
    }

    return (true);
}

/*********************************************************************
* Show image in window
*********************************************************************/
void Drawing::Paint()
{
    if (display && window && imgWidth && imgHeight)
    {
        XClearArea(display, window, 0, 0, width, height, false);
        XCopyArea(display, pixmap, window, gc, 0, 0, imgWidth, imgHeight,  (width - imgWidth) / 2, (height - imgHeight) / 2);
    }
}

/*********************************************************************
* Clear image
*********************************************************************/
void Drawing::Clear()
{
    if (display && window && imgWidth && imgHeight)
    {
        imgWidth  = 0;
        imgHeight = 0;
        XClearArea(display, window, 0, 0, width, height, false);
    }
}

