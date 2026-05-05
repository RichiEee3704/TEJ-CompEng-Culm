#include <Arduino.h>
#include <LiquidCrystal.h>
#include <string.h>

const int X_PIN = A0; // Analog pin for X-axis
const int Y_PIN = A1; // Analog pin for Y-axis
const int SW_PIN = 7; // Digital pin for button (SW)

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // RS, E, D4, D5, D6, D7

void printPage(char page[2][16]);
void printTop(String str);
void setCursorPosition();
void setCurrentPage();
void blinkCursor();

char page1[2][16] = {
  "Hello,World",
  "Line 2 Here"
};

int cursorPosition[2] = {0,0};
char currentPage[2][16];

unsigned long previousBlinkMillis = 0UL;
unsigned long blinkInterval = 1000UL;
unsigned long previousMillis = 0UL;
unsigned long interval = 100UL;
// String str = "Hello Tim";

void setup() {
  
  lcd.begin(16,2);
  pinMode(SW_PIN, INPUT_PULLUP); 


  Serial.begin(9600);
}

void loop() {
  unsigned long currentMillis = millis();

  int xValue = analogRead(X_PIN);
  int yValue = analogRead(Y_PIN);
  setCursorPosition();
  if(currentMillis - previousMillis > interval){
    if(xValue > 850 && cursorPosition[0] > 0) cursorPosition[0]--;
    else if(xValue < 173 && cursorPosition[0] < 15) cursorPosition[0]++;
    if(yValue > 850 && cursorPosition[1] > 0) cursorPosition[1]--;
    else if(yValue < 173 && cursorPosition[1] < 1) cursorPosition[1]++;
    previousMillis = currentMillis;
    setCurrentPage();// ONLY redraw when position changes

  }
  // Read digital value (0 = pressed, 1 = not pressed)
  // int swVal = digitalRead(SW_PIN);

}

void setCursorPosition(){
  lcd.setCursor(cursorPosition[0], cursorPosition[1]);
  blinkCursor();
}

void setCurrentPage(){
  memcpy(currentPage, page1, sizeof(page1));
  printPage(currentPage);
}

void blinkCursor(){
  unsigned long currentMillis = millis();
  char currentChar = currentPage[cursorPosition[1]][cursorPosition[0]];
  if (currentChar == '\0') currentChar = ' ';
  
  lcd.setCursor(cursorPosition[0], cursorPosition[1]);//Makes sure that cursor is on correct position
  

  if(currentMillis - previousBlinkMillis >250){
    lcd.print('_');
    if(currentMillis - previousBlinkMillis > 500){
    lcd.setCursor(cursorPosition[0], cursorPosition[1]);//Makes sure that cursor is on correct position
    lcd.print(currentChar);
    previousBlinkMillis = currentMillis;
    }
  }
}

void printPage(char page[2][16]){
  for(int row = 0; row < 2; row++){
    String str = page[row];
    for(int col = 0; col < 16; col++){
      lcd.setCursor(col, row);
      if(col >= (int)str.length()) lcd.print(" ");
      else lcd.print(page[row][col]);
    }
  }
}