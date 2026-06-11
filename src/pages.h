// Pages/Menus for LCD display

struct CursorCell {
    int col;
    int row;
};

struct Page {
    char top[17];
    char bottom[17];
    CursorCell allowedCells[4];   
    CursorCell skippedCells[2];   
    int allowedCellCount;
};

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
    { {5,1}, {6,1}, {8,1}, {9,1} }, 
    { {7,1}, {-1,-1} },             
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
Page statsPage     = { "Stats & Streaks ", " Total Hours:   ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page timeLeftPage  = { "Time Remaining: ", "   00:00:00     ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page emergencyPage = { "!! EMERGENCY !! ", " Alarm Triggered", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };
Page endScreenPage = { " Session Ended! ", " Box Unlocked   ", { {0,0}, {-1,-1}, {-1,-1}, {-1,-1} }, { {-1,-1}, {-1,-1} }, 1 };

// Menu Navigation Configuration
const char* menuOptions[] = {
    " > 1. Pomodoro  ",
    " > 2. Tomodachi ",
    " > 3. Stats     ",
    " > 4. Time Left "
};
const int TOTAL_MENU_ITEMS = 4;

enum StudyState {
    STATE_START,
    STATE_TIME_SELECT,
    STATE_LOCKING,
    STATE_MAIN_MENU,
    STATE_POMODORO,
    STATE_PET,
    STATE_STATS,
    STATE_TIME_LEFT,
    STATE_EMERGENCY,
    STATE_END_SCREEN
};