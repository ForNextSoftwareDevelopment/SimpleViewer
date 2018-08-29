#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <chrono>
#include <thread>

#include "Error.h"
#include "Files.h"
#include "FiFoList.h"
#include "Drawing.h"

#define PREVWIDTH   400
#define PREVHEIGHT  400
#define VOFFSET     6
#define HOFFSET     6

#define EVENTDEBUG

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
bool mapped;
int curScreenX, curScreenY;
unsigned int curScreenWidth, curScreenHeight;
int oldScreenX, oldScreenY;
unsigned int oldScreenWidth, oldScreenHeight;

void FullScreen(void);
void MaximizedScreen(void);
void NormalScreen(void);
