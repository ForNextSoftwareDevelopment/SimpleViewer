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
    this->imWidth  = 0;
    this->imHeight = 0;
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

    // handle mouse button
    attrs.event_mask = ButtonPressMask | KeyPressMask;

    // Do not hide any events from child window
    attrs.do_not_propagate_mask = 0;

    // Background color
    attrs.background_pixel = white.pixel;

    // Border color
    attrs.border_pixel = black.pixel;

    unsigned long attrs_mask = CWEventMask | CWBackPixel | CWBorderPixel;

    // Create and map container window
    window = XCreateWindow(display, parentWindow, x, y, width, height, 3, CopyFromParent, InputOutput, CopyFromParent, attrs_mask, &attrs);

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
bool Drawing::LoadImage(std::string img)
{
    FILE *f;
    unsigned char a, r, g, b;
    struct jpeg_decompress_struct cinfo;

    // Output row buffer
    JSAMPARRAY pJpegBuffer;

    // physical row width in output buffer
    int row_stride;

    f = fopen(img.c_str(), "rb");
    if (!f) {
        std::string message = "Can't open imagefile: " + img;
        Error::WriteLog("ERROR", "Drawing::LoadImage", message.c_str());
        return (false);
    }

    // We set up the normal JPEG error routines, then override error_exit
    jpegErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;

    // Establish the setjmp return context for my_error_exit to use
    if (setjmp(jerr.setjmp_buffer))
    {
        std::string message = "Can't read image: " + img;
        message.append("\r\nError: ");
        message.append(jpegLastErrorMsg);
        Error::WriteLog("ERROR", "Drawing::LoadImage", message.c_str());
        jpeg_destroy_decompress(&cinfo);
        fclose(f);
        return (false);
    }

    // Create header info struct for jpg
    jpeg_create_decompress(&cinfo);

    // Set the file as source
    jpeg_stdio_src(&cinfo, f);

    // Read jpeg header
    bool result = jpeg_read_header(&cinfo, TRUE);
    if (result != 1)
    {
        std::string message = "Bad jpeg: " + img;
        Error::WriteLog("ERROR", "Drawing::LoadImage", message.c_str());
        fclose(f);
        return (false);
    }

    (void) jpeg_start_decompress(&cinfo);
    imWidth  = cinfo.output_width;
    imHeight = cinfo.output_height;
    pixSize  = cinfo.output_components;

    printf("depth = %d\r\n", pixSize);

    #ifdef DEBUGIMAGE
        std::string message = "Image is ";
        message.append(Utils::IntToStr(imWidth));
        message.append(" by ");
        message.append(Utils::IntToStr(imHeight));
        message.append(" and with a depth of ");
        message.append(Utils::IntToStr(pixSize));
        message.append(" bytes per pixel");
        Error::WriteLog("INFO", "Drawing::LoadImage", message.c_str());
    #endif

    // Bitmap to draw
    pBitmap = new uint32_t [imWidth * imHeight];
    unsigned char *ptr = (unsigned char *)pBitmap;

    row_stride = imWidth * pixSize;
    pJpegBuffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    while (cinfo.output_scanline < cinfo.output_height)
    {
        (void) jpeg_read_scanlines(&cinfo, pJpegBuffer, 1);
        for (unsigned int x = 0; x < imWidth; x++)
        {
            a = 0; // alpha value is not supported on jpg
            r = pJpegBuffer[0][cinfo.output_components * x];
            if (cinfo.output_components > 2)
            {
                g = pJpegBuffer[0][cinfo.output_components * x + 1];
                b = pJpegBuffer[0][cinfo.output_components * x + 2];
            } else
            {
                g = r;
                b = r;
            }

            *ptr++ = b;
            *ptr++ = g;
            *ptr++ = r;
            *ptr++ = a;
        }
    }

    // Free memory
    if (jpeg_finish_decompress(&cinfo) == 0)
    {
        std::string message = "Can't decompress: " + img;
        Error::WriteLog("ERROR", "Drawing::LoadImage", message.c_str());
        jpeg_destroy_decompress(&cinfo);
        fclose(f);
        return (false);
    }

    jpeg_destroy_decompress(&cinfo);

    // Close file
    fclose(f);

    // Check resize for deformation
    imResHeight = height;
    imResWidth = width;
    float x_ratio = ((float)imWidth)/(float)width;
    float y_ratio = ((float)imHeight)/(float)height;
    if (y_ratio > x_ratio)
    {
        imResWidth = (unsigned int)((float)imWidth / y_ratio);
    }

    if (x_ratio > y_ratio)
    {
        imResHeight = (unsigned int)((float)imHeight / x_ratio);
    }

    // Get resized picture
    uint32_t *pScaledBitmap = ScaleImage(pBitmap, imWidth, imHeight, imResWidth, imResHeight);

    // Free memory from original bitmap
    delete (pBitmap);

    // 32 for 24 and 32 bpp, 16, for 15&16
    int bitmap_pad = 32;

    // number of bytes in the client image between the start of one scanline and the start of the next
    int bytes_per_line = 0;

    int screen = DefaultScreen(display);
    int depth = DefaultDepth(display, screen);

    XImage *pImg = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, (char*)pScaledBitmap, imResWidth, imResHeight, bitmap_pad, bytes_per_line);
    if (!pImg)
    {
        Error::WriteLog("ERROR", "Drawing::ReadJPEG", "Can't create image");
        imWidth = 0;
        imHeight = 0;
        return(false);
    }

    // Free old pixmap
    if (pixmap) XFreePixmap(display, pixmap);

    // Create new pixmap and put image in it
    pixmap = XCreatePixmap(display, window, imResWidth, imResHeight, depth);
    XPutImage(display, pixmap, gc, pImg, 0, 0, 0, 0, imResWidth, imResHeight);

    // Free image
    XDestroyImage(pImg);

    // Draw image
    Paint();

    return (true);
}

/*********************************************************************
* Show image in window
*********************************************************************/
void Drawing::Paint()
{
    if (display && window && imWidth && imHeight)
    {
        XClearArea(display, window, 0, 0, width, height, false);
        XCopyArea(display, pixmap, window, gc, 0, 0, imResWidth, imResHeight,  (width - imResWidth) / 2, (height - imResHeight) / 2);
    }
}

/*********************************************************************
* Bilinear resize image
*********************************************************************/
uint32_t* Drawing::ScaleImage(uint32_t *pBitmap, unsigned int w, unsigned int h, unsigned int w2, unsigned int h2)
{
    uint32_t *temp = new uint32_t[w2*h2];
    uint32_t a, b, c, d;
    unsigned int x, y, index;
    float x_diff, y_diff, blue, red, green;
    uint32_t offset = 0;
    float x_ratio = ((float)(w-1))/(float)w2;
    float y_ratio = ((float)(h-1))/(float)h2;

    for (unsigned int i=0;i<h2;i++)
    {
        for (unsigned int j=0;j<w2;j++)
        {
            x = (unsigned int)(x_ratio * j);
            y = (unsigned int)(y_ratio * i);
            x_diff = (x_ratio * j) - x;
            y_diff = (y_ratio * i) - y;
            index = (y*w+x);
            a = pBitmap[index];
            b = pBitmap[index+1];
            c = pBitmap[index+w];
            d = pBitmap[index+w+1];

            // blue element
            // Yb = Ab(1-w)(1-h) + Bb(w)(1-h) + Cb(h)(1-w) + Db(wh)
            blue = (a&0xff)*(1-x_diff)*(1-y_diff) + (b&0xff)*(x_diff)*(1-y_diff) +
                   (c&0xff)*(y_diff)*(1-x_diff)   + (d&0xff)*(x_diff*y_diff);

            // green element
            // Yg = Ag(1-w)(1-h) + Bg(w)(1-h) + Cg(h)(1-w) + Dg(wh)
            green = ((a>>8)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>8)&0xff)*(x_diff)*(1-y_diff) +
                    ((c>>8)&0xff)*(y_diff)*(1-x_diff)   + ((d>>8)&0xff)*(x_diff*y_diff);

            // red element
            // Yr = Ar(1-w)(1-h) + Br(w)(1-h) + Cr(h)(1-w) + Dr(wh)
            red = ((a>>16)&0xff)*(1-x_diff)*(1-y_diff) + ((b>>16)&0xff)*(x_diff)*(1-y_diff) +
                  ((c>>16)&0xff)*(y_diff)*(1-x_diff)   + ((d>>16)&0xff)*(x_diff*y_diff);

            temp[offset++] =
                    0xff000000 | // hardcode alpha
                    ((((uint32_t)red)<<16)&0xff0000) |
                    ((((uint32_t)green)<<8)&0xff00) |
                    ((uint32_t)blue);
        }
    }

    return temp;
}
