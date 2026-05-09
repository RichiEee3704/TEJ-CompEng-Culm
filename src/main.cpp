#include <Arduino.h>
#include <LiquidCrystal.h>
#include <string.h>
#include <pages.h>
#include <customChars.h>

const int X_PIN = A0; // Analog pin for X-axis
const int Y_PIN = A1; // Analog pin for Y-axis
const int SW_PIN = 7; // Digital pin for button (SW)

const int leftButtonPin = 9;
const int rightButtonPin = 8;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // RS, E, D4, D5, D6, D7

void printPage(Page page);
void printTop(String str);
void setCursorPosition();
void setCurrentPage(Page page = startPage);
void blinkCursor();
void updateCursorFromJoystick(int joystickX, int joystickY);
void refreshPage();
void switchPage(boolean button1, boolean button2);
void moveCursor(int colDir, int rowDir);

int cursorPosition[2] = {0,0};
Page currentPage;
int pageCount = 3;
int currentPageIndex = 0;


unsigned long currentMillis;
unsigned long previousBlinkMillis = 0UL;
unsigned long blinkInterval = 1000UL;
unsigned long previousMillis = 0UL;
unsigned long interval = 150UL;
String str = "Hello Tim";

void setup() {
  
  lcd.begin(16,2);
  pinMode(SW_PIN, INPUT_PULLUP); 
  pinMode(leftButtonPin, INPUT_PULLUP);
  pinMode(rightButtonPin, INPUT_PULLUP);
  setCurrentPage();

  Serial.begin(9600);
}

void loop() {
  currentMillis = millis();
  int xValue = analogRead(X_PIN);
  int yValue = analogRead(Y_PIN);

  boolean leftButtonInput = !digitalRead(leftButtonPin);
  boolean rightButtonInput = !digitalRead(rightButtonPin);

  static boolean prevLeft = false;
  static boolean prevRight = false;

  setCursorPosition();
  updateCursorFromJoystick(xValue, yValue);

  // Only trigger on the rising edge (moment of press)
  if (leftButtonInput && !prevLeft)   switchPage(true, false);
  if (rightButtonInput && !prevRight) switchPage(false, true);

  prevLeft = leftButtonInput;
  prevRight = rightButtonInput;
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
    // if not found, cursor doesn't move
}

void updateCursorFromJoystick(int joystickX, int joystickY) {
    if (currentMillis - previousMillis > interval) {
        if (joystickX > 700) moveCursor(-1, 0);
        else if (joystickX < 323) moveCursor(1, 0);
        if (joystickY > 850) moveCursor(0, -1);
        else if (joystickY < 173) moveCursor(0, 1);
        previousMillis = currentMillis;
        refreshPage();
    }
}

void switchPage(boolean button1, boolean button2) {
    if (button1 && currentPageIndex > 0) {
        currentPageIndex--;
        setCurrentPage(pages[currentPageIndex]);
    }
    else if (button2 && currentPageIndex < pageCount - 1) {
        currentPageIndex++;
        setCurrentPage(pages[currentPageIndex]);
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
  unsigned long currentMillis = millis();
  char currentChar = cursorPosition[1] == 0 
      ? currentPage.top[cursorPosition[0]] 
      : currentPage.bottom[cursorPosition[0]];
  if (currentChar == '\0') currentChar = ' ';
  
  lcd.setCursor(cursorPosition[0], cursorPosition[1]);

  if(currentMillis - previousBlinkMillis > 250){
    lcd.print('_');
    if(currentMillis - previousBlinkMillis > 500){
      lcd.setCursor(cursorPosition[0], cursorPosition[1]);
      lcd.print(currentChar);
      previousBlinkMillis = currentMillis;
    }
  }
}

void printPage(Page page) {
    for (int col = 0; col < 16; col++) {
        lcd.setCursor(col, 0);
        lcd.print(page.top[col]);
    }
    for (int col = 0; col < 16; col++) {
        lcd.setCursor(col, 1);
        lcd.print(page.bottom[col]);
    }
}