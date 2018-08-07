#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>

#include "Error.h"
#include "Files.h"
#include "FiFoList.h"
#include "Drawing.h"

#define PREVWIDTH   400
#define PREVHEIGHT  400
#define VOFFSET     6
#define HOFFSET     6

#undef EVENTDEBUG

static const char *event_names[] = {
   "",
   "",
   "KeyPress",
   "KeyRelease",
   "ButtonPress",
   "ButtonRelease",
   "MotionNotify",
   "EnterNotify",
   "LeaveNotify",
   "FocusIn",
   "FocusOut",
   "KeymapNotify",
   "Expose",
   "GraphicsExpose",
   "NoExpose",
   "VisibilityNotify",
   "CreateNotify",
   "DestroyNotify",
   "UnmapNotify",
   "MapNotify",
   "MapRequest",
   "ReparentNotify",
   "ConfigureNotify",
   "ConfigureRequest",
   "GravityNotify",
   "ResizeRequest",
   "CirculateNotify",
   "CirculateRequest",
   "PropertyNotify",
   "SelectionClear",
   "SelectionRequest",
   "SelectionNotify",
   "ColormapNotify",
   "ClientMessage",
   "MappingNotify"
};

// main display and window
Display* pDisplay;
Window mainWindow;

void FullScreen(void);
void MaximizedScreen(void);

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

	// Create new logfile (overwrite old one)
	Error::CreateLog();

    // Read settings
    int prevWidth = PREVWIDTH;
    int prevHeight = PREVHEIGHT;
    int fontScaleFi = 160;
    int fontScaleFo = 160;
    std::string iniFile = Error::GetLogFilePath();
    iniFile.append(".ini");
    char *pSettings = Files::ReadFile(iniFile);
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
            }
        } catch (...)
        {
            Error::WriteLog("ERROR", "Main", "Can't read (all) preferences");
        }

        delete pSettings;
    }

    pDisplay = XOpenDisplay(NULL);
    if (pDisplay == NULL)
    {
        Error::WriteLog("ERROR", "Main", "Can't open display");
        exit(1);
    }

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

    attrs.event_mask
        = SubstructureRedirectMask // handle child window requests
        | StructureNotifyMask      // handle container notifications
        | ExposureMask             // handle container redraw
        | PropertyChangeMask;      // Handle changed properties

    // Do not hide any events from child window
    attrs.do_not_propagate_mask = 0;

    // Background color
    attrs.background_pixel = white.pixel;

    unsigned long attrs_mask = CWEventMask | CWBackPixel;

    // Create and map container window
    Screen *pScreen = ScreenOfDisplay(pDisplay, 0);
    mainWindow = XCreateWindow(pDisplay, RootWindow(pDisplay, screen), 0, 0, 800, 600, 0, CopyFromParent, InputOutput, CopyFromParent, attrs_mask, &attrs);

    // Maximize screen
    MaximizedScreen();

    // Make window visible
    XMapWindow(pDisplay, mainWindow);

    // Get WM_DELETE_WINDOW atom
    Atom wm_delete = XInternAtom(pDisplay, "WM_DELETE_WINDOW", True);

    // Subscribe WM_DELETE_WINDOW message
    XSetWMProtocols(pDisplay, mainWindow, &wm_delete, 1);

    // Create fifolist control (for folders only to the left side)
    FiFoList *pFolderList = new FiFoList(0, 0, prevWidth, pScreen->height - prevHeight - VOFFSET, fontScaleFo, "/home/xubuntu/Pictures/");
    pFolderList->showFolders = true;
    pFolderList->showFiles = false;
    pFolderList->CreateWindow(pDisplay, mainWindow);

    // Create preview drawing control
    std::string pic = Error::GetLogFilePath();
    pic.append(".jpg");
    Drawing *pPrevDrawing = new Drawing(0, pScreen->height - prevHeight, prevWidth, prevHeight);
    pPrevDrawing->CreateWindow(pDisplay, mainWindow);
    pPrevDrawing->LoadImage(pic.c_str());

    // Create fifolist control (for files and folders to the right side
    FiFoList *pFileList = new FiFoList(prevWidth + 6, 0, pScreen->width - prevWidth - 6 - HOFFSET, pScreen->height - VOFFSET, fontScaleFi, "/home/xubuntu/Pictures/");
    pFileList->showFolders = true;
    pFileList->showFiles = true;
    pFileList->CreateWindow(pDisplay, mainWindow);

    // Show current folder to be displayed in fileList
    current = "SimpleViewer: ";
    current.append(pFileList->currentFolder);
    XStoreName(pDisplay, mainWindow, current.c_str());

    // Create full drawing window, but do not attach it
    Drawing *pDrawing = new Drawing(0, 0, pScreen->width, pScreen->height);
    pDrawing->CreateWindow(pDisplay, mainWindow);
    pDrawing->Detach();

    // Window event loop
    bool exit = false;
    while (!exit)
    {
        XEvent event;
        XNextEvent(pDisplay, &event);
        std::string img;

        switch (event.type)
        {
            case CreateNotify:
                #ifdef EVENTDEBUG
                    message = "Window CreateNotify Event: ";
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
                break;

            case PropertyNotify:
                #ifdef EVENTDEBUG
                    message = "Window PropertyNotify Event: ";
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
                if ((event.xexpose.window == mainWindow) && pFolderList && pFolderList->window)
                {
                   pFolderList->SetSize(0, event.xconfigure.height - prevHeight - VOFFSET);
                   pFolderList->Paint();
                }

                if ((event.xexpose.window == mainWindow) && pPrevDrawing && pPrevDrawing->window)
                {
                   pPrevDrawing->SetPosition(0, event.xconfigure.height - prevHeight - VOFFSET);
                   pPrevDrawing->Paint();
                }

                if ((event.xexpose.window == mainWindow) && pFileList && pFileList->window)
                {
                   pFileList->SetSize(event.xconfigure.width - prevWidth - 6 - HOFFSET, event.xconfigure.height - VOFFSET);
                   pFileList->Paint();
                }

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

                                MaximizedScreen();

                                pFileList->Paint();
                                pFolderList->Paint();

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pPrevDrawing->LoadImage(img);
                                pPrevDrawing->Paint();
                            }
                            break;

                        // Scroll up
                        case 4:
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                if (pFileList->selectedFile != pFileList->fiList.begin())
                                {
                                    pFileList->selectedFile--;
                                    img = pFileList->currentFolder;
                                    img.append("/");
                                    img.append(*pFileList->selectedFile);
                                    pDrawing->LoadImage(img);
                                }
                            }
                            break;

                        // Scroll down
                        case 5:
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                pFileList->selectedFile++;
                                if (pFileList->selectedFile == pFileList->fiList.end())
                                {
                                    pFileList->selectedFile--;
                                }

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pDrawing->LoadImage(img);
                            }
                            break;
                    }
                }

                // Mouse button events in folderlist
                if (pFolderList && pFolderList->window && (event.xexpose.window == pFolderList->window))
                {
                    switch (event.xbutton.button)
                    {
                        case 1:
                            pFolderList->ButtonPressed(event.xbutton.x, event.xbutton.y);

                            // Check for double click
                            t2 = t1;
                            t1 = std::chrono:: high_resolution_clock::now();
                            timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2);
                            if (timeSpan.count() < 0.3)
                            {
                                pFolderList->EnterSelectedFolder();
                            }
                            break;

                        // Scroll up
                        case 4:
                            if (pFolderList->selectedFolder != pFolderList->foList.begin())
                            {
                                pFolderList->selectedFolder--;
                                pFolderList->Paint();
                            }
                            break;

                        // Scroll down
                        case 5:
                            pFolderList->selectedFolder++;
                            if (pFolderList->selectedFolder == pFolderList->foList.end()) pFolderList->selectedFolder--;
                            pFolderList->Paint();
                            break;
                    }

                    if ((*pFolderList->selectedFolder != ".") && (*pFolderList->selectedFolder != ".."))
                    {
                        newFolder = pFolderList->currentFolder;
                        newFolder.append("/");
                        newFolder.append(*pFolderList->selectedFolder);
                        fileListFill = pFileList->Fill(newFolder);
                        pPrevDrawing->Clear();
                    }
                }

                // Mouse button events in filelist
                if (pFileList && pFileList->window && (event.xexpose.window == pFileList->window))
                {
                    pPrevDrawing->Clear();

                    switch (event.xbutton.button)
                    {
                        case 1:
                            pFileList->ButtonPressed(event.xbutton.x, event.xbutton.y);
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pPrevDrawing->LoadImage(img);
                            }

                            t2 = t1;
                            t1 = std::chrono:: high_resolution_clock::now();
                            timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t2);
                            if (timeSpan.count() < 0.3)
                            {
                                // Check if this is a folder
                                if (pFileList->selectedFolder != pFileList->foList.end())
                                {
                                    fileListFill = pFileList->EnterSelectedFolder();
                                }

                                // Check if this is an image file
                                if (pFileList->selectedFile != pFileList->fiList.end())
                                {
                                    // Show full screen drawing
                                    FullScreen();

                                    // Load image
                                    img = pFileList->currentFolder;
                                    img.append("/");
                                    img.append(*pFileList->selectedFile);
                                    pDrawing->LoadImage(img.c_str());
                                    pDrawing->Attach();
                                    pDrawing->Paint();
                                }
                            }
                            break;

                        // Scroll up
                        case 4:
                            if (pFileList->selectedFolder != pFileList->foList.end())
                            {
                                // Scroll in folderlist
                                if (pFileList->selectedFolder != pFileList->foList.begin())
                                {
                                    pFileList->selectedFolder--;
                                    pFileList->Paint();
                                }
                            } else
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                if (pFileList->selectedFile != pFileList->fiList.begin())
                                {
                                    pFileList->selectedFile--;
                                    pFileList->Paint();

                                    img = pFileList->currentFolder;
                                    img.append("/");
                                    img.append(*pFileList->selectedFile);
                                    pPrevDrawing->LoadImage(img);
                                } else
                                {
                                    // Goto folderlist
                                    pFileList->selectedFile = pFileList->fiList.end();
                                    pFileList->selectedFolder = pFileList->foList.end();
                                    pFileList->selectedFolder--;
                                    pFileList->Paint();
                                }
                            }
                            break;

                        // Scroll down
                        case 5:
                            if (pFileList->selectedFolder != pFileList->foList.end())
                            {
                                // Scroll in folderlist
                                pFileList->selectedFolder++;
                                if (pFileList->selectedFolder != pFileList->foList.end())
                                {
                                    pFileList->Paint();
                                } else
                                {
                                    if (pFileList->fiList.size() > 0)
                                    {
                                        pFileList->selectedFile = pFileList->fiList.begin();
                                        pFileList->Paint();
                                    } else
                                    {
                                        pFileList->selectedFolder--;
                                        pFileList->Paint();
                                    }
                                }
                            } else
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                pFileList->selectedFile++;
                                if (pFileList->selectedFile == pFileList->fiList.end())
                                {
                                    pFileList->selectedFile--;
                                }
                                pFileList->Paint();

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pPrevDrawing->LoadImage(img);
                            }
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

                // For all windows
                switch (event.xkey.keycode)
                {
                        // Escape
                        case 9:
                            Error::WriteLog("INFO", "Main", "EXIT");
                            exit = true;
                            break;

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
                        // Enter
                        case 36:
                            pDrawing->Detach();
                            pFileList->Paint();
                            pFolderList->Paint();

                            img = pFileList->currentFolder;
                            img.append("/");
                            img.append(*pFileList->selectedFile);
                            pPrevDrawing->LoadImage(img);
                            pPrevDrawing->Paint();
                            break;

                        // Scroll up
                        case 111:
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                if (pFileList->selectedFile != pFileList->fiList.begin())
                                {
                                    pFileList->selectedFile--;
                                    img = pFileList->currentFolder;
                                    img.append("/");
                                    img.append(*pFileList->selectedFile);
                                    pDrawing->LoadImage(img);
                                }
                            }
                            break;

                        // Scroll down
                        case 116:
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                pFileList->selectedFile++;
                                if (pFileList->selectedFile == pFileList->fiList.end())
                                {
                                    pFileList->selectedFile--;
                                }

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pDrawing->LoadImage(img);
                            }
                            break;
                    }
                }

                // Keyboard events in folderlist
                if (pFolderList && pFolderList->window && (event.xexpose.window == pFolderList->window))
                {
                    switch (event.xkey.keycode)
                    {
                        // Enter
                        case 36:
                            pFolderList->EnterSelectedFolder();
                            break;

                        // Scroll up
                        case 111:
                            if (pFolderList->selectedFolder != pFolderList->foList.begin())
                            {
                                pFolderList->selectedFolder--;
                                pFolderList->Paint();
                            }
                            break;

                        // Scroll down
                        case 116:
                            pFolderList->selectedFolder++;
                            if (pFolderList->selectedFolder == pFolderList->foList.end()) pFolderList->selectedFolder--;
                            pFolderList->Paint();
                            break;
                    }

                    newFolder = pFolderList->currentFolder;
                    newFolder.append("/");
                    newFolder.append(*pFolderList->selectedFolder);
                    fileListFill = pFileList->Fill(newFolder);
                }

                // Keyboard events in filelist
                if (pFileList && pFileList->window && (event.xexpose.window == pFileList->window))
                {
                    std::string img;
                    switch (event.xkey.keycode)
                    {
                        // Enter
                        case 36:
                            // Check if this is a folder
                            if (pFileList->selectedFolder != pFileList->foList.end())
                            {
                                fileListFill = pFileList->EnterSelectedFolder();
                            }

                            // Check if this is an image file
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Show full screen drawing
                                FullScreen();

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pDrawing->LoadImage(img.c_str());
                                pDrawing->Attach();
                                pDrawing->Paint();
                            }
                            break;

                        // Scroll up
                        case 111:
                            if (pFileList->selectedFolder != pFileList->foList.end())
                            {
                                // Scroll in folderlist
                                if (pFileList->selectedFolder != pFileList->foList.begin())
                                {
                                    pFileList->selectedFolder--;
                                    pFileList->Paint();
                                }
                            } else
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                if (pFileList->selectedFile != pFileList->fiList.begin())
                                {
                                    pFileList->selectedFile--;
                                    pFileList->Paint();
                                    img = pFileList->currentFolder;
                                    img.append("/");
                                    img.append(*pFileList->selectedFile);
                                    pPrevDrawing->LoadImage(img);
                                } else
                                {
                                    // Goto folderlist
                                    pFileList->selectedFile = pFileList->fiList.end();
                                    pFileList->selectedFolder = pFileList->foList.end();
                                    pFileList->selectedFolder--;
                                    pFileList->Paint();
                                }
                            }
                            break;

                        // Scroll down
                        case 116:
                            if (pFileList->selectedFolder != pFileList->foList.end())
                            {
                                // Scroll in folderlist
                                pFileList->selectedFolder++;
                                if (pFileList->selectedFolder != pFileList->foList.end())
                                {
                                    pFileList->Paint();
                                } else
                                {
                                    if (pFileList->fiList.size() > 0)
                                    {
                                        pFileList->selectedFile = pFileList->fiList.begin();
                                        pFileList->Paint();
                                    } else
                                    {
                                        pFileList->selectedFolder--;
                                        pFileList->Paint();
                                    }
                                }
                            } else
                            if (pFileList->selectedFile != pFileList->fiList.end())
                            {
                                // Scroll in filelist
                                pFileList->selectedFile++;
                                if (pFileList->selectedFile == pFileList->fiList.end())
                                {
                                    pFileList->selectedFile--;
                                }
                                pFileList->Paint();

                                img = pFileList->currentFolder;
                                img.append("/");
                                img.append(*pFileList->selectedFile);
                                pPrevDrawing->LoadImage(img);
                            }
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
                break;

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
    // Unmap window for enabling changing properties
    XUnmapWindow(pDisplay, mainWindow);
    XSync(pDisplay, false);

    Atom atoms[3] = {
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_FULLSCREEN", false),
                        None
                    };

    XChangeProperty(pDisplay, mainWindow, XInternAtom(pDisplay, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (const unsigned char*)atoms, 2);

    // Make window visible
    XMapWindow(pDisplay, mainWindow);
    XFlush(pDisplay);
 }

/*********************************************************************
* Maximized screen
*********************************************************************/
void MaximizedScreen(void)
{
    // Unmap window for enabling changing properties
    XUnmapWindow(pDisplay, mainWindow);
    XMoveResizeWindow(pDisplay, mainWindow, 0, 0, 800, 600);
    XSync(pDisplay, false);

    Atom atoms[5] = {
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_ADD", false),
                        XInternAtom(pDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", false),
                        None
                    };

    XChangeProperty(pDisplay, mainWindow, XInternAtom(pDisplay, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, (const unsigned char*)atoms, 4);

    // Make window visible
    XMapWindow(pDisplay, mainWindow);
    XFlush(pDisplay);
}

