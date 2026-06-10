#include <Arduino.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <pages.h>
#include <customChars.h>
#include <Servo.h>

const int X_PIN = A0; // Analog pin for X-axis
const int Y_PIN = A1; // Analog pin for Y-axis
const int SW_PIN = 7; // Digital pin for button (SW)

const int leftButtonPin = 9;
const int rightButtonPin = 8;

const int buzzerPin = 6;

const int servoPin = 10;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // RS, E, D4, D5, D6, D7

Servo servo;

void printPage(Page page);
void setCursorPosition();
void setCurrentPage(Page page);
void blinkCursor();
void updateCursorFromJoystick(int joystickX, int joystickY);
void refreshPage();
void moveCursor(int colDir, int rowDir);
void runActiveCountdown();
void updateTimeDisplay();

void stateStart(boolean swPressed);
void stateTimeSelect(boolean swPressed, int xValue, int yValue, boolean btnLeft, boolean btnRight);
void stateLocking();
void stateMainMenu(boolean swPressed, boolean btnLeft, boolean btnRight);
void stateActiveApp(boolean swPressed);
void stateEmergency();
void stateEndScreen(boolean swPressed);

StudyState currentState = STATE_START;
Page currentPage;

int cursorPosition[2] = {0,0};
int currentMenuSelection = 1; 

// Dynamic Timer values
int selectedHours = 0;       
int selectedMinutes = 25;
unsigned long studyTimerStart = 0;
unsigned long studyDurationMs = 0;

unsigned long currentMillis;
unsigned long previousBlinkMillis = 0UL;
unsigned long blinkInterval = 400UL; // Adjusted for human visible pacing
unsigned long previousMillis = 0UL;
unsigned long previousButtonMillis = 0UL;
unsigned long interval = 150UL;
bool editingHours = true; // true = setting hours, false = setting minutes
// String str = "Hello Tim";

// Keep track of cursor state globally to prevent looping redraws
boolean showCursor = true;

void setup() {
    Serial.begin(9600);
    Serial.println("--- StudyBox Booting Up ---");

    lcd.begin(16,2);
    lcd.clear();
    
    servo.attach(servoPin);
    servo.write(0); 

    pinMode(SW_PIN, INPUT_PULLUP); 
    pinMode(leftButtonPin, INPUT_PULLUP);
    pinMode(rightButtonPin, INPUT_PULLUP);
    pinMode(buzzerPin, OUTPUT);
    
    setCurrentPage(startPage);
    Serial.println("Setup Completed Successfully.");
}

void loop() {
    currentMillis = millis();
    int xValue = analogRead(X_PIN);
    int yValue = analogRead(Y_PIN);
    
    boolean swPressed = (digitalRead(SW_PIN) == LOW);
    boolean btnLeft = (digitalRead(leftButtonPin) == LOW);
    boolean btnRight = (digitalRead(rightButtonPin) == LOW);

    switch (currentState) {
        case STATE_START:       stateStart(swPressed);                                      break;
        case STATE_TIME_SELECT: stateTimeSelect(swPressed, xValue, yValue, btnLeft, btnRight);  break;
        case STATE_LOCKING:     stateLocking();                                             break;
        case STATE_MAIN_MENU:   stateMainMenu(swPressed, btnLeft, btnRight);                break;
        case STATE_POMODORO:    
        case STATE_PET:         
        case STATE_MUSIC:       
        case STATE_STATS:       stateActiveApp(swPressed);                                  break;
        case STATE_EMERGENCY:   stateEmergency();                                           break;
        case STATE_END_SCREEN:  stateEndScreen(swPressed);                                  break;
    }

    // currentMillis = millis();
    
    // boolean swPressed = (digitalRead(SW_PIN) == LOW);
    // boolean btnLeft = (digitalRead(leftButtonPin) == LOW);
    // boolean btnRight = (digitalRead(rightButtonPin) == LOW);

    // // testttttttttt
    // Serial.print("SW: "); Serial.print(swPressed);
    // Serial.print(" | Left Button: "); Serial.print(btnLeft);
    // Serial.print(" | Right Button: "); Serial.println(btnRight);
    
    // delay(100); // Slow down the feed so you can read it clearly
}

void stateStart(boolean swPressed) {
    if (swPressed) {
        delay(250); 
        currentState = STATE_TIME_SELECT;
        
        // Match timestamps instantly to bypass interval checks on your first button press
        previousButtonMillis = millis(); 
        
        // Force the initial variables to load up smoothly
        setCurrentPage(timeSelectPage);
        updateTimeDisplay(); 
    }
}

