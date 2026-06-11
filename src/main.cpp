/*
  StudyBox - Arduino Study Timer with Pomodoro, Pet Companion, and Statistics
  
  This project implements an interactive study timer with:
  - Joystick-based time selection and menu navigation
  - Pomodoro technique with 25-min focus and 5-min break cycles
  - Virtual pet that gains/loses happiness based on activity
  - Session statistics tracking
  - Physical servo lock mechanism
  - Emergency override button for quick exit
*/

#include <Arduino.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <pages.h>
#include <customChars.h>
#include <Servo.h>

// ----- Hardware Pin Configuration -----
const int X_PIN = A0;                 // Joystick X-axis (analog)
const int Y_PIN = A1;                 // Joystick Y-axis (analog)
const int SW_PIN = 7;                 // Joystick switch button
const int EMERGENCY_PIN = 8;          // Emergency override panic button

// const int buzzerPin = 6; buzzer abandoned i think
const int servoPin = 10;              // Servo motor for lock mechanism

// ----- Display and Hardware Objects -----
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // LCD display (16x2 chars)
Servo servo;                            // Servo for box lock/unlock

// ----- Function Declarations -----
// UI/Display functions
void printPage(Page page);
void setCursorPosition();
void setCurrentPage(Page page);
void blinkCursor();
void refreshPage();
void moveCursor(int colDir, int rowDir);

// Timer/Countdown functions
void runActiveCountdown();
void updateTimeDisplay();
void updateCountdownDisplay();

// Feature update functions
void updatePomodoroLogic();
void updatePetLogic();
void updateStatsLogic();

// State handler functions
void stateStart(boolean swPressed);
void stateTimeSelect(boolean swPressed, int xValue, int yValue);
void stateLocking();
void stateMainMenu(boolean swPressed, int xValue, boolean emergencyPressed);
void stateActiveApp(boolean swPressed, int xValue, int yValue, boolean emergencyPressed);
void stateTimeLeftView(boolean swPressed, boolean emergencyPressed);
void stateEmergency();
void stateEndScreen(boolean swPressed);

// ----- Global State Variables -----
StudyState currentState = STATE_START;  // Current state in the state machine
Page currentPage;                       // Current LCD display page

// ----- UI Variables -----
int cursorPosition[2] = {0,0};          // Cursor position [column, row]
int currentMenuSelection = 1;           // Current menu item selected (1-indexed)

// ----- Study Timer Variables -----
int selectedHours = 0;                  // User-selected study hours
int selectedMinutes = 25;               // User-selected study minutes (default: 25)
unsigned long studyTimerStart = 0;      // Timestamp when study session started
unsigned long studyDurationMs = 0;      // Total study duration in milliseconds

// ----- Timing Variables -----
unsigned long currentMillis;            // Current millisecond count (updated each loop)
unsigned long previousBlinkMillis = 0UL;// Last cursor blink timing
unsigned long blinkInterval = 400UL;    // Cursor blink interval (ms)
unsigned long previousMillis = 0UL;     // Last debounce/update check
unsigned long previousJoystickMillis = 0UL; // Last joystick input check
unsigned long interval = 300UL;         // Joystick input debounce interval (ms)
bool editingHours = true;               // Toggle between editing hours vs minutes

bool showCursor = true;                 // Cursor visibility state for blinking

//Pomodoro Feature variables
unsigned long pomoCycleStart = 0;       // Timestamp when current Pomodoro cycle started
unsigned long pomoDuration = 25 * 60 * 1000UL;  // Duration of current cycle (25 min focus or 5 min break)
bool isBreakMode = false;               // True if in break cycle, false if in focus cycle
int completedCycles = 0;                // Number of completed 25-min focus cycles

// Tamagotchi Pet Feature variables
int petHappiness = 100;                 // Pet happiness level (0-100%)
unsigned long lastDecayMillis = 0;      // Last timestamp when happiness decayed

// ----- Servo Lock Angles -----
const int servoAngleLock = 100;         // Servo angle to LOCK the box
const int servoAngleUnlock = 180;       // Servo angle to UNLOCK the box

// ----- SETUP: Initialize hardware and display -----
void setup() {
    // Initialize serial communication for debugging
    Serial.begin(9600);
    Serial.println("--- StudyBox Booting Up ---");

    // Initialize 16x2 LCD display
    lcd.begin(16,2);
    lcd.clear();
    
    // Initialize servo and set to unlocked position
    servo.attach(servoPin);
    servo.write(servoAngleUnlock); 

    // Initialize button pins with pull-up resistors
    pinMode(SW_PIN, INPUT_PULLUP);           // Joystick switch
    pinMode(EMERGENCY_PIN, INPUT_PULLUP);    // Emergency button
    // pinMode(buzzerPin, OUTPUT);              // Buzzer (disabled)
    
    // Display start page
    setCurrentPage(startPage);
    Serial.println("Setup Completed Successfully.");
}

// ----- MAIN LOOP: Read inputs and execute state machine -----
void loop() {
    // Update current time reference
    currentMillis = millis();
    
    // Read joystick analog values
    int xValue = analogRead(X_PIN);
    int yValue = analogRead(Y_PIN);
    
    // Read digital button states (LOW = pressed due to INPUT_PULLUP)
    boolean swPressed = (digitalRead(SW_PIN) == LOW);
    boolean emergencyPressed = (digitalRead(EMERGENCY_PIN) == LOW);

    // Execute current state handler
    switch (currentState) {
        case STATE_START:       stateStart(swPressed);                                             break;
        case STATE_TIME_SELECT: stateTimeSelect(swPressed, xValue, yValue);                        break;
        case STATE_LOCKING:     stateLocking();                                                    break;
        case STATE_MAIN_MENU:   stateMainMenu(swPressed, xValue, emergencyPressed);                break;
        case STATE_POMODORO:    
        case STATE_PET:         
        case STATE_STATS:       stateActiveApp(swPressed, xValue, yValue, emergencyPressed);       break;
        case STATE_TIME_LEFT:   stateTimeLeftView(swPressed, emergencyPressed);                    break;
        case STATE_EMERGENCY:   stateEmergency();                                                  break;
        case STATE_END_SCREEN:  stateEndScreen(swPressed);                                         break;
    }
}

// ----- STATE HANDLERS -----

// STATE: Start screen - wait for button press to enter time selection
void stateStart(boolean swPressed) {
    if (swPressed) {
        delay(250);  // Debounce delay
        currentState = STATE_TIME_SELECT;
        previousJoystickMillis = millis();
        setCurrentPage(timeSelectPage);
        updateTimeDisplay();
    }
}

// STATE: Time selection - use joystick to set hours and minutes
// Press button to toggle between hours and minutes editing
// When finished, enter STATE_LOCKING
void stateTimeSelect(boolean swPressed, int xValue, int yValue) {
    if (swPressed) {
        delay(250);
        if (editingHours) {
            // Switch from hours to minutes
            editingHours = false;
            cursorPosition[0] = 8;
            refreshPage();
        } 
        else {
            // Finished - calculate total duration and lock the box
            editingHours = true;
            studyDurationMs = ((selectedHours * 3600UL) + (selectedMinutes * 60UL)) * 1000UL;
            currentState = STATE_LOCKING;
            setCurrentPage(timeStartedPage);
            return;
        }
    }

    blinkCursor();
    
    // Debounce joystick input
    if (currentMillis - previousJoystickMillis > interval) {
        // Joystick left: decrease value
        if (xValue < 300) {
            if (editingHours && selectedHours > 0) selectedHours--;
            else if (!editingHours && selectedMinutes > 0) selectedMinutes--;
            updateTimeDisplay();
            previousJoystickMillis = currentMillis;
        }
        // Joystick right: increase value
        else if (xValue > 700) {
            if (editingHours && selectedHours < 99) selectedHours++;
            else if (!editingHours && selectedMinutes < 59) selectedMinutes++;
            updateTimeDisplay();
            previousJoystickMillis = currentMillis;
        }
    }
}

// STATE: Locking - physically lock the box and initialize timer
void stateLocking() {
    servo.write(servoAngleLock);  // Lock the box
    delay(1000);                   // Wait for servo to complete
    
    // Start all timers
    studyTimerStart = millis();
    pomoCycleStart = millis();
    lastDecayMillis = millis();
    
    // Transition to main menu
    strcpy(mainMenuPage.bottom, menuOptions[currentMenuSelection - 1]);
    currentState = STATE_MAIN_MENU;
    setCurrentPage(mainMenuPage);
}

// STATE: Main menu - navigate between Pomodoro, Pet, Stats, and Time Left views
// Emergency button triggers immediate exit
void stateMainMenu(boolean swPressed, int xValue, boolean emergencyPressed) {
    // Check for emergency override
    if (emergencyPressed) {
        currentState = STATE_EMERGENCY;
        setCurrentPage(emergencyPage);
        return;
    }

    // Debounce joystick navigation
    if (currentMillis - previousMillis > interval) {
        bool changed = false;

        // Joystick left: previous menu item
        if (xValue < 300) {
            if (currentMenuSelection > 1) {
                currentMenuSelection--;
                changed = true;
            }
            previousMillis = currentMillis;
        }
        // Joystick right: next menu item
        else if (xValue > 700) {
            if (currentMenuSelection < TOTAL_MENU_ITEMS) {
                currentMenuSelection++;
                changed = true;
            }
            previousMillis = currentMillis;
        }

        if (changed) {
            strcpy(mainMenuPage.bottom, menuOptions[currentMenuSelection - 1]);
            currentPage = mainMenuPage;
            refreshPage();
        }
    }

    // Select menu item
    if (swPressed) {
        delay(250);
        switch(currentMenuSelection) {
            case 1: currentState = STATE_POMODORO; setCurrentPage(pomodoroPage); break;
            case 2: currentState = STATE_PET;      setCurrentPage(petPage);      break;
            case 3: currentState = STATE_STATS;    setCurrentPage(statsPage);    break;
            case 4: currentState = STATE_TIME_LEFT; setCurrentPage(timeLeftPage); break;
        }
    }
}

// STATE: Active apps - handle Pomodoro, Pet, and Stats features during study session
// Emergency button triggers immediate exit
void stateActiveApp(boolean swPressed, int xValue, int yValue, boolean emergencyPressed) {
    // Check for emergency override
    if (emergencyPressed) {
        currentState = STATE_EMERGENCY;
        setCurrentPage(emergencyPage);
        return;
    }

    runActiveCountdown();  // Check if total study time is complete

    // Update the currently active feature
    if (currentState == STATE_POMODORO) {
        updatePomodoroLogic();
        // Joystick left/right: toggle between focus and break modes
        if (currentMillis - previousJoystickMillis > interval) {
            if (xValue < 300 || xValue > 700) {
                isBreakMode = !isBreakMode;
                pomoDuration = (isBreakMode) ? (5 * 60 * 1000UL) : (25 * 60 * 1000UL);
                pomoCycleStart = currentMillis;
                if(!isBreakMode) completedCycles++;
                previousJoystickMillis = currentMillis;
            }
        }
    } 
    else if (currentState == STATE_PET) {
        updatePetLogic();
        // Joystick right: give pet attention (increase happiness)
        if (currentMillis - previousJoystickMillis > interval) {
            if (xValue > 700) {
                petHappiness = min(100, petHappiness + 1);
                previousJoystickMillis = currentMillis;
            }
        }
    } 
    else if (currentState == STATE_STATS) {
        updateStatsLogic();
    }
    
    // Button press: return to main menu
    if (swPressed) {
        delay(250);
        currentState = STATE_MAIN_MENU;
        setCurrentPage(mainMenuPage);
    }
}

// STATE: Time left view - display remaining study time
// Emergency button triggers immediate exit
void stateTimeLeftView(boolean swPressed, boolean emergencyPressed) {
    // Check for emergency override
    if (emergencyPressed) {
        currentState = STATE_EMERGENCY;
        setCurrentPage(emergencyPage);
        return;
    }

    runActiveCountdown();  // Check if study time is complete
    
    if (currentState == STATE_TIME_LEFT) {
        updateCountdownDisplay();  // Update time display
    }

    // Button press: return to main menu
    if (swPressed) {
        delay(250);
        currentState = STATE_MAIN_MENU;
        setCurrentPage(mainMenuPage);
    }
}

// STATE: Emergency - unlock the box immediately (penalty: pet happiness to 0)
void stateEmergency() {
    // Unlock the box immediately
    servo.write(servoAngleUnlock);
    delay(500);
    
    // Move to end screen and reset state machine
    currentState = STATE_END_SCREEN;
    setCurrentPage(endScreenPage);

    // Penalty: reset pet happiness to 0
    petHappiness = 0;
}

// STATE: End screen - study session complete, unlock box and wait for button press
void stateEndScreen(boolean swPressed) {
    servo.write(servoAngleUnlock);  // Keep box unlocked
    if (swPressed) {
        delay(250);
        // Reset to start screen
        currentState = STATE_START;
        setCurrentPage(startPage);
    }
}

// ----- DISPLAY UPDATE FUNCTIONS -----

// Format and display the currently selected time (HH:MM)
void updateTimeDisplay() {
    char timeBuffer[17];
    sprintf(timeBuffer, "   < %02d:%02d >    ", selectedHours, selectedMinutes);
    strcpy(currentPage.bottom, timeBuffer);
    refreshPage();
}

// Display remaining study time countdown (HH:MM:SS)
void updateCountdownDisplay() {
    static unsigned long lastUpdateMillis = 0;
    if (currentMillis - lastUpdateMillis > 200) {
        // Calculate time remaining
        unsigned long elapsed = currentMillis - studyTimerStart;
        unsigned long remainingMs = (studyDurationMs > elapsed) ? (studyDurationMs - elapsed) : 0;
        
        // Convert milliseconds to hours, minutes, seconds
        unsigned long totalSecs = remainingMs / 1000UL;
        int hours = totalSecs / 3600UL;
        int mins = (totalSecs % 3600UL) / 60UL;
        int secs = totalSecs % 60UL;

        // Format and display
        char displayBuffer[17];
        sprintf(displayBuffer, "    %02d:%02d:%02d    ", hours, mins, secs);
        strcpy(currentPage.bottom, displayBuffer);
        refreshPage();
        lastUpdateMillis = currentMillis;
    }
}

// ----- FEATURE UPDATE FUNCTIONS -----

// Update Pomodoro timer: toggle between 25-min focus and 5-min break cycles
void updatePomodoroLogic() {
    static long lastPomoRender = 0;
    if (currentMillis - lastPomoRender > 250) {
        // Calculate elapsed time in current cycle
        long elapsedPomo = currentMillis - pomoCycleStart;
        
        // Check if current cycle is complete
        if (elapsedPomo >= pomoDuration) {
            // Auto-toggle between focus and break modes
            isBreakMode = !isBreakMode;
            pomoDuration = (isBreakMode) ? (5 * 60 * 1000UL) : (25 * 60 * 1000UL);
            pomoCycleStart = currentMillis;
            if (!isBreakMode) completedCycles++;  // Increment cycle counter when returning to focus mode
            return;
        }

        // Calculate and display remaining time in current cycle
        long remSecs = (pomoDuration - elapsedPomo) / 1000UL;
        int pMins = remSecs / 60;
        int pSecs = remSecs % 60;

        char topBuffer[17];
        char btmBuffer[17];
        const char* cycleMode = (isBreakMode ? "Break! " : "Focus! ");
        sprintf(topBuffer, "%s  Cyc:%02d", cycleMode, completedCycles);
        sprintf(btmBuffer, "Rem:   %02d:%02d  ", pMins, pSecs);
        
        strcpy(currentPage.top, topBuffer);
        strcpy(currentPage.bottom, btmBuffer);
        refreshPage();
        lastPomoRender = currentMillis;
    }
}

// Update virtual pet: decay happiness over time, display pet mood
void updatePetLogic() {
    // Decay pet happiness every 12 seconds (penalty for not interacting)
    if (currentMillis - lastDecayMillis > 12000UL) {
        if (petHappiness > 0) petHappiness -= 2;
        lastDecayMillis = currentMillis;
    }

    static unsigned long lastPetRender = 0;
    if (currentMillis - lastPetRender > 250) {
        char topBuffer[17];
        char btmBuffer[17];

        // Change pet expression based on happiness level
        char expression = 'o';      // Normal
        if (petHappiness < 30) expression = 'x';        // Sad
        else if (petHappiness > 75) expression = '^';   // Happy

        sprintf(topBuffer, "Pet:   ( %c_%c )  ", expression, expression);
        sprintf(btmBuffer, "Happiness: %d%% ", petHappiness);

        strcpy(currentPage.top, topBuffer);
        strcpy(currentPage.bottom, btmBuffer);
        refreshPage();
        lastPetRender = currentMillis;
    }
}

// Display session statistics: elapsed time and focus score
void updateStatsLogic() {
    static unsigned long lastStatsRender = 0;
    if (currentMillis - lastStatsRender > 1000) {
        // Calculate total elapsed time since session start
        unsigned long elapsedSecs = (currentMillis - studyTimerStart) / 1000UL;
        int activeHours = elapsedSecs / 3600UL;
        int activeMins = (elapsedSecs % 3600UL) / 60UL;

        char topBuffer[17];
        char btmBuffer[17];
        sprintf(topBuffer, "Session: %02dh:%02dm", activeHours, activeMins);
        
        // Calculate focus score based on completed Pomodoro cycles
        int efficiencyScore = min(100, (completedCycles * 35) + 40);
        sprintf(btmBuffer, "Focus Score:%d%%", efficiencyScore);

        strcpy(currentPage.top, topBuffer);
        strcpy(currentPage.bottom, btmBuffer);
        refreshPage();
        lastStatsRender = currentMillis;
    }
}

// ----- UTILITY FUNCTIONS -----

// Check if total study time has elapsed - transition to end screen
void runActiveCountdown() {
    unsigned long elapsed = millis() - studyTimerStart;
    if (elapsed >= studyDurationMs) {
        currentState = STATE_END_SCREEN;
        setCurrentPage(endScreenPage);
    }
}

// Move cursor to adjacent editable cell (skip disabled cells)
void moveCursor(int colDir, int rowDir) {
    int newCol = cursorPosition[0] + colDir;
    int newRow = cursorPosition[1] + rowDir;
    for (int i = 0; i < currentPage.allowedCellCount; i++) {
        if(currentPage.skippedCells[i].col == newCol) cursorPosition[0] = newCol + colDir;
        if (currentPage.allowedCells[i].col == newCol &&
            currentPage.allowedCells[i].row == newRow) {
            cursorPosition[0] = newCol;
            cursorPosition[1] = newRow;
            return;
        }
    }
}

// Position cursor on LCD and make it blink
void setCursorPosition(){
    lcd.setCursor(cursorPosition[0], cursorPosition[1]);
    blinkCursor();
}

// Refresh the LCD with current page content
void refreshPage(){
    printPage(currentPage);
}

// Load and display a new page with cursor at first allowed position
void setCurrentPage(Page page) {
    currentPage = page;
    cursorPosition[0] = page.allowedCells[0].col;
    cursorPosition[1] = page.allowedCells[0].row;
    printPage(currentPage);
}

// Blink the cursor at current position (toggle between _ and character)
void blinkCursor(){
    if (currentMillis - previousBlinkMillis > blinkInterval) {
        showCursor = !showCursor;
        previousBlinkMillis = currentMillis;
        
        lcd.setCursor(cursorPosition[0], cursorPosition[1]);
        if (showCursor) {
            lcd.print('_');  // Display cursor
        } else {
            // Get character under cursor
            char currentChar = cursorPosition[1] == 0 
                ? currentPage.top[cursorPosition[0]] 
                : currentPage.bottom[cursorPosition[0]];
            if (currentChar == '\0' || currentChar == '\255') currentChar = ' ';
            lcd.print(currentChar);  // Display character
        }
    }
}

// Print page content to LCD display
void printPage(Page page) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(page.top);
    lcd.setCursor(0, 1);
    lcd.print(page.bottom);
}