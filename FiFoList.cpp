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
  	if (gc) XFreeGC(display, gc);
	if (window) XDestroyWindow(display, window);
}

/*********************************************************************
* Create a window for the filelist and attach it to a parent window
*********************************************************************/
void FiFoList::CreateWindow(Display *display, Window parentWindow)
{
    this->display = display;
    this->parentWindow = parentWindow;

    int screen = DefaultScreen(display);

    // Define used colors
    Colormap colormap = DefaultColormap(display, screen);
    XAllocNamedColor(display, colormap, "white", &white, &white);
    XAllocNamedColor(display, colormap, "red",   &red,   &red);
    XAllocNamedColor(display, colormap, "blue",  &blue,  &blue);
    XAllocNamedColor(display, colormap, "grey",  &grey,  &grey);
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

    // Make window visible
    XMapWindow(display, window);

/*  TODO: Add extra font to application
    // Add new font path (if needed)
    std::string fontFolder = Error::GetLogFilePath() + "/font/";

    int numFonts;
    char **fontDirListOrg;
    fontDirListOrg = XGetFontPath(display, &numFonts);
    char **fontDirListNew = new char*[numFonts+1];

    bool found = false;
    for (int i=0; i<numFonts; i++)
    {
        fontDirListNew[i] = fontDirListOrg[i];
        if (strcmp(fontDirListOrg[i], fontFolder.c_str()) == 0) found = true;
    }

    if (!found)
    {
        char newDir[fontFolder.length()+1];
        for (unsigned int i=0; i < fontFolder.length(); i++) newDir[i] = fontFolder[i];
        newDir[fontFolder.length()] = 0;
        fontDirListNew[numFonts] = newDir;
        XSetFontPath(display, fontDirListNew, numFonts + 1);
    }
*/

    // Check sizable fonts and pick one
    int number;
    char **listFonts = XListFonts(display, "*medium-r-normal--0-0-0-0*", 255, &number);

/*
    #ifdef DEBUG
        printf("%s\r\n", "Fonts available:");
        for (int i=0; i<number; i++)
        {
            printf("%s\r\n", listFonts[i]);
        }
        printf("\r\n");
    #endif // DEBUG
*/

    // Get scaled font
    std::string font = listFonts[0];
    pFont = Utils::LoadQueryScalableFont(display, screen, listFonts[0], fontScale);
    if (!pFont) fontScale = 0;

    // Get font
    if (!pFont)
    {
        std::string message = "Unable to scale font: ";
        message.append(font);
        Error::WriteLog("ERROR", "FiFoList::Attach", message.c_str());

        // Try original font
        std::string font = listFonts[0];
        pFont = XLoadQueryFont(display, font.c_str());
    }

    if (!pFont)
    {
        std::string message = "using 9x15 font, unable to load font: ";
        message.append(font);
        Error::WriteLog("ERROR", "FiFoList::Attach", message.c_str());
        font = "9x15";
        pFont = XLoadQueryFont (display, font.c_str());
    }

    // If the font could not be loaded, revert to the "fixed" font
    if (!pFont)
    {
        std::string message = "using fixed font, unable to load font: ";
        message.append(font);
        Error::WriteLog("ERROR", "FiFoList::Attach", message.c_str());
        pFont = XLoadQueryFont (display, "fixed");
    }

    // Set font
    XSetFont (display, gc, pFont->fid);

    // Set lineheight
    if (fontScale > 0) lineHeight = fontScale / 8; else lineHeight=14;

    // Fill with root folder
    Fill(currentFolder);
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

        if (*pStr == '/') pStr--;
        while ((pStr != pStrStart) && (*pStr != '/')) pStr--;
        *pStr = 0x00;

        newFolder = "";
        newFolder.append (pStrStart);
        newFolder.append ("/");
        currentFolder = newFolder;
    } else if (newFolder != ".")
    {
        currentFolder += "/";
        currentFolder += newFolder;
    }

    // Fill file list with entries from the selected folder
    succes = this->Fill(currentFolder);

    return (succes);
}

/*********************************************************************
* Fill the liststore with directories (folder names and content count)
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
* Button pressed in file/folder list, so select entry
*********************************************************************/
void FiFoList::ButtonPressed(int mouseX, int mouseY)
{
    int y = lineHeight;
    int fileOffset = offset - foList.size();

    if (fileOffset < 0)
    {
        std::list <std::string> :: iterator start = foList.begin();
        for (int i=0; i < offset; i++) start++;

        std::list <std::string> :: iterator it;
        for(it = start; it != foList.end(); it++)
        {
            // Select different background color if selected entry
            if ((mouseY > (y-5-lineHeight/2)) && mouseY < (y+5+lineHeight/2))
            {
                selectedFolder = it;
                selectedFile   = fiList.end();
                Paint();
            }

            y+=lineHeight;
        }
    }

    std::list <std::string> :: iterator start = fiList.begin();
    if (fileOffset > 0)
    {
        for (int i=0; i < fileOffset; i++) start++;
    }

    std::list <std::string> :: iterator it;
    for(it = start; it != fiList.end(); it++)
    {
        // Select different background color if selected entry
        if ((mouseY > (y-5-lineHeight/2)) && mouseY < (y+5+lineHeight/2))
        {
            selectedFile   = it;
            selectedFolder = foList.end();
            Paint();
        }

        y+=lineHeight;
    }
}

/*********************************************************************
* Show file/folder entries in window
*********************************************************************/
void FiFoList::Paint()
{
    // Clear all
    XClearWindow(display, window);

    int x = 0;
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
                XSetForeground(display, gc, red.pixel);
                XFillRectangle(display, window, gc, 0, y+5-lineHeight, width, lineHeight);
                XSetForeground(display, gc, white.pixel);
            } else
            {
                XSetForeground(display, gc, black.pixel);
            }

            std::string temp = *it;
            XDrawString (display, window, gc, x, y, temp.c_str(), strlen(temp.c_str()));

            y+=lineHeight;
        }
    }

    std::list <std::string> :: iterator start = fiList.begin();
    if (fileOffset > 0)
    {
        for (int i=0; i < fileOffset; i++) start++;
    }

    std::list <std::string> :: iterator it;
    for(it = start; it != fiList.end(); it++)
    {
        // Select different background color if selected entry
        if (it == selectedFile)
        {
            XSetForeground(display, gc, blue.pixel);
            XFillRectangle(display, window, gc, 0, y+5-lineHeight, width, lineHeight);
            XSetForeground(display, gc, white.pixel);
        } else
        {
            XSetForeground(display, gc, blue.pixel);
        }

        std::string temp = *it;
        XDrawString (display, window, gc, x, y, temp.c_str(), strlen(temp.c_str()));

        y+=lineHeight;
    }

    // Flush all pending requests to the X server
    XFlush(display);
}

/*********************************************************************
* Set window size
*********************************************************************/
void FiFoList::SetSize (int width, int height)
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
void FiFoList::SetPosition (int x, int y)
{
    XMoveWindow(display, window, width, height);

    this->x = x;
    this->y = y;
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
