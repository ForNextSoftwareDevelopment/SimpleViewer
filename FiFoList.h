#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <iterator>
#include <list>

#include "Error.h"
#include "Utils.h"

#undef FONTDEBUG

enum FTYPE
{
    FFILE = 0,
    FFOLDER
};

struct FiFo
{
    std::string name;
    FTYPE       type;
    int         bytes;
};

class FiFoList
{
    public:

        // Window handle
        Window window;

        // Current folder to display
        std::string currentFolder;

        // Entry to be selected on Fill
        std::string toBeSelected;

        // Show files/folders or both
        bool showFiles;
        bool showFolders;

        // Constructor
        FiFoList(int x, int y, int width, int height, int fontScale, std::string folder);

        // Destructor
        virtual ~FiFoList();

        // Create a window for the filelist and attach it to a parent window
        void CreateWindow(Display *display, Window parentWindow);

        // Fill the list with files and folders
        bool Fill (std::string folder);

        // Button pressed in filelist, so select entry (return value: scrollbar selected)
        bool ButtonPressed(int mouseX, int mouseY);

        // Folder doubleclicked or pressed enter
        bool EnterSelectedFolder(void);

        // Go to the parent folder
        bool EnterParentFolder(void);

        // Get selected folder
        std::string GetSelectedFolder(void);

        // Get selected file
        std::string GetSelectedFile(void);

        // Scroll up
        void ScrollUp(int n);

        // Scroll down
        void ScrollDown(int n);

        // Resize window
        void SetSize (int width, int height);

        // Position window
        void SetPosition (int x, int y);

        // Paint file/folder entries in window
        void Paint();

    protected:

        // List with file/folder names
        std::list<FiFo> fifoList;

        // Selected file or folder in the list
        std::list<FiFo>::iterator selectedFiFo;

        // Coordinates of the start of the control
        int x, y;

        // Size of the control
        int width, height;

        // Lineheight (depending on fontsize used)
        int lineHeight;

        // Offset in displaying entries
        int offset;

        // Position of scrollbar slider
        int posSlider;

        // Calculate offset
        void CalculateOffset(void);

        // Test if file is an image
        bool IsImage (std::string imageName);

        // Sort list
        void Sort(void);

    private:

        // Standard colors to use
        XColor white;
        XColor blue;
        XColor red;
        XColor grey;
        XColor black;

        // Display handle
        Display* pDisplay;

        // Parent window
        Window parentWindow;

        // Graphical context
        GC gc;

        // Font to use
        XFontStruct *pFont;

        // Using scaled font
        unsigned short fontScale;
};
