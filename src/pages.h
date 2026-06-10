//Pages/Menus for LCD display
//Pages: Set Time Page, Timer Page, 

// Ensure the struct matches this definition exactly:
struct CursorCell {
    int col;
    int row;
};

struct Page {
    char top[17];
    char bottom[17];
    CursorCell allowedCells[4];   // Max 4 interactive points
    CursorCell skippedCells[2];   // Max 2 skipped points
    int allowedCellCount;
};

// Explicitly populate every single array index to protect memory structures:
Page startPage = {
    "    StudyBox    ",
    " Click to Start ",
    { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, 
    { {-1,-1}, {-1,-1} },
    1
};

Page timeSelectPage = {
    "  Choose Time   ",
    "   < 00:25 >    ", 
    { {5,1}, {6,1}, {8,1}, {9,1} }, // Col 5 & 6 are Hours, 8 & 9 are Minutes
    { {7,1}, {-1,-1} },             // Skip the colon ':' at column 7
    4 
};

Page timeStartedPage = {
    "  Timer started ",
    "    Locking...  ",
    { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} },
    { {-1,-1}, {-1,-1} },
    1
};

Page mainMenuPage  = { "   Main Menu:   ", " > 1. Pomodoro  ", { {0,1}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page pomodoroPage  = { "Pomodoro Timer ", " Remaining:     ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page petPage       = { "Tomodachi Pet   ", " Status: Happy  ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page musicPage     = { "Music Playlist  ", " Track: Lo-Fi   ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page statsPage     = { "Stats & Streaks ", " Total Hours:   ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page emergencyPage = { "!! EMERGENCY !! ", " Alarm Triggered", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page endScreenPage = { " Session Ended! ", " Box Unlocked   ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };

// Menu Navigation Configuration
const char* menuOptions[] = {
    " > 1. Pomodoro  ",
    " > 2. Tomodachi ",
    " > 3. Music     ",
    " > 4. Stats     "
};
const int TOTAL_MENU_ITEMS = 4;

enum StudyState {
    STATE_START,
    STATE_TIME_SELECT,
    STATE_LOCKING,
    STATE_MAIN_MENU,
    STATE_POMODORO,
    STATE_PET,
    STATE_MUSIC,
    STATE_STATS,
    STATE_EMERGENCY,
    STATE_END_SCREEN
};

