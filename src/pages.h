//Pages/Menus for LCD display
//Pages: Set Time Page, Timer Page, 

struct CursorCell {
    int col;
    int row;
};

struct Page {
    char top[16];
    char bottom[16];
    CursorCell allowedCells[16]; // max 16 allowed positions
    CursorCell skippedCells[15];
    int allowedCellCount;
};

Page timeSelectPage = {
    "  Choose Time   ",
    "   < ##:## >    ",
    { {5,1}, {6,1}, {8,1}, {9,1} }, // only the ##:## digits
    { {7,1} },
    4 // number of allowed cells
};

Page startPage = {
    "    StudyBox  ",
    " Click to Start",
    { {0,0} }, // fully locked, cursor stays at 0,0
    1
};
Page timeStartedPage = {
    "  Timer started ",
    "                ",
    { {0,0} },
    1
};
Page pages[] = {startPage, timeSelectPage, timeStartedPage};