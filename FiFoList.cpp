#include "FiFoList.h"

/*********************************************************************
* Constructor
*********************************************************************/
FiFoList::FiFoList(int x, int y, int width, int height, int fontScale, std::string folder)
{
    this->x               = x;
    this->y               = y;
    this->width           = width;
    this->height          = height;
    this->fontScale       = fontScale;
    this->currentFolder   = folder;
    this->selectedFile    = fiList.end();
    this->selectedFolder  = foList.end();
    this->selectedItem    = 0;
    this->showFiles       = true;
    this->showFolders     = true;
    this->offset          = 0;
}

/*********************************************************************
* Destructor
*********************************************************************/
FiFoList::~FiFoList()
{
    // Free resources
  	if (gc) XFreeGC(pDisplay, gc);
	if (window) XDestroyWindow(pDisplay, window);
}

/*********************************************************************
* Create a window for the filelist and attach it to a parent window
*********************************************************************/
void FiFoList::CreateWindow(Display *pDisplay, Window parentWindow)
{
    this->pDisplay = pDisplay;
    this->parentWindow = parentWindow;

    int screen = DefaultScreen(pDisplay);

    // Define used colors
    Colormap colormap = DefaultColormap(pDisplay, screen);
    XAllocNamedColor(pDisplay, colormap, "white", &white, &white);
    XAllocNamedColor(pDisplay, colormap, "red",   &red,   &red);
    XAllocNamedColor(pDisplay, colormap, "blue",  &blue,  &blue);
    XAllocNamedColor(pDisplay, colormap, "grey",  &grey,  &grey);
    XAllocNamedColor(pDisplay, colormap, "black", &black, &black);

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
    window = XCreateWindow(pDisplay, parentWindow, x, y, width, height, 3, CopyFromParent, InputOutput, CopyFromParent, attrs_mask, &attrs);

    // Get graphical context
    gc = XCreateGC(pDisplay, window, 0, 0);

    // Make window visible
    XMapWindow(pDisplay, window);

    // Check sizable fonts and pick one
    int number;
    char **listFonts = XListFonts(pDisplay, "*medium-r-normal--0-0-0-0*", 255, &number);

    #ifdef FONTDEBUG
        printf("%s\r\n", "Fonts available:");
        for (int i=0; i<number; i++)
        {
            printf("%s\r\n", listFonts[i]);
        }
        printf("\r\n");
    #endif // FONTDEBUG

    // Get scaled font
    std::string font = listFonts[0];
    pFont = Utils::LoadQueryScalableFont(pDisplay, screen, listFonts[0], fontScale);
    if (!pFont) fontScale = 0;

    // Get font
    if (!pFont)
    {
        std::string message = "Unable to scale font: ";
        message.append(font);
        Error::WriteLog("ERROR", "FiFoList::Attach", message.c_str());

        // Try original font
        std::string font = listFonts[0];
        pFont = XLoadQueryFont(pDisplay, font.c_str());
    }

    if (!pFont)
    {
        std::string message = "using 9x15 font, unable to load font: ";
        message.append(font);
        Error::WriteLog("ERROR", "FiFoList::Attach", message.c_str());
        font = "9x15";
        pFont = XLoadQueryFont (pDisplay, font.c_str());
    }

    // If the font could not be loaded, revert to the "fixed" font
    if (!pFont)
    {
        std::string message = "using fixed font, unable to load font: ";
        message.append(font);
        Error::WriteLog("ERROR", "FiFoList::Attach", message.c_str());
        pFont = XLoadQueryFont (pDisplay, "fixed");
    }

    // Set font
    XSetFont (pDisplay, gc, pFont->fid);

    // Set lineheight
    if (fontScale > 0) lineHeight = fontScale / 8; else lineHeight=15;

    // Fill with root folder
    Fill(currentFolder);
}

/*********************************************************************
* Fill the liststore
*********************************************************************/
bool FiFoList::Fill (std::string folder)
{
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(folder.c_str())) == NULL)
    {
        std::string message = "Error opening folder: ";
        message.append(folder);
        Error::WriteLog("ERROR", "FiFoList::Fill", message.c_str());
        return (false);
    }

    currentFolder = folder;
    fiList.clear();
    foList.clear();
    selectedFolder = foList.end();
    selectedFile   = fiList.end();

    if (showFolders)
    {
        // Add current folder
        foList.push_back(".");

        // Add parent folder
        if (currentFolder != "/")
        {
            foList.push_back("..");
        }
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        if (dirp->d_name[0] != '.')
        {
            // File
            if (dirp->d_type == DT_REG)
            {
                if (showFiles)
                {
                    // Check if this is an image
                    if (IsImage(dirp->d_name))
                    {
                        fiList.push_back(dirp->d_name);
                    }
                }
            }

            // Folder
            if (dirp->d_type == DT_DIR)
            {
                if (showFolders)
                {
                    foList.push_back(dirp->d_name);
                }
            }
        }
    }

    // Sort the lists alphabettically
    fiList.sort();
    foList.sort();

    // Set selected to start of foList
    if (showFolders) selectedFolder = foList.begin();
    if (!showFolders && showFiles && (fiList.size() > 0)) selectedFile = fiList.begin();

    // Show in window
    Paint();

    return (true);
}

/*********************************************************************
* File or Folder doubleclicked or pressed enter
*********************************************************************/
bool FiFoList::EnterSelectedFolder(void)
{
    bool succes = true;
    std::list <std::string> :: iterator it;
    std::string newFolder = *selectedFolder;

    // Goto parent folder
    if (newFolder == "..")
    {
        char *pStr = (char *) currentFolder.c_str();
        char *pStrStart = pStr;
        pStr += currentFolder.length() - 1;

        if (*pStr == '/') *pStr=0x00;
        while ((pStr != pStrStart) && (*pStr != '/')) pStr--;
        *pStr = 0x00;

        newFolder = "";
        newFolder.append (pStrStart);
        newFolder.append ("/");
        currentFolder = newFolder;
    } else if (newFolder != ".")
    {
        if (currentFolder[currentFolder.length() - 1] != '/') currentFolder += "/";
        currentFolder += newFolder;
    }

    // Fill file list with entries from the selected folder
    succes = this->Fill(currentFolder);

    return (succes);
}

/*********************************************************************
* Scroll up
*********************************************************************/
void FiFoList::ScrollUp(int n)
{
    for (int i=0; i<n; i++)
    {
        if (selectedFolder != foList.end())
        {
            // Scroll in folderlist
            if (selectedFolder != foList.begin())
            {
                selectedFolder--;
            }
        } else
        if (selectedFile != fiList.end())
        {
            // Scroll in filelist
            if (selectedFile != fiList.begin())
            {
                selectedFile--;
            } else
            {
                // Goto folderlist
                selectedFile = fiList.end();
                selectedFolder = foList.end();
                selectedFolder--;
            }
        }
    }

    CalculateOffset();
    Paint();
}

/*********************************************************************
* Scroll down
*********************************************************************/
void FiFoList::ScrollDown(int n)
{
    for (int i=0; i<n; i++)
    {
        if (selectedFolder != foList.end())
        {
            // Scroll in folderlist
            selectedFolder++;
            if (selectedFolder == foList.end())
            {
                if (fiList.size() > 0)
                {
                    selectedFile = fiList.begin();
                } else
                {
                    selectedFolder--;
                }
            }
        } else
        if (selectedFile != fiList.end())
        {
            // Scroll in filelist
            selectedFile++;
            if (selectedFile == fiList.end())
            {
                selectedFile--;
            }
        }
    }

    CalculateOffset();
    Paint();
}

/*********************************************************************
* Calculate offset
*********************************************************************/
void FiFoList::CalculateOffset(void)
{
    int y = 2 * lineHeight;

    // Calculate offset (folder selected)
    if (selectedFolder != foList.end())
    {
        // Find selected folder
        std::list <std::string> :: iterator it;
        for(it = foList.begin(); it != foList.end() && it != selectedFolder; it++)
        {
            y+=lineHeight;
        }

        if (y > (height -lineHeight))
        {
            offset = (y - height) / lineHeight;
        } else
        {
            offset = 0;
        }
    }

    // Calculate offset (file selected)
    if (selectedFile != fiList.end())
    {
        // Add size of folders
        y += (foList.size() * lineHeight);

        // Find selected file
        std::list <std::string> :: iterator it;
        for(it = fiList.begin(); it != fiList.end() && it != selectedFile; it++)
        {
            y+=lineHeight;
        }

        if (y > (height -lineHeight))
        {
            offset = (y - height) / lineHeight;
        } else
        {
            offset = 0;
        }
    }
}

/*********************************************************************
* Button pressed in file/folder list, so select entry
* (return value: scrollbar selected)
*********************************************************************/
bool FiFoList::ButtonPressed(int mouseX, int mouseY)
{
    // Check if clicked in scrollbar
    if (mouseX > (width - 10))
    {
        int step = (foList.size() + fiList.size()) / 10;
        if (step == 0) step = 1;
        if (mouseY < posSlider) ScrollUp(step);
        else ScrollDown(step);

        return(true);
    } else
    {
        int y = lineHeight;
        int fileOffset = offset - foList.size();

        if (fileOffset < 0)
        {
            std::list <std::string> :: iterator start = foList.begin();
            for (int i=0; i < offset; i++) start++;

            // Search selected folder entry
            std::list <std::string> :: iterator it;
            for(it = start; it != foList.end(); it++)
            {
                if ((mouseY > (y-5-lineHeight/2)) && mouseY < (y+5+lineHeight/2))
                {
                    selectedFolder = it;
                    selectedFile   = fiList.end();
                }

                y+=lineHeight;
            }
        }

        std::list <std::string> :: iterator start = fiList.begin();
        if (fileOffset > 0)
        {
            for (int i=0; i < fileOffset; i++) start++;
        }

        // Search selected file entry
        std::list <std::string> :: iterator it;
        for(it = start; it != fiList.end(); it++)
        {
            if ((mouseY > (y-5-lineHeight/2)) && mouseY < (y+5+lineHeight/2))
            {
                selectedFile   = it;
                selectedFolder = foList.end();
            }

            y+=lineHeight;
        }
    }

    Paint();
    return(false);
}

/*********************************************************************
* Show file/folder entries in window
*********************************************************************/
void FiFoList::Paint()
{
    std::list <std::string> :: iterator it;
    selectedItem = 0;

    // Total number of items
    int numberItems;

    // Set total number of items
    numberItems = foList.size() + fiList.size();

    // Search selected folder entry
    for(it = foList.begin(); (it != foList.end()) && (it != selectedFolder); it++)
    {
        selectedItem++;
    }

    // Search selected file entry
    if (it == foList.end())
    {
        // Search selected file entry
        for(it = fiList.begin(); (it != fiList.end()) && (it != selectedFile); it++)
        {
            selectedItem++;
        }
    }

    // Clear all
    XClearWindow(pDisplay, window);

    int x = 0;

    y = lineHeight;
    int fileOffset = offset - foList.size();

    if (fileOffset < 0)
    {
        std::list <std::string> :: iterator start = foList.begin();
        for (int i=0; i<offset; i++) start++;

        std::list <std::string> :: iterator it;
        for(it = start; it != foList.end(); it++)
        {
            // Select different background color if selected entry
            if (it == selectedFolder)
            {
                XSetForeground(pDisplay, gc, red.pixel);
                XFillRectangle(pDisplay, window, gc, 0, y+5-lineHeight, width-10, lineHeight);
                XSetForeground(pDisplay, gc, white.pixel);
            } else
            {
                XSetForeground(pDisplay, gc, black.pixel);
            }

            std::string temp = *it;
            XDrawString (pDisplay, window, gc, x, y, temp.c_str(), strlen(temp.c_str()));

            y+=lineHeight;
        }
    }

    std::list <std::string> :: iterator start = fiList.begin();
    if (fileOffset > 0)
    {
        for (int i=0; i < fileOffset; i++) start++;
    }

    for(it = start; it != fiList.end(); it++)
    {
        // Select different background color if selected entry
        if (it == selectedFile)
        {
            XSetForeground(pDisplay, gc, blue.pixel);
            XFillRectangle(pDisplay, window, gc, 0, y+5-lineHeight, width-10, lineHeight);
            XSetForeground(pDisplay, gc, white.pixel);
        } else
        {
            XSetForeground(pDisplay, gc, blue.pixel);
        }

        std::string temp = *it;
        XDrawString (pDisplay, window, gc, x, y, temp.c_str(), strlen(temp.c_str()));

        y+=lineHeight;
    }

    // Draw scrollbar
    XSetForeground(pDisplay, gc, black.pixel);
    XDrawLine(pDisplay, window, gc, width - 10, 0, width -10, height);

    // Draw marker in scrollbar
    XSetForeground(pDisplay, gc, red.pixel);
    posSlider = (selectedItem + 0.5) * height / numberItems;
    XFillRectangle(pDisplay, window, gc, width - 9, posSlider-4, width -1, 4);
    XSetForeground(pDisplay, gc, blue.pixel);
    XFillRectangle(pDisplay, window, gc, width - 9, posSlider, width -1, 4);

    // Flush all pending requests to the X server
    XFlush(pDisplay);
}

/*********************************************************************
* Set window size
*********************************************************************/
void FiFoList::SetSize (int width, int height)
{
    if (width  == 0) width =this->width;
    if (height == 0) height=this->height;

    XResizeWindow(pDisplay, window, width, height);

    this->width  = width;
    this->height = height;

    Paint();
}

/*********************************************************************
* Position window
*********************************************************************/
void FiFoList::SetPosition (int x, int y)
{
    XMoveWindow(pDisplay, window, width, height);

    this->x = x;
    this->y = y;

    Paint();
}

/*********************************************************************
* Test if the file is an image
*********************************************************************/
bool FiFoList::IsImage (std::string fileName)
{
    int i = fileName.length() - 1;
    while ((i >= 0) && (fileName[i] != '.')) i--;
    if ((tolower(fileName[i+1]) == 'j') && (tolower(fileName[i+2]) == 'p') && (tolower(fileName[i+3]) == 'g')) return (true);
    if ((tolower(fileName[i+1]) == 'j') && (tolower(fileName[i+2]) == 'p') && (tolower(fileName[i+3]) == 'e')) return (true);

    return (false);
}
