#include <Arduino.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <pages.h>
#include <customChars.h>
#include <Servo.h>

const int X_PIN = A0; 
const int Y_PIN = A1; 
const int SW_PIN = 7; 
const int EMERGENCY_PIN = 8; // NEW: Dedicated hardware panic/emergency override button

// const int buzzerPin = 6; buzzer abdandoned i think
const int servoPin = 10;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 

Servo servo;

void printPage(Page page);
void setCursorPosition();
void setCurrentPage(Page page);
void blinkCursor();
void refreshPage();
void moveCursor(int colDir, int rowDir);
void runActiveCountdown();
void updateTimeDisplay();
void updateCountdownDisplay();

//features
void updatePomodoroLogic();
void updatePetLogic();
void updateStatsLogic();

void stateStart(boolean swPressed);
void stateTimeSelect(boolean swPressed, int xValue, int yValue);
void stateLocking();
void stateMainMenu(boolean swPressed, int xValue, boolean emergencyPressed);
void stateActiveApp(boolean swPressed, int xValue, int yValue, boolean emergencyPressed);
void stateTimeLeftView(boolean swPressed, boolean emergencyPressed);
void stateEmergency();
void stateEndScreen(boolean swPressed);

StudyState currentState = STATE_START;
Page currentPage;

int cursorPosition[2] = {0,0};
int currentMenuSelection = 1; 

int selectedHours = 0;       
int selectedMinutes = 25;
unsigned long studyTimerStart = 0;
unsigned long studyDurationMs = 0;

unsigned long currentMillis;
unsigned long previousBlinkMillis = 0UL;
unsigned long blinkInterval = 400UL; 
unsigned long previousMillis = 0UL;
unsigned long previousJoystickMillis = 0UL;
unsigned long interval = 300UL; 
bool editingHours = true; 

boolean showCursor = true;

//Pomodoro Feature variables
unsigned long pomoCycleStart = 0;
unsigned long pomoDuration = 25 * 60 * 1000UL; 
bool isBreakMode = false;
int completedCycles = 0;

//Tomodachi Pet Feature variables
int petHappiness = 100;
unsigned long lastDecayMillis = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("--- StudyBox Booting Up ---");

    lcd.begin(16,2);
    lcd.clear();
    
    servo.attach(servoPin);
    servo.write(0); 

    pinMode(SW_PIN, INPUT_PULLUP); 
    pinMode(EMERGENCY_PIN, INPUT_PULLUP); // NEW: Initialize emergency hardware button pin
    // pinMode(buzzerPin, OUTPUT);
    
    setCurrentPage(startPage);
    Serial.println("Setup Completed Successfully.");
}

void loop() {
    currentMillis = millis();
    int xValue = analogRead(X_PIN);
    int yValue = analogRead(Y_PIN);
    
    boolean swPressed = (digitalRead(SW_PIN) == LOW);
    boolean emergencyPressed = (digitalRead(EMERGENCY_PIN) == LOW); // NEW: Continuously sample emergency input state

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

void stateStart(boolean swPressed) {
    if (swPressed) {
        delay(250); 
        currentState = STATE_TIME_SELECT;
        previousJoystickMillis = millis(); 
        setCurrentPage(timeSelectPage);
        updateTimeDisplay(); 
    }
}

void stateTimeSelect(boolean swPressed, int xValue, int yValue) {
    if (swPressed) {
        delay(250); 
        if (editingHours) {
            editingHours = false;
            cursorPosition[0] = 8; 
            refreshPage();
        } 
        else {
            editingHours = true; 
            studyDurationMs = ((selectedHours * 3600UL) + (selectedMinutes * 60UL)) * 1000UL;
            currentState = STATE_LOCKING;
            setCurrentPage(timeStartedPage);
            return; 
        }
    }

    blinkCursor();
    
    if (currentMillis - previousJoystickMillis > interval) {
        if (xValue < 300) { 
            if (editingHours && selectedHours > 0) selectedHours--;
            else if (!editingHours && selectedMinutes > 0) selectedMinutes--;
            updateTimeDisplay();
            previousJoystickMillis = currentMillis; 
        }
        else if (xValue > 700) { 
            if (editingHours && selectedHours < 99) selectedHours++;
            else if (!editingHours && selectedMinutes < 59) selectedMinutes++;
            updateTimeDisplay();
            previousJoystickMillis = currentMillis; 
        }
    }
}

void stateLocking() {
    servo.write(90); 
    delay(1000);     
    studyTimerStart = millis();
    pomoCycleStart = millis();
    lastDecayMillis = millis();
    
    strcpy(mainMenuPage.bottom, menuOptions[currentMenuSelection - 1]);
    currentState = STATE_MAIN_MENU;
    setCurrentPage(mainMenuPage);
}

void stateMainMenu(boolean swPressed, int xValue, boolean emergencyPressed) {
    // NEW: Intercept menu runtime loops if physical panic override is clicked
    if (emergencyPressed) {
        currentState = STATE_EMERGENCY;
        setCurrentPage(emergencyPage);
        return;
    }

    if (currentMillis - previousMillis > interval) {
        bool changed = false;

        if (xValue < 300) { 
            if (currentMenuSelection > 1) {
                currentMenuSelection--;
                changed = true;
            }
            previousMillis = currentMillis;
        } 
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

void stateActiveApp(boolean swPressed, int xValue, int yValue, boolean emergencyPressed) {
    // NEW: Instantly break out from actively viewed sub-apps (Pomodoro/Pet/Stats) if emergency occurs
    if (emergencyPressed) {
        currentState = STATE_EMERGENCY;
        setCurrentPage(emergencyPage);
        return;
    }

    runActiveCountdown();

    if (currentState == STATE_POMODORO) {
        updatePomodoroLogic();
        // Skip/Force next Pomodoro cycle manually via Joystick Left/Right, Maybe get rid of this
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
        //Give attention/play by moving joystick Right
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
    
    if (swPressed) { 
        delay(250);
        currentState = STATE_MAIN_MENU;
        setCurrentPage(mainMenuPage);
    }
}

void stateTimeLeftView(boolean swPressed, boolean emergencyPressed) {
    //check emergency button during time tracking screens
    if (emergencyPressed) {
        currentState = STATE_EMERGENCY;
        setCurrentPage(emergencyPage);
        return;
    }

    runActiveCountdown();
    
    if (currentState == STATE_TIME_LEFT) {
        updateCountdownDisplay();
    }

    if (swPressed) {
        delay(250);
        currentState = STATE_MAIN_MENU;
        setCurrentPage(mainMenuPage);
    }
}

void stateEmergency() {
    //open up physical box safe lock instantly
    servo.write(0); 
    delay(500);
    
    // Proceed to final end screen state to clear state machine cycle
    currentState = STATE_END_SCREEN;
    setCurrentPage(endScreenPage);

    //Happiness set to 0% as punishment
    petHappiness = 0;
}

void stateEndScreen(boolean swPressed) {
    servo.write(0); 
    if (swPressed) {
        delay(250);
        currentState = STATE_START;
        setCurrentPage(startPage);
    }
}

void updateTimeDisplay() {
    char timeBuffer[17];
    sprintf(timeBuffer, "   < %02d:%02d >    ", selectedHours, selectedMinutes);
    strcpy(currentPage.bottom, timeBuffer); 
    refreshPage();
}

void updateCountdownDisplay() {
    static unsigned long lastUpdateMillis = 0;
    if (currentMillis - lastUpdateMillis > 200) {
        unsigned long elapsed = currentMillis - studyTimerStart;
        unsigned long remainingMs = (studyDurationMs > elapsed) ? (studyDurationMs - elapsed) : 0;
        
        unsigned long totalSecs = remainingMs / 1000UL;
        int hours = totalSecs / 3600UL;
        int mins = (totalSecs % 3600UL) / 60UL;
        int secs = totalSecs % 60UL;

        char displayBuffer[17];
        sprintf(displayBuffer, "    %02d:%02d:%02d    ", hours, mins, secs);
        
        strcpy(currentPage.bottom, displayBuffer);
        refreshPage();
        lastUpdateMillis = currentMillis;
    }
}

void updatePomodoroLogic() {
    static long lastPomoRender = 0;
    if (currentMillis - lastPomoRender > 250) {
        long elapsedPomo = currentMillis - pomoCycleStart;
        
        if (elapsedPomo >= pomoDuration) {
            // cycle expired, toggle modes automatically
            isBreakMode = !isBreakMode;
            pomoDuration = (isBreakMode) ? (5 * 60 * 1000UL) : (25 * 60 * 1000UL);
            pomoCycleStart = currentMillis;
            if (!isBreakMode) completedCycles++;
            return;
        }

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

void updatePetLogic() {
    // Decay pet happiness every 12 seconds
    if (currentMillis - lastDecayMillis > 12000UL) {
        if (petHappiness > 0) petHappiness -= 2;
        lastDecayMillis = currentMillis;
    }

    static unsigned long lastPetRender = 0;
    if (currentMillis - lastPetRender > 250) {
        char topBuffer[17];
        char btmBuffer[17];

        char expression = 'o';
        if (petHappiness < 30) expression = 'x';
        else if (petHappiness > 75) expression = '^';

        sprintf(topBuffer, "Pet:   ( %c_%c )  ", expression, expression);
        sprintf(btmBuffer, "Happiness: %d%% ", petHappiness);

        strcpy(currentPage.top, topBuffer);
        strcpy(currentPage.bottom, btmBuffer);
        refreshPage();
        lastPetRender = currentMillis;
    }
}

void updateStatsLogic() {
    static unsigned long lastStatsRender = 0;
    if (currentMillis - lastStatsRender > 1000) {
        unsigned long elapsedSecs = (currentMillis - studyTimerStart) / 1000UL;
        int activeHours = elapsedSecs / 3600UL;
        int activeMins = (elapsedSecs % 3600UL) / 60UL;

        char topBuffer[17];
        char btmBuffer[17];
        sprintf(topBuffer, "Session: %02dh:%02dm", activeHours, activeMins);
        
        // efficiency score relative to completed pomodoro focus milestones
        int efficiencyScore = min(100, (completedCycles * 35) + 40); 
        sprintf(btmBuffer, "Focus Score:%d%%", efficiencyScore);

        strcpy(currentPage.top, topBuffer);
        strcpy(currentPage.bottom, btmBuffer);
        refreshPage();
        lastStatsRender = currentMillis;
    }
}

void runActiveCountdown() {
    unsigned long elapsed = millis() - studyTimerStart;
    if (elapsed >= studyDurationMs) {
        currentState = STATE_END_SCREEN;
        setCurrentPage(endScreenPage);
    }
}

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

void setCursorPosition(){
  lcd.setCursor(cursorPosition[0], cursorPosition[1]);
  blinkCursor();
}

void refreshPage(){
  printPage(currentPage);
}

void setCurrentPage(Page page) {
    currentPage = page;
    cursorPosition[0] = page.allowedCells[0].col;
    cursorPosition[1] = page.allowedCells[0].row;
    printPage(currentPage);
}

void blinkCursor(){
  if (currentMillis - previousBlinkMillis > blinkInterval) {
    showCursor = !showCursor;
    previousBlinkMillis = currentMillis;
    
    lcd.setCursor(cursorPosition[0], cursorPosition[1]);
    if (showCursor) {
      lcd.print('_');
    } else {
      char currentChar = cursorPosition[1] == 0 
          ? currentPage.top[cursorPosition[0]] 
          : currentPage.bottom[cursorPosition[0]];
      if (currentChar == '\0' || currentChar == '\255') currentChar = ' ';
      lcd.print(currentChar);
    }
  }
}

void printPage(Page page) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(page.top);
    lcd.setCursor(0, 1);
    lcd.print(page.bottom);
}