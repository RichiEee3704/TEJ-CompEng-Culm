#include <Arduino.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); // RS, E, D4, D5, D6, D7
void printPage(char page[2][16]);
void printTop(String str);

char page1[2][16] = {
  "Hello,World",
  "Line 2 Here"
};

// String str = "Hello Tim";

void setup() {

  lcd.begin(16,2);

}

void loop() {
  printPage(page1);
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