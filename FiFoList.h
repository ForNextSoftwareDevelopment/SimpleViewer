#pragma once
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <iterator>
#include <list>

#include "Error.h"
#include "Utils.h"

class FiFoList
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

        // List with file names
        std::list<std::string> fiList;

        // List with folder names
        std::list<std::string> foList;

        // Current Folder to display
        std::string currentFolder;

        // Selected item in the list
        std::list <std::string> :: iterator selectedFile;
        std::list <std::string> :: iterator selectedFolder;

        // Show files/folders or both
        bool showFiles;
        bool showFolders;

        // Coordinates of the start of the control
        int x, y;

        // Size of the control
        int width, height;

        // Constructor
        FiFoList(int x, int y, int width, int height, int fontScale, std::string folder);

        // Destructor
        virtual ~FiFoList();

        // Create a window for the filelist and attach it to a parent window
        void CreateWindow(Display *display, Window parentWindow);

        // Fill the list with files and folders
        bool Fill (std::string folder);

        // Button pressed in filelist, so select entry
        void ButtonPressed(int mouseX, int mouseY);

        // Folder doubleclicked or pressed enter
        bool EnterSelectedFolder(void);

        // Paint file/folder entries in window
        void Paint();

        // Resize window
        void SetSize (int width, int height);

        // Position window
        void SetPosition (int x, int y);

    protected:

    private:

        // Display handle
        Display* display;

        // Parent window
        Window parentWindow;

        // Graphical context
        GC gc;

        // Font to use
        XFontStruct *pFont;

        // Using scaled font
        unsigned short fontScale;

        // Lineheight (depending on fontsize used)
        int lineHeight;

        // Offset in displaying entries
        int offset;

        // Test if file is an image
        bool IsImage (std::string imageName);
};