void stateTimeSelect(boolean swPressed, int xValue, int yValue, boolean btnLeft, boolean btnRight) {
    // 1. Handle joystick click transitions
    if (swPressed) {
        delay(250); // Debounce
        
        if (editingHours) {
            editingHours = false;
            cursorPosition[0] = 8; // Move cursor position under minutes column
            refreshPage();
        } 
        else {
            editingHours = true; // Reset back to default for next setup cycle
            
            // Calculate total milliseconds safely using unsigned longs
            studyDurationMs = ((selectedHours * 3600UL) + (selectedMinutes * 60UL)) * 1000UL;
            
            currentState = STATE_LOCKING;
            setCurrentPage(timeStartedPage);
            return; 
        }
    }

    //Animate the cursor blink dynamically
    blinkCursor();
    
    //Edit numerical values using the independent button timer variable
    if (currentMillis - previousButtonMillis > interval) {
        if (btnLeft) { 
            if (editingHours && selectedHours > 0) selectedHours--;
            else if (!editingHours && selectedMinutes > 0) selectedMinutes--;
            
            updateTimeDisplay();
            previousButtonMillis = currentMillis; 
        }
        else if (btnRight) { 
            if (editingHours && selectedHours < 99) selectedHours++;
            else if (!editingHours && selectedMinutes < 59) selectedMinutes++;
            
            updateTimeDisplay();
            previousButtonMillis = currentMillis; 
        }
    }
}

void stateLocking() {
    servo.write(90); 
    delay(1000);     
    studyTimerStart = millis();
    
    strcpy(mainMenuPage.bottom, menuOptions[currentMenuSelection - 1]);
    currentState = STATE_MAIN_MENU;
    setCurrentPage(mainMenuPage);
}

void stateMainMenu(boolean swPressed, boolean btnLeft, boolean btnRight) {
    if (currentMillis - previousMillis > interval) {
        bool changed = false;

        if (btnLeft) { 
            if (currentMenuSelection > 1) {
                currentMenuSelection--;
                changed = true;
            }
        } 
        else if (btnRight) { 
            if (currentMenuSelection < TOTAL_MENU_ITEMS) {
                currentMenuSelection++;
                changed = true;
            }
        }

        if (changed) {
            strcpy(mainMenuPage.bottom, menuOptions[currentMenuSelection - 1]);
            refreshPage();
            previousMillis = currentMillis;
        }
    }

    if (swPressed) {
        delay(100);
        switch(currentMenuSelection) {
            case 1: currentState = STATE_POMODORO; setCurrentPage(pomodoroPage); break;
            case 2: currentState = STATE_PET;      setCurrentPage(petPage);      break;
            case 3: currentState = STATE_MUSIC;    setCurrentPage(musicPage);    break;
            case 4: currentState = STATE_STATS;    setCurrentPage(statsPage);    break;
        }
    }
}

void stateActiveApp(boolean swPressed) {
    runActiveCountdown();
    
    if (swPressed) { 
        delay(250);
        currentState = STATE_MAIN_MENU;
        setCurrentPage(mainMenuPage);
    }
}

void stateEmergency() {
    for(int i = 0; i < 5; i++){
        digitalWrite(buzzerPin, HIGH);
        delay(100);
        digitalWrite(buzzerPin, LOW);
        delay(100);
    }
    currentState = STATE_END_SCREEN;
    setCurrentPage(endScreenPage);
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
    // Format layout securely: "   < HH:MM >    "
    sprintf(timeBuffer, "   < %02d:%02d >    ", selectedHours, selectedMinutes);
    
    // Inject the freshly typed characters straight into the ACTIVE runtime page buffer
    strcpy(currentPage.bottom, timeBuffer); 
    
    // Push the changes instantly onto the physical hardware panel
    refreshPage();
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

void updateCursorFromJoystick(int joystickX, int joystickY) {
    if (currentMillis - previousMillis > interval) {
        bool dynamicMove = false;
        if (joystickX > 700) { moveCursor(-1, 0); dynamicMove = true; }
        else if (joystickX < 323) { moveCursor(1, 0); dynamicMove = true; }
        if (joystickY > 850) { moveCursor(0, -1); dynamicMove = true; }
        else if (joystickY < 173) { moveCursor(0, 1); dynamicMove = true; }
        
        if (dynamicMove) {
            previousMillis = currentMillis;
            refreshPage();
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

// Rewritten, stable toggle-based cursor blinker
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