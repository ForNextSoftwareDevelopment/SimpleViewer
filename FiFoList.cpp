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
    this->selectedFiFo    = fifoList.end();
    this->toBeSelected    = "";
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

    // Reset variables
    currentFolder = folder;
    fifoList.clear();
    selectedFiFo = fifoList.end();
    offset = 0;

    // Add current and parent folder
    if (showFolders)
    {
        FiFo fifo = {".", FTYPE::FFOLDER, 0};

        // Add current folder
        fifoList.push_back(fifo);

        // Add parent folder
        if (currentFolder != "/")
        {
            fifo.name = "..";
            fifoList.push_back(fifo);
        }
    }

    // Read all files/folders
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
                        // Check filesize
                        struct stat st;
                        int bytes = 0;
                        std::string imgName = currentFolder;
                        if (imgName[imgName.size()-1] != '/')  imgName.append("/");
                        imgName.append(dirp->d_name);
                        if (stat(imgName.c_str(), &st) == 0) bytes = st.st_size;

                        // New entry
                        FiFo fifo = {dirp->d_name, FTYPE::FFILE, bytes};
                        fifoList.push_back(fifo);
                    }
                }
            }

            // Folder
            if (dirp->d_type == DT_DIR)
            {
                if (showFolders)
                {
                    FiFo fifo = {dirp->d_name, FTYPE::FFOLDER, 0};
                    fifoList.push_back(fifo);
                }
            }
        }
    }

    // Sort the list alphabettically
    Sort();

    // Set selected to the first entry
    selectedFiFo = fifoList.begin();

    // Check if toBeSelected entry is valid, if so -> select
    std::list<FiFo>::iterator it;
    for(it = fifoList.begin(); it != fifoList.end(); it++)
    {
        if ((*it).name == toBeSelected) selectedFiFo = it;
    }

    // Show in window
    Paint();

    return (true);
}

/*********************************************************************
* Sort list
*********************************************************************/
void FiFoList::Sort(void)
{
    std::list<FiFo> tempFolders;
    std::list<FiFo> tempFiles;

    // Copy all items to temp lists
    std::list<FiFo>::iterator it;
    for(it = fifoList.begin(); it != fifoList.end(); it++)
    {
        if ((*it).type == FTYPE::FFOLDER) tempFolders.push_back(*it);
        if ((*it).type == FTYPE::FFILE) tempFiles.push_back(*it);
    }

    // Clear old list
    fifoList.clear();

    // Folders allways on top
    while (tempFolders.size() > 0)
    {
        std::list<FiFo>::iterator first = tempFolders.begin();
        for(it = tempFolders.begin(); it != tempFolders.end(); it++)
        {
            bool exitLoop = false;
            bool change = false;
            for (unsigned int i=0; (i<(*first).name.size()) && (i<(*it).name.size()) && !exitLoop; i++)
            {
                char iFirst = std::tolower((*first).name[i]);
                char iIt = std::tolower((*it).name[i]);
                if (iIt > iFirst)
                {
                    exitLoop = true;
                    change = false;
                }
                if (iIt < iFirst)
                {
                    exitLoop = true;
                    change = true;
                }
            }

            if (change) first = it;
        }

        fifoList.push_back(*first);
        tempFolders.erase(first);
    }

    // Files next
    while (tempFiles.size() > 0)
    {
        std::list<FiFo>::iterator first = tempFiles.begin();
        for(it = tempFiles.begin(); it != tempFiles.end(); it++)
        {
            bool exitLoop = false;
            bool change = false;
            for (unsigned int i=0; (i<(*first).name.size()) && (i<(*it).name.size()) && !exitLoop; i++)
            {
                char iFirst = std::tolower((*first).name[i]);
                char iIt = std::tolower((*it).name[i]);
                if (iIt > iFirst)
                {
                    exitLoop = true;
                    change = false;
                }
                if (iIt < iFirst)
                {
                    exitLoop = true;
                    change = true;
                }
            }

            if (change) first = it;
        }

        fifoList.push_back(*first);
        tempFiles.erase(first);
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
        int step = fifoList.size() / 10;
        if (step == 0) step = 1;
        if (mouseY < posSlider) ScrollUp(step);
        else ScrollDown(step);

        return(true);
    } else
    {
        int yText = lineHeight;

        std::list<FiFo>::iterator start = fifoList.begin();
        for (int i=0; i < offset; i++) start++;

        // Search selected entry
        std::list<FiFo>::iterator it;
        for(it = start; it != fifoList.end(); it++)
        {
            if ((mouseY > (yText-5-lineHeight/2)) && mouseY < (yText+5+lineHeight/2))
            {
                selectedFiFo = it;
            }

            yText+=lineHeight;
        }
    }

    Paint();
    return(false);
}

/*********************************************************************
* Folder doubleclicked or pressed enter
*********************************************************************/
bool FiFoList::EnterSelectedFolder(void)
{
    std::string newFolder = (*selectedFiFo).name;
    bool succes = true;

    // Check if folder selected
    if ((*selectedFiFo).type != FTYPE::FFOLDER) return(false);

    // Goto parent folder
    if (newFolder == "..")
    {
        char *pStr = (char *) currentFolder.c_str();
        char *pStrStart = pStr;
        pStr += currentFolder.length() - 1;

        if (*pStr == '/') *pStr=0x00;
        while ((pStr != pStrStart) && (*pStr != '/')) pStr--;

        // Set folder from which we came
        toBeSelected = "";
        toBeSelected.append(pStr+1);

        // Set string endmarker
        *pStr = 0x00;

        newFolder = "";
        newFolder.append (pStrStart);
        newFolder.append ("/");
        currentFolder = newFolder;
    } else if (newFolder != ".")
    {
        if (currentFolder[currentFolder.length() - 1] != '/') currentFolder += "/";
        currentFolder += newFolder;

        toBeSelected = "";
    }

    // Fill file list with entries from the selected folder
    succes = this->Fill(currentFolder);

    return (succes);
}

/*********************************************************************
* Enter the parent folder
*********************************************************************/
bool FiFoList::EnterParentFolder(void)
{
    bool succes = true;

    char *pStr = (char *) currentFolder.c_str();
    char *pStrStart = pStr;
    pStr += currentFolder.length() - 1;

    if (*pStr == '/') *pStr=0x00;
    while ((pStr != pStrStart) && (*pStr != '/')) pStr--;

    // Set folder from which we came
    toBeSelected = "";
    toBeSelected.append(pStr+1);

    // Set string endmarker
    *pStr = 0x00;

    std::string newFolder = "";
    newFolder.append (pStrStart);
    newFolder.append ("/");
    currentFolder = newFolder;

    // Fill file list with entries from the selected folder
    succes = this->Fill(currentFolder);

    return (succes);
}

/*********************************************************************
* Get selected folder
*********************************************************************/
std::string FiFoList::GetSelectedFolder(void)
{
    if ((*selectedFiFo).type == FTYPE::FFOLDER)
    {
        return ((*selectedFiFo).name);
    }

    return ("");
}

/*********************************************************************
* Get selected file
*******************************************************************/
std::string FiFoList::GetSelectedFile(void)
{
    if ((*selectedFiFo).type == FTYPE::FFILE)
    {
        return ((*selectedFiFo).name);
    }

    return ("");
}

/*********************************************************************
* Scroll up
*********************************************************************/
void FiFoList::ScrollUp(int n)
{
    for (int i=0; i<n; i++)
    {
        if (selectedFiFo != fifoList.begin())
        {
            selectedFiFo--;
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
        if (selectedFiFo != fifoList.end())
        {
            selectedFiFo++;
            if (selectedFiFo == fifoList.end())
            {
                selectedFiFo--;
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
    int yText = 2 * lineHeight;

    // Find selected file/folder
    std::list<FiFo>::iterator it;
    for(it = fifoList.begin(); (it != fifoList.end()) && (it != selectedFiFo); it++)
    {
        yText+=lineHeight;
    }

    if (yText > (height -lineHeight))
    {
        offset = (yText - height) / lineHeight;
    } else
    {
        offset = 0;
    }
}

/*********************************************************************
* Show file/folder entries in window
*********************************************************************/
void FiFoList::Paint()
{
    // Clear all
    XClearWindow(pDisplay, window);

    // Check offset
    std::list<FiFo>::iterator start = fifoList.begin();
    for (int i=0; i<offset; i++) start++;

    // Check max filename size
    unsigned int maxFileNameSize = 0;
    std::list<FiFo>::iterator it;
    for(it = fifoList.begin(); it != fifoList.end(); it++)
    {
        if ((*it).name.size() > maxFileNameSize) maxFileNameSize = (*it).name.size();
    }
    int xSize = maxFileNameSize * fontScale / 10;

    // Paint all file names and sizes
    int xText = 0;
    int yText = lineHeight;
    for(it = start; it != fifoList.end(); it++)
    {
        // Select different background color if selected entry is a folder
        if ((*it).type == FTYPE::FFOLDER)
        {
            XSetForeground(pDisplay, gc, red.pixel);

            if (it == selectedFiFo)
            {
                XFillRectangle(pDisplay, window, gc, 0, yText+5-lineHeight, width-10, lineHeight);
                XSetForeground(pDisplay, gc, white.pixel);
            }
        }

        // Select different background color if selected entry is a file
        if ((*it).type == FTYPE::FFILE)
        {
            XSetForeground(pDisplay, gc, blue.pixel);

            if (it == selectedFiFo)
            {
                XFillRectangle(pDisplay, window, gc, 0, yText+5-lineHeight, width-10, lineHeight);
                XSetForeground(pDisplay, gc, white.pixel);
            }
        }

        // Name
        XDrawString (pDisplay, window, gc, xText, yText, (*it).name.c_str(), strlen((*it).name.c_str()));

        // Size
        if ((*it).type == FTYPE::FFILE)
        {
            std::string strSize = Utils::IntToStr((*it).bytes);
            XDrawString (pDisplay, window, gc, xSize, yText, strSize.c_str(), strlen(strSize.c_str()));
        }

        yText+=lineHeight;
    }

    // Draw scrollbar
    XSetForeground(pDisplay, gc, black.pixel);
    XDrawLine(pDisplay, window, gc, width - 10, 0, width -10, height);
    XSetForeground(pDisplay, gc, white.pixel);
    XFillRectangle(pDisplay, window, gc, width - 9, 0, 9, height);

    // Search selected entry (nummerical value)
    int selectedItem = 0;
    for(it = fifoList.begin(); (it != fifoList.end()) && (it != selectedFiFo); it++)
    {
        selectedItem++;
    }

    // Draw marker in scrollbar
    XSetForeground(pDisplay, gc, red.pixel);
    posSlider = (selectedItem + 0.5) * height / fifoList.size();
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

    CalculateOffset();
}

/*********************************************************************
* Position window
*********************************************************************/
void FiFoList::SetPosition (int x, int y)
{
    XMoveWindow(pDisplay, window, width, height);

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
