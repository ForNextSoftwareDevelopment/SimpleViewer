#include "Main.h"

int main(int argc, char* argv[])
{
    // Name for window
    std::string current = "SimpleViewer: ";

    // New folder for filelist fill
    std::string newFolder;

    // Filelist enter/filled return value
    bool fileListFill = true;

    // Message for error/info
    std::string message;

    // Timer for double clicks
    std::chrono::high_resolution_clock::time_point t1 = std::chrono:: high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point t2 = std::chrono:: high_resolution_clock::now();
    std::chrono::duration<double> timeSpan;

    /*
    // Get the application folder
    char result[255];
    for (int i=0; i<255; i++) result[i]=0;
    ssize_t rCount = readlink( "/proc/self/exe", result, 255);
    if (rCount)
    {
        Error::SetLogFilePath(result);
    } else
    {
        Error::SetLogFilePath(argv[0]);
    }
    */

    // Get the home folder
    const char *homeDir = getenv("HOME");
    if (homeDir == NULL)
    {
        homeDir = getpwuid(getuid())->pw_dir;
    }

    std::string strHome;
    if (homeDir == NULL)
    {
        strHome == "/home";
        Error::WriteLog("ERROR", "Main", "Can't read home folder, using '/home'");
    } else
    {
        strHome.append(homeDir);
        strHome.append("/.simpleviewer");
        if (opendir(strHome.c_str()) == NULL)
        {
            int err = mkdir(strHome.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (err == -1)
            {
                Error::WriteLog("ERROR", "Main", "Can't create subfolder in home folder, using '/home'");
                strHome == "/home";
            }
        }
    }

	// Create new logfile (overwrite old one)
    Error::SetLogFilePath(strHome.c_str());
	Error::CreateLog();

    // Read settings
    int prevWidth = PREVWIDTH;
    int prevHeight = PREVHEIGHT;
    int fontScaleFi = 160;
    int fontScaleFo = 160;
    std::string startFolder = "/";
    std::string toBeSelected = "";
    std::string iniFile = Error::GetLogFilePath();
    iniFile.append("/simpleviewer.ini");
    char *pSettings = Files::ReadFile(iniFile);

    // No ini file, so copy from /usr/share/simpleviewer/ to home folder (together with simpleviewer.jpg)
    if (pSettings == NULL)
    {
        Files::CopyFile("/usr/share/simpleviewer/simpleviewer.ini", iniFile);
        std::string jpgFile = Error::GetLogFilePath();
        jpgFile.append("/simpleviewer.jpg");
        Files::CopyFile("/usr/share/simpleviewer/simpleviewer.jpg", jpgFile);
    }

    pSettings = Files::ReadFile(iniFile);
    if (pSettings != NULL)
    {
        try
        {
            std::string settings;
            settings.append(pSettings);

            std::vector<std::string> lines;
            std::string::size_type pos = 0;
            std::string::size_type prev = 0;
            while ((pos = settings.find("\n", prev)) != std::string::npos)
            {
                lines.push_back(settings.substr(prev, pos - prev));
                prev = pos + 1;
            }

            for (int i=0; i<(int)lines.size(); i++)
            {
                std::vector<std::string> words;
                std::string::size_type pos = 0;
                std::string::size_type prev = 0;
                while ((pos = lines[i].find(" ", prev)) != std::string::npos)
                {
                    words.push_back(lines[i].substr(prev, pos - prev));
                    prev = pos + 1;
                }
                words.push_back(lines[i].substr(prev, pos - prev));

                if (words[0] == "preview")
                {
                    if (words.size() > 2)
                    {
                        prevWidth = Utils::StrToInt((char*)words[1].c_str());
                        prevHeight = Utils::StrToInt((char*)words[2].c_str());
                    } else
                    {
                        Error::WriteLog("ERROR", "Main", "Can't read preview preference");
                    }
                }

                if (words[0] == "fontscale")
                {
                    if (words.size() > 2)
                    {
                        fontScaleFo = Utils::StrToInt((char*)words[1].c_str());
                        fontScaleFi = Utils::StrToInt((char*)words[2].c_str());
                    } else
                    {
                        Error::WriteLog("ERROR", "Main", "Can't read fontscale preference");
                    }
                }

                if (words[0] == "startfolder")
                {
                    if (words.size() > 1)
                    {
                        startFolder.append(words[1].c_str());
                    } else
                    {
                        Error::WriteLog("ERROR", "Main", "Can't read startfolder preference");
                    }
                }
            }
        } catch (...)
        {
            Error::WriteLog("ERROR", "Main", "Can't read (all) preferences");
        }

        delete pSettings;
    } else
    {
        Error::WriteLog("ERROR", "Main", "Can't read preferences file");
    }

    // Check for picturename in arguments
    std::string startPic = "";
    if (argc > 1)
    {
        startPic = argv[1];

        // Adjust startfolder
        std::string temp = "";
        temp.append(startPic.c_str());
        char *pStr = (char *) temp.c_str();
        char *pStrStart = pStr;
        pStr += temp.length() - 1;

        while ((pStr != pStrStart) && (*pStr != '/')) pStr--;
        *pStr=0x00;

        startFolder = "";
        startFolder.append(pStrStart);

        // Set toBeSelected entry
        toBeSelected = "";
        toBeSelected.append(pStr+1);
    }

    pDisplay = XOpenDisplay(NULL);
    if (pDisplay == NULL)
    {
        Error::WriteLog("ERROR", "Main", "Can't open display");
        exit(1);
    }

    // get default screen
    int screen = DefaultScreen(pDisplay);

    // Define used colors
    Colormap colormap = DefaultColormap(pDisplay, screen);

    XColor white;
    XAllocNamedColor(pDisplay, colormap, "white", &white, &white);

    XColor blue;
    XAllocNamedColor(pDisplay, colormap, "blue", &blue, &blue);

    XColor red;
    XAllocNamedColor(pDisplay, colormap, "red", &red, &red);

    XColor grey;
    XAllocNamedColor(pDisplay, colormap, "grey", &grey, &grey);

    XColor black;
    XAllocNamedColor(pDisplay, colormap, "black", &black, &black);

    // Initialize window attributes
    XSetWindowAttributes attrs;

    // Set event masks
    attrs.event_mask = StructureNotifyMask | ExposureMask | PropertyChangeMask;

    // Do not hide events from child window
    attrs.do_not_propagate_mask = 0;

    // Background color
    attrs.background_pixel = white.pixel;

    unsigned long attrs_mask =  CWEventMask | CWBackPixel;

    // Create and map container window
    Screen *pScreen = ScreenOfDisplay(pDisplay, 0);
    curScreenWidth = pScreen->width;
    curScreenHeight = pScreen->height;
    mainWindow = XCreateWindow(pDisplay, RootWindow(pDisplay, screen), 0, 0, curScreenWidth, curScreenHeight, 0, CopyFromParent, InputOutput, CopyFromParent, attrs_mask, &attrs);

    // Get WM_DELETE_WINDOW atom
    Atom wm_delete = XInternAtom(pDisplay, "WM_DELETE_WINDOW", True);

    // Subscribe WM_DELETE_WINDOW message
    XSetWMProtocols(pDisplay, mainWindow, &wm_delete, 1);

    // Create fifolist control (for folders only to the left side)
    FiFoList *pFolderList = new FiFoList(0, 0, prevWidth, pScreen->height - prevHeight - VOFFSET, fontScaleFo, startFolder);
    pFolderList->showFolders = true;
    pFolderList->showFiles = false;
    pFolderList->CreateWindow(pDisplay, mainWindow);

    // Create fifolist control (for files and folders to the right side
    FiFoList *pFileList = new FiFoList(prevWidth + 6, 0, pScreen->width - prevWidth - 6 - HOFFSET, pScreen->height - VOFFSET, fontScaleFi, startFolder);
    pFileList->showFolders = true;
    pFileList->showFiles = true;
    pFileList->CreateWindow(pDisplay, mainWindow);

    // If image provided in arguments, then adjust tobeselected in filelist control
    if (argc > 1)
    {
        pFileList->toBeSelected = toBeSelected;
        pFileList->Fill(startFolder);
    }

    // Create preview drawing control
    if (startPic == "")
    {
        startPic = Error::GetLogFilePath();
        startPic.append("/simpleviewer.jpg");
    }
    Drawing *pPrevDrawing = new Drawing(HOFFSET-4, pScreen->height - prevHeight + VOFFSET-4, prevWidth, prevHeight - VOFFSET);
    pPrevDrawing->CreateWindow(pDisplay, mainWindow);
    pPrevDrawing->LoadImage(startPic.c_str());

    // Change icon in taskbar
    unsigned int iconWidth = 48;
    unsigned int iconHeight = 48;
    uint32_t *pImageData = Utils::LoadImage(startPic, &iconWidth, &iconHeight, true, true);
    char *ptr = (char*) pImageData;

    if (pImageData != NULL)
    {
        uint64_t *pIconData = new uint64_t [2 + (iconWidth * iconHeight)];
        pIconData[0] = iconWidth;
        pIconData[1] = iconHeight;
        for (int i = 0; i < (iconWidth * iconHeight); i++)
        {
            uint64_t px;
            px = 0xFF000000 | ((ptr[i * 4 + 2]) << 16) | (ptr[i * 4 + 1] << 8) | (ptr[i * 4]);
            pIconData[i + 2] = px;
        }

        XChangeProperty(pDisplay, mainWindow, XInternAtom(pDisplay, "_NET_WM_ICON", FALSE), XA_CARDINAL, 32, PropModeReplace, (unsigned char*) pIconData, 2 + (iconWidth * iconHeight));
        free(pImageData);
        free(pIconData);
    } else
    {
        Error::WriteLog("ERROR", "Main", "Can't read icon file (icon.jpg)");
    }

    // Show current folder to be displayed
    current = "SimpleViewer: ";
    current.append(pFileList->currentFolder);
    XStoreName(pDisplay, mainWindow, current.c_str());

    // Create full drawing window
    Drawing *pDrawing = new Drawing(0, 0, pScreen->width, pScreen->height);
    pDrawing->CreateWindow(pDisplay, mainWindow);
    pDrawing->Detach();

    // Map main window
    XMapWindow(pDisplay, mainWindow);

    // If image provided in arguments, then show full-size
    if (argc > 1)
    {
        pDrawing->Attach();
        pDrawing->LoadImage(startPic.c_str());
        FullScreen();
        pDrawing->Paint();
    }

    // Window event loop
    bool exit = false;
    while (!exit)
    {
        XEvent event;
        XNextEvent(pDisplay, &event);
        std::string img;
        std::string iFile;
        std::string selectedFolder;
        bool scrollBarSelected;

        switch (event.type)
        {
            case Expose:
                #ifdef EVENTDEBUG
                    message = "Window Expose Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG

                if ((event.xexpose.window == mainWindow) && pFolderList && pFolderList->window)
                {
                   pFolderList->Paint();
                }

                if ((event.xexpose.window == mainWindow) && pPrevDrawing && pPrevDrawing->window)
                {
                   pPrevDrawing->Paint();
                }

                if ((event.xexpose.window == mainWindow) && pFileList && pFileList->window)
                {
                   pFileList->Paint();
                }
                break;

            case ConfigureNotify:
                #ifdef EVENTDEBUG
                    message = "Window ConfigureNotify Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG

                // Set new window size and position
                curScreenX       = event.xconfigure.x;
                curScreenY       = event.xconfigure.y;
                curScreenWidth   = event.xconfigure.width;
                curScreenHeight  = event.xconfigure.height;

                if ((event.xexpose.window == mainWindow) && pFolderList && pFolderList->window)
                {
                   pFolderList->SetSize(0, event.xconfigure.height - prevHeight - VOFFSET);
                }

                if ((event.xexpose.window == mainWindow) && pPrevDrawing && pPrevDrawing->window)
                {
                   pPrevDrawing->SetPosition(HOFFSET-4, event.xconfigure.height - prevHeight + VOFFSET-4);
                }

                if ((event.xexpose.window == mainWindow) && pFileList && pFileList->window)
                {
                   pFileList->SetSize(event.xconfigure.width - prevWidth - 6 - HOFFSET, event.xconfigure.height - VOFFSET);
                }
                break;

            case UnmapNotify:
                #ifdef EVENTDEBUG
                    message = "Window UnmapNotify Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG

                if (event.xexpose.window == mainWindow) mapped = false;
                break;

            case MapNotify:
                #ifdef EVENTDEBUG
                    message = "Window MapNotify Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG

                if (event.xexpose.window == mainWindow) mapped = true;
                break;

            case ButtonPress:
                #ifdef EVENTDEBUG
                    message = "Window ButtonPress Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    message.append("Keycode  : ");
                    message.append(Utils::IntToStr(event.xbutton.button));
                    message.append("\r\n");
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG

                // Clear preview drawing
                pPrevDrawing->Clear();

                // Mouse button events in full drawing window
                if (pDrawing && pDrawing->window && (event.xexpose.window == pDrawing->window))
                {
                    switch (event.xbutton.button)
                    {
                        case 1:
                            // Check for double click
                            t2 = t1;
                            t1 = std::chrono:: high_resolution_clock::now();
                            timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2);
                            if (timeSpan.count() < 0.3)
                            {
                                pDrawing->Detach();
                                NormalScreen();
                                pFileList->Paint();
                                pFolderList->Paint();
                            }
                            break;

                        // Scroll up
                        case 4:
                            pFileList->ScrollUp(1);
                            break;

                        // Scroll down
                        case 5:
                            pFileList->ScrollDown(1);
                            break;
                    }

                    // Load image
                    iFile = pFileList->GetSelectedFile();
                    if (iFile != "")
                    {
                        img = pFileList->currentFolder;
                        img.append("/");
                        img.append(iFile);
                        pDrawing->LoadImage(img);
                    }
                }

                // Mouse button events in folderlist
                if (pFolderList && pFolderList->window && (event.xexpose.window == pFolderList->window))
                {
                    switch (event.xbutton.button)
                    {
                        // left click
                        case 1:
                            // Calculate selected file/folder
                            scrollBarSelected = pFolderList->ButtonPressed(event.xbutton.x, event.xbutton.y);

                            if (!scrollBarSelected)
                            {
                                // Check for double click
                                t2 = t1;
                                t1 = std::chrono:: high_resolution_clock::now();
                                timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2);
                                if (timeSpan.count() < 0.3)
                                {
                                    pFolderList->EnterSelectedFolder();
                                }
                            }
                            break;

                        // Right click
                        case 3:
                            pFolderList->EnterParentFolder();
                            break;

                        // Scroll up
                        case 4:
                            pFolderList->ScrollUp(1);
                            break;

                        // Scroll down
                        case 5:
                            pFolderList->ScrollDown(1);
                            break;
                    }

                    // Fill filelist with entries from the selected folder
                    selectedFolder = pFolderList->GetSelectedFolder();
                    if ((selectedFolder != "") && (selectedFolder != ".") && (selectedFolder != ".."))
                    {
                        newFolder = pFolderList->currentFolder;
                        if (newFolder[newFolder.length() - 1] != '/') newFolder += "/";
                        newFolder.append(selectedFolder);
                        fileListFill = pFileList->Fill(newFolder);

                        // Redraw filelist
                        pFileList->Paint();
                    }
                }

                // Mouse button events in filelist
                if (pFileList && pFileList->window && (event.xexpose.window == pFileList->window))
                {
                    switch (event.xbutton.button)
                    {
                        // Left click
                        case 1:
                            // Calculate selected file/folder
                            scrollBarSelected = pFileList->ButtonPressed(event.xbutton.x, event.xbutton.y);

                            if (!scrollBarSelected)
                            {
                                t2 = t1;
                                t1 = std::chrono:: high_resolution_clock::now();
                                timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2);
                                if (timeSpan.count() < 0.3)
                                {
                                    // Check if this is a folder
                                    if (pFileList->GetSelectedFolder() != "")
                                    {
                                        fileListFill = pFileList->EnterSelectedFolder();
                                    }

                                    // Load image
                                    iFile = pFileList->GetSelectedFile();
                                    if (iFile != "")
                                    {
                                        // Show full screen drawing
                                        FullScreen();

                                        img = pFileList->currentFolder;
                                        img.append("/");
                                        img.append(iFile);
                                        pDrawing->LoadImage(img);
                                        pDrawing->Attach();
                                        pDrawing->Paint();
                                    }
                                }
                            }
                            break;

                        // Right click
                        case 3:
                            pFileList->EnterParentFolder();
                            break;

                        // Scroll up
                        case 4:
                            pFileList->ScrollUp(1);
                            break;

                        // Scroll down
                        case 5:
                            pFileList->ScrollDown(1);
                            break;
                    }
                }

                // Show current folder to be displayed in fileList
                current = "SimpleViewer: ";
                if (!fileListFill)
                {
                    current.append("Can't read ");
                    current.append(newFolder.c_str());
                    current.append("=> Showing: ");
                }
                current.append(pFileList->currentFolder);
                XStoreName(pDisplay, mainWindow, current.c_str());

                // Load preview image
                iFile = pFileList->GetSelectedFile();
                if (iFile != "")
                {
                    img = pFileList->currentFolder;
                    img.append("/");
                    img.append(iFile);
                    pPrevDrawing->LoadImage(img);
                }
                break;

            case KeyPress:
                #ifdef EVENTDEBUG
                    message = "Window KeyPress Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    message.append("Keycode  : ");
                    message.append(Utils::IntToStr(event.xkey.keycode));
                    message.append("\r\n");
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG

                // Clear preview drawing
                pPrevDrawing->Clear();

                // For all windows
                switch (event.xkey.keycode)
                {
                        // Q
                        case 24:
                            Error::WriteLog("INFO", "Main", "EXIT");
                            exit = true;
                            break;

                        // X
                        case 53:
                            Error::WriteLog("INFO", "Main", "EXIT");
                            exit = true;
                            break;
                }

                // Keyboard events in full screen drawing
                if (pDrawing && pDrawing->window && (event.xexpose.window == pDrawing->window))
                {
                    switch (event.xkey.keycode)
                    {
                        // Escape
                        case 9:
                            pDrawing->Detach();
                            NormalScreen();
                            pFileList->Paint();
                            pFolderList->Paint();
                            break;

                        // Enter
                        case 36:
                            pDrawing->Detach();
                            NormalScreen();
                            pFileList->Paint();
                            pFolderList->Paint();
                            break;

                        // Scroll up
                        case 111:
                            pFileList->ScrollUp(1);
                            break;

                        // Scroll down
                        case 116:
                            pFileList->ScrollDown(1);
                            break;

                        // Page up
                        case 112:
                            pFileList->ScrollUp(1);
                            break;

                        // Page down
                        case 117:
                            pFileList->ScrollDown(1);
                            break;
                    }

                    // Load image
                    iFile = pFileList->GetSelectedFile();
                    if (iFile != "")
                    {
                        img = pFileList->currentFolder;
                        img.append("/");
                        img.append(iFile);
                        pDrawing->LoadImage(img);
                    }
                }

                // Keyboard events in folderlist
                if (pFolderList && pFolderList->window && (event.xexpose.window == pFolderList->window))
                {
                    switch (event.xkey.keycode)
                    {
                        // backspace
                        case 22:
                            pFolderList->EnterParentFolder();
                            break;

                        // Enter
                        case 36:
                            pFolderList->EnterSelectedFolder();
                            break;

                        // Scroll up
                        case 111:
                            pFolderList->ScrollUp(1);
                            break;

                        // Scroll down
                        case 116:
                            pFolderList->ScrollDown(1);
                            break;

                        // Page up
                        case 112:
                            pFolderList->ScrollUp(10);
                            break;

                        // Page down
                        case 117:
                            pFolderList->ScrollDown(10);
                            break;
                    }

                    // Fill filelist with entries from the selected folder
                    selectedFolder = pFolderList->GetSelectedFolder();
                    if ((selectedFolder != "") && (selectedFolder != ".") && (selectedFolder != ".."))
                    {
                        newFolder = pFolderList->currentFolder;
                        if (newFolder[newFolder.length() - 1] != '/') newFolder += "/";
                        newFolder.append(selectedFolder);
                        fileListFill = pFileList->Fill(newFolder);

                        // Redraw filelist
                        pFileList->Paint();
                    }
                }

                // Keyboard events in filelist
                if (pFileList && pFileList->window && (event.xexpose.window == pFileList->window))
                {
                    // Clear the preview drawing
                    pPrevDrawing->Clear();

                    switch (event.xkey.keycode)
                    {
                        // backspace
                        case 22:
                            pFileList->EnterParentFolder();
                            break;

                        // Enter
                        case 36:
                            // Check if this is a folder
                            if (pFileList->GetSelectedFolder() != "")
                            {
                                fileListFill = pFileList->EnterSelectedFolder();
                            }

                            // Check if this is an image file
                            iFile = pFileList->GetSelectedFile();
                            if (iFile != "")
                            {
                                // Show full screen drawing
                                FullScreen();

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(iFile);
                                pDrawing->LoadImage(img);
                                pDrawing->Attach();
                                pDrawing->Paint();
                            }
                            break;

                        // Scroll up
                        case 111:
                            pFileList->ScrollUp(1);
                            break;

                        // Scroll down
                        case 116:
                            pFileList->ScrollDown(1);
                            break;

                        // Page up
                        case 112:
                            pFileList->ScrollUp(10);
                            break;

                        // Page down
                        case 117:
                            pFileList->ScrollDown(10);
                            break;
                    }
                }

                // Show current folder to be displayed in fileList
                current = "SimpleViewer: ";
                if (!fileListFill)
                {
                    current.append("Can't read ");
                    current.append(newFolder.c_str());
                    current.append("=> Showing: ");
                }
                current.append(pFileList->currentFolder);
                XStoreName(pDisplay, mainWindow, current.c_str());

                // Load preview image
                iFile = pFileList->GetSelectedFile();
                if (iFile != "")
                {
                    img = pFileList->currentFolder;
                    img.append("/");
                    img.append(iFile);
                    pPrevDrawing->LoadImage(img);
                }

                break;

            case DestroyNotify:
                #ifdef EVENTDEBUG
                    message = "Window DestroyNotify Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG
                exit = true;
                break;

            case ClientMessage:
                #ifdef EVENTDEBUG
                    message = "Window ClientMessage Event: ";
                    if (event.xexpose.window == mainWindow) message.append("Main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("Unknown Window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG
                if ((Atom)event.xclient.data.l[0] == wm_delete)
                {
                    Error::WriteLog("INFO", "Main", "EXIT");
                    exit = true;
                }
                break;

            default:
                #ifdef EVENTDEBUG
                    message = "Unhandled Event: ";
                    message.append(event_names[event.type]);
                    message.append("; ");
                    if (event.xexpose.window == mainWindow) message.append("From main window\r\n"); else
                    if (event.xexpose.window == pFolderList->window) message.append("Folderlist window\r\n"); else
                    if (event.xexpose.window == pFileList->window) message.append("Filelist window\r\n"); else
                    if (event.xexpose.window == pPrevDrawing->window) message.append("Preview drawing window\r\n"); else
                    if (event.xexpose.window == pDrawing->window) message.append("Drawing window\r\n"); else
                    {
                        message.append("From unknown window\r\n");
                    }
                    Error::WriteLog("INFO", "Main", message.c_str());
                #endif // EVENTDEBUG
                break;
        }
    }

    // Free resources
	XDestroyWindow(pDisplay, mainWindow);

    // Close connection to X-server
    XCloseDisplay(pDisplay);

    return 0;
}

/*********************************************************************
* Full screen
*********************************************************************/
void FullScreen(void)
{

    oldScreenX      = curScreenX;
    oldScreenY      = curScreenY;
    oldScreenWidth  = curScreenWidth;
    oldScreenHeight = curScreenHeight;

    // Unmap window for enabling changing properties
    XUnmapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    for (int i=0; (i<10) && mapped; i++)
    {
        // Wait for unmapping (needed for some distributions)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        XEvent event;
        XNextEvent(pDisplay, &event);
        switch (event.type)
        {
            case UnmapNotify:
                mapped = false;
                break;

            case MapNotify:
                mapped = true;
                break;
        }
    }

    Atom atoms[] = {
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_FULLSCREEN", false),
                        None
                   };

    XChangeProperty(pDisplay, mainWindow, XInternAtom(pDisplay, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (const unsigned char*)atoms, 2);

    // Make window visible
    XMapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    for (int i=0; (i<10) && !mapped; i++)
    {
        // Wait for mapping (needed for some distributions)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        XEvent event;
        XNextEvent(pDisplay, &event);
        switch (event.type)
        {
            case UnmapNotify:
                mapped = false;
                break;

            case MapNotify:
                mapped = true;
                break;
        }
    }
 }

/*********************************************************************
* Maximized screen
*********************************************************************/
void MaximizedScreen(void)
{
    Screen *pScreen = ScreenOfDisplay(pDisplay, 0);

    // Unmap window for enabling changing properties
    XUnmapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    for (int i=0; (i<10) && mapped; i++)
    {
        // Wait for unmapping (needed for some distributions)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        XEvent event;
        XNextEvent(pDisplay, &event);
        switch (event.type)
        {
            case UnmapNotify:
                mapped = false;
                break;

            case MapNotify:
                mapped = true;
                break;
        }
    }

    Atom atoms[] = {
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", false),
                        None
                   };

    XChangeProperty(pDisplay, mainWindow, XInternAtom(pDisplay, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (const unsigned char*)atoms, 4);

    // Make window visible
    XMapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    for (int i=0; (i<10) && !mapped; i++)
    {
        // Wait for mapping (needed for some distributions)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        XEvent event;
        XNextEvent(pDisplay, &event);
        switch (event.type)
        {
            case UnmapNotify:
                mapped = false;
                break;

            case MapNotify:
                mapped = true;
                break;
        }
    }

    // Change to 0 position and max size
    XMoveResizeWindow(pDisplay, mainWindow, 0, 0, pScreen->width, pScreen->height);
}

/*********************************************************************
* Normal screen
*********************************************************************/
void NormalScreen(void)
{
    // Unmap window for enabling changing properties
    XUnmapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    for (int i=0; (i<10) && mapped; i++)
    {
        // Wait for unmapping (needed for some distributions)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        XEvent event;
        XNextEvent(pDisplay, &event);
        switch (event.type)
        {
            case UnmapNotify:
                mapped = false;
                break;

            case MapNotify:
                mapped = true;
                break;
        }
    }

    Atom atoms[] = {
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_MODAL", false),
                        None
                   };

    XChangeProperty(pDisplay, mainWindow, XInternAtom(pDisplay, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (const unsigned char*)atoms, 2);

    // Change to old position and size
    XMoveResizeWindow(pDisplay, mainWindow, oldScreenX, oldScreenY, oldScreenWidth, oldScreenHeight);

    // Make window visible
    XMapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    for (int i=0; (i<10) && !mapped; i++)
    {
        // Wait for mapping (needed for some distributions)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        XEvent event;
        XNextEvent(pDisplay, &event);
        switch (event.type)
        {
            case UnmapNotify:
                mapped = false;
                break;

            case MapNotify:
                mapped = true;
                break;
        }
    }
}

