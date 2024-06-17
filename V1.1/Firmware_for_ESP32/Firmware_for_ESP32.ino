/*
 ESP32 Tetris with Nintendo 64 Controller
 Based on the work of Brian Lough
 Adapted for Nintendo 64 Controller by Maxim Bortnikov
 For more information please visit
 https://github.com/Northstrix/ESP32-Tetris-With-Nintendo-64-Controller
 Original project description is presented below:
*/
/*******************************************************************
    Testing using a Nintendo Wii Nunchuck with the CYD

    More info on Nunchucks with Arduino: https://youtu.be/Cl9f1DUbMnc

    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display

    Addtional Hardware Required:
      - Nunchuck adapter
        - My open source one from Oshpark: https://oshpark.com/shared_projects/RcIxSx2D
        - From Aliexpress*: https://s.click.aliexpress.com/e/_AEEtc3
      - A nintendo Nunchuck
        - Amazon.co.uk Search*: https://amzn.to/3nQrXcE
        - Amazon.com Search*: https://amzn.to/3nRJTUd
        - Aliexpress (Third Party)*: https://s.click.aliexpress.com/e/_AaQbXh

     = Affiliate Links

    Wiring:
      - Plug wire that came with CYD into the JST connector closest to the Micro SD card slot
      - Connect the wire to the adapter as follows:
          CYD  -> Adapter
          ------------------------------------------
          GND  -> - (AKA GND) - Black wire for me
          3.3V -> + (AKA 3V)  - Red wire for me
          IO22 -> d (AKA SDA) - Blue wire for me
          IO27 -> c (AKA SCL) - Yellow wire for me

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/

    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// Make sure to copy the UserSetup.h file into the library as
// per the Github Instructions. The pins are defined in there.

// ----------------------------
// Standard Libraries
// ----------------------------

#include <Wire.h>
#include <TFT_eSPI.h>
#include "Crypto.h"
#include "images.h"
#include "custom_hebrew_font.h"
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI

TFT_eSPI tft = TFT_eSPI();
//--------------------------------
//Game Config Options:
//--------------------------------

#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 320

#define WORLD_TO_PIXEL 14 //each dot on the game world will be reperesented by these many pixels.

#define DELAY_BETWEEN_FRAMES 50
#define DELAY_ON_LINE_CLEAR 100

#define NUM_PIECES_TIL_SPEED_CHANGE 20

#define PRINT_DELAY 1000

bool fillBlocks = true;
int x;
bool rec_data;

uint16_t myRED = TFT_RED;
uint16_t myGREEN = TFT_GREEN;
uint16_t myBLUE = TFT_BLUE;
uint16_t myWHITE = TFT_WHITE;
uint16_t myYELLOW = TFT_YELLOW;
uint16_t myCYAN = TFT_CYAN;
uint16_t myMAGENTA = TFT_MAGENTA;
uint16_t myBLACK = TFT_BLACK;

// [0] is empty space
// [1-7] are tetromino colours
// [8] is the colour a completed line changes to before dissapearing
// [9] is the walls of the board.
uint16_t gameColours[10] = {myBLACK, myRED, myGREEN, myBLUE, myWHITE, myYELLOW, myCYAN, myMAGENTA, myRED, myGREEN};

char tetromino[7][17];
int nFieldWidth = 12;
int nFieldHeight = 18;

int leftOffset = (DISPLAY_WIDTH - (nFieldWidth * WORLD_TO_PIXEL)) / 2;
int topOffset = DISPLAY_HEIGHT - 20 - (nFieldHeight * WORLD_TO_PIXEL);

int centerScreenX = DISPLAY_WIDTH / 2;

char *pField = nullptr;

RNG random_number;

bool show_game_over_inscr;

int Rotate(int px, int py, int r)
{
  int pi = 0;
  switch (r % 4)
  {
    case 0: // 0 degrees      // 0  1  2  3
      pi = py * 4 + px;     // 4  5  6  7
      break;            // 8  9 10 11
    //12 13 14 15

    case 1: // 90 degrees     //12  8  4  0
      pi = 12 + py - (px * 4);  //13  9  5  1
      break;                      //14 10  6  2
    //15 11  7  3

    case 2: // 180 degrees      //15 14 13 12
      pi = 15 - (py * 4) - px;  //11 10  9  8
      break;                    // 7  6  5  4
    // 3  2  1  0

    case 3: // 270 degrees      // 3  7 11 15
      pi = 3 - py + (px * 4);   // 2  6 10 14
      break;                    // 1  5  9 13
  }   // 0  4  8 12

  return pi;
}

bool DoesPieceFit(int nTetromino, int nRotation, int nPosX, int nPosY)
{
  // All Field cells >0 are occupied
  for (int px = 0; px < 4; px++)
    for (int py = 0; py < 4; py++)
    {
      // Get index into piece
      int pi = Rotate(px, py, nRotation);

      // Get index into field
      int fi = (nPosY + py) * nFieldWidth + (nPosX + px);

      // Check that test is in bounds. Note out of bounds does
      // not necessarily mean a fail, as the long vertical piece
      // can have cells that lie outside the boundary, so we'll
      // just ignore them
      if (nPosX + px >= 0 && nPosX + px < nFieldWidth)
      {
        if (nPosY + py >= 0 && nPosY + py < nFieldHeight)
        {
          // In Bounds so do collision check
          if (tetromino[nTetromino][pi] != L'.' && pField[fi] != 0)
            return false; // fail on first hit
        }
      }
    }

  return true;
}

void disp_centered_text_b_w(String text, int h, uint16_t hghl_color, uint16_t text_color) {
  tft.setTextColor(hghl_color);
  tft.drawCentreString(text, 120, h - 1, 1);
  tft.drawCentreString(text, 120, h + 1, 1);
  tft.drawCentreString(text, 119, h, 1);
  tft.drawCentreString(text, 121, h, 1);
  tft.setTextColor(text_color);
  tft.drawCentreString(text, 120, h, 1);
}

void show_lock_screen(){
  show_game_over_inscr = false;
  byte init_scr = random_number.get() % 5;
  if (init_scr == 0){
    tft.fillScreen(0x1376);
    for (int i = 0; i < 240; i++){
      for (int j = 0; j < 184; j++){
        tft.drawPixel(i, j + 136, Dallas[i][j]);
      }
    }
  }
  if (init_scr == 1){
    tft.fillScreen(64978);
    for (int i = 0; i < 240; i++){
      for (int j = 0; j < 184; j++){
        tft.drawPixel(i, j + 136, Salt_Lake[i][j]);
      }
    }
  }
  if (init_scr == 2){
    tft.fillScreen(196);
    for (int i = 0; i < 182; i++){
      for (int j = 0; j < 185; j++){
        tft.drawPixel(i + 29, j + 135, Jewish_style_building[i][j]);
      }
    }
  }
  if (init_scr == 3){
    tft.fillScreen(0x08a4);
    for (int i = 0; i < 89; i++) {
      for (int j = 0; j < 92; j++) {
        tft.drawPixel(i + 31, j + 98, quarter_pattern[i][j]);
      }
    }
    for (int i = 0; i < 89; i++) {
      for (int j = 0; j < 92; j++) {
        tft.drawPixel(i + 120, j + 98, quarter_pattern[88 - i][j]);
      }
    }

    for (int i = 0; i < 89; i++) {
      for (int j = 0; j < 92; j++) {
        tft.drawPixel(i + 31, j + 190, quarter_pattern[i][91 - j]);
      }
    }
    for (int i = 0; i < 89; i++) {
      for (int j = 0; j < 92; j++) {
        tft.drawPixel(i + 120, j + 190, quarter_pattern[88 - i][91 - j]);
      }
    }
  }
  if (init_scr == 4){
    tft.fillScreen(0x0821);
    for (int i = 0; i < 240; i++){
      for (int j = 0; j < 236; j++){
        tft.drawPixel(i, j + 85, Flower_pattern[i][j]);
      }
    }
  }
  rec_data = false;
  tft.setTextSize(2);
  x = 0;
  if (init_scr == 0 || init_scr == 1){
    disp_centered_text_b_w("ESP32 Tetris", 5, 0x0882, 0xf7de);
    disp_centered_text_b_w("With", 25, 0x0882, 0xf7de);
    disp_centered_text_b_w("Nintendo 64", 45, 0x0882, 0xf7de);
    disp_centered_text_b_w("Controller", 65, 0x0882, 0xf7de);
  }
  else{
    disp_centered_text_b_w("ESP32 Tetris", 5, 0x1376, 0xf7de);
    disp_centered_text_b_w("With", 25, 0x1376, 0xf7de);
    disp_centered_text_b_w("Nintendo 64", 45, 0x1376, 0xf7de);
    disp_centered_text_b_w("Controller", 65, 0x1376, 0xf7de);
  }
  
  unsigned long previousMillis = 0;
  bool sw = false;
  bool cont_to_next = false;
  while (x != 13) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= 1200 && sw == false) {
      previousMillis = currentMillis;
      if (init_scr == 0 || init_scr == 1)
        disp_centered_text_b_w("Press Start", 105, 0x0882, 0xf7de);
      else if (init_scr == 2)
        disp_centered_text_b_w("Press Start", 105, 0xf7de, 0x0882);
      else if (init_scr > 2)
        disp_centered_text_b_w("Press Start", 300, 0xf7de, 0x0882);
      sw = true;
    }

    if (currentMillis - previousMillis >= 1200 && sw == true) {
      previousMillis = currentMillis;
      if (init_scr == 0 || init_scr == 2)
        disp_centered_text_b_w("Press Start", 105, 0xf7de, 0x1376);
      
      if (init_scr == 1)
        disp_centered_text_b_w("Press Start", 105, 0x0882, 64978);
      
      if (init_scr > 2)
        disp_centered_text_b_w("Press Start", 300, 0xf7de, 0x1376);
      sw = false;
    }

    delay(1);
  }

  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
}

void setup() {

  //Serial.begin(115200);
  
  tft.init();
  tft.setRotation(0); //This is the display in portrait

  Wire.setPins(22, 27);  // Set custom pins for I2C SDA and SCL
  Wire.begin(13);        // Initialize I2C slave mode with address 13
  Wire.onReceive(receiveEvent);  // Register receiveEvent function for I2C data reception
  
  show_lock_screen();
  
  strcpy(tetromino[0], "..X...X...X...X."); // Tetronimos 4x4
  strcpy(tetromino[1], "..X..XX...X.....");
  strcpy(tetromino[2], ".....XX..XX.....");
  strcpy(tetromino[3], "..X..XX..X......");
  strcpy(tetromino[4], ".X...XX...X.....");
  strcpy(tetromino[5], ".X...X...XX.....");
  strcpy(tetromino[6], "..X...X..XX.....");

  // inits field
  restartGame();

  tft.fillScreen(TFT_BLACK);
}

bool bGameOver = false;

bool moveLeft;
bool moveRight;
bool dropDown;
bool rotatePiece;
bool cButtonPressed;

bool isPaused;

int moveThreshold = 30;

int nCurrentPiece = 2;
int nCurrentRotation = 0;
int nCurrentX = (nFieldWidth / 2) - 2;
int nCurrentY = 0;

bool clearPreviousPiece = false;
int nPreviousX = -1;
int nPreviousY = -1;
int nPreviousRotation = -1;
int nPreviousPiece = -1;

bool drawLockedPiece = false;
int nLockedX = -1;
int nLockedY = -1;
int nLockedPiece = -1;
int nLockedRotation = -1;

int nSpeed = 20;
int nSpeedCount = 0;
bool bForceDown = false;
bool bRotateHold = true;

int nPieceCount = 0;
int nScore = 0;
int lastDrawnScore = -1;

int completedLinesIndex[4];
int numCompletedLines;

bool redrawWorld = false;
bool somethingMoved = true;

void getNewPiece()
{
  // Pick New Piece
  nCurrentX = (nFieldWidth / 2) - 2;
  nCurrentY = 0;

  nCurrentRotation = 0;
  nCurrentPiece =  random_number.get() % 7;

  somethingMoved = true;
}

void gameTiming() {
  delay(DELAY_BETWEEN_FRAMES);
  nSpeedCount++;
  bForceDown = (nSpeedCount == nSpeed);
}

void clearLines() {
  if (numCompletedLines > 0)
  {
    delay(DELAY_ON_LINE_CLEAR);
    for (int i = 0; i < numCompletedLines; i ++ )
    {
      for (int px = 1; px < nFieldWidth - 1; px++)
      {
        for (int py = completedLinesIndex[i]; py > 0; py--)
          pField[py * nFieldWidth + px] = pField[(py - 1) * nFieldWidth + px];
        pField[px] = 0;
      }
    }
    numCompletedLines = 0;
    redrawWorld = true;
  }
}

void gameLogic() {

  somethingMoved = false;

  //Handle updating lines cleared last tick
  clearLines();

  nPreviousX = nCurrentX;
  nPreviousY = nCurrentY;
  nPreviousRotation = nCurrentRotation;
  nPreviousPiece = nCurrentPiece;

  if (moveRight && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) {
    somethingMoved = true;
    nCurrentX += 1;
  }
  if (moveLeft && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) {
    somethingMoved = true;
    nCurrentX -= 1;
  }
  if (dropDown && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1)) {
    somethingMoved = true;
    nCurrentY += 1;
  }

  if (rotatePiece)
  {
    nCurrentRotation += (bRotateHold && DoesPieceFit(nCurrentPiece, nCurrentRotation + 1, nCurrentX, nCurrentY)) ? 1 : 0;
    somethingMoved = true;
    bRotateHold = false;
  }
  else
    bRotateHold = true;

  // Force the piece down the playfield if it's time
  if (bForceDown)
  {
    somethingMoved = true;
    // Update difficulty every 50 pieces
    nSpeedCount = 0;
    nPieceCount++;
    if (nPieceCount % NUM_PIECES_TIL_SPEED_CHANGE == 0)
      if (nSpeed >= 10) nSpeed--;

    // Test if piece can be moved down
    if (DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY + 1))
      nCurrentY++; // It can, so do it!
    else
    {
      // It can't! Lock the piece in place
      for (int px = 0; px < 4; px++)
        for (int py = 0; py < 4; py++)
          if (tetromino[nCurrentPiece][Rotate(px, py, nCurrentRotation)] != '.')
            pField[(nCurrentY + py) * nFieldWidth + (nCurrentX + px)] = nCurrentPiece + 1;

      nLockedX = nCurrentX;
      nLockedY = nCurrentY;
      nLockedRotation = nCurrentRotation;
      nLockedPiece = nCurrentPiece;
      drawLockedPiece = true;

      // Check for lines
      for (int py = 0; py < 4; py++)
        if (nCurrentY + py < nFieldHeight - 1)
        {
          bool bLine = true;
          for (int px = 1; px < nFieldWidth - 1; px++)
            bLine &= (pField[(nCurrentY + py) * nFieldWidth + px]) != 0;

          if (bLine)
          {
            // Remove Line, set to =
            for (int px = 1; px < nFieldWidth - 1; px++)
              pField[(nCurrentY + py) * nFieldWidth + px] = 8;
            //vLines.push_back(nCurrentY + py);
            completedLinesIndex[numCompletedLines] = nCurrentY + py;
            numCompletedLines++;
            redrawWorld = true;
          }
        }

      nScore += 25;
      if (numCompletedLines > 0) {
        nScore += (1 << numCompletedLines) * 100;
      }


      // Pick New Piece
      getNewPiece();

      // If piece does not fit straight away, game over!
      bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
    }
  }
  clearPreviousPiece = somethingMoved;
}

void gameInput() {
  moveLeft = false;
  moveRight = false;
  dropDown = false;
  rotatePiece = false;
  
  if (rec_data == true){
    rec_data = false;
    
    if (x == 129)
      moveLeft = true;
     else
      moveLeft = false;
      
    if (x == 130)
      moveRight = true;
    else
      moveRight = false;
    
    if (x == 132)
      dropDown = true;
    else
      dropDown = false; 

    if (x == 131 || x == 133 || x == 27)
      rotatePiece = true;
    else
      rotatePiece = false;
      
    if (x == 13){
      isPaused = false;
      tft.fillRect(0, 10, 240, 20, TFT_BLACK);
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
      tft.drawCentreString(String(nScore), centerScreenX, 10, 4);
    }

    if (x == 8){
      isPaused = true;
      tft.fillRect(0, 10, 240, 20, TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.drawCentreString("Game Paused", centerScreenX, 10, 4);
    }

    x = 0;
  }
}

uint16_t getFieldColour(int index, bool isDeathScreen) {
  if (isDeathScreen && pField[index] != 0) {
    return myRED;
  } else {
    return gameColours[pField[index]];
  }
}

void drawBlock(int x, int y, uint16_t colour) {
  int realX;
  int realY;
  realX = (x * WORLD_TO_PIXEL) + leftOffset;
  realY = (y * WORLD_TO_PIXEL) + topOffset;
  if (WORLD_TO_PIXEL > 1) {
    if (fillBlocks) {
      tft.fillRect(realX, realY, WORLD_TO_PIXEL, WORLD_TO_PIXEL, colour);
      tft.drawRect(realX, realY, WORLD_TO_PIXEL, WORLD_TO_PIXEL, TFT_BLACK);
    } else {
      tft.drawRect(realX, realY, WORLD_TO_PIXEL, WORLD_TO_PIXEL, colour);
    }

  } else {
    tft.drawPixel(realX, realY, colour);
  }
}

void drawPiece(int x, int y, int rotation, int piece, uint16_t colour) {
  for (int px = 0; px < 4; px++) {
    for (int py = 0; py < 4; py++) {
      if (tetromino[piece][Rotate(px, py, rotation)] != '.') {
        drawBlock((x + px), (y + py), colour);
      }
    }
  }
}

void displayLogic(bool isDeathScreen = false) {

  int realX;
  int realY;

  bool forceScoreUpdate = false;

  if (redrawWorld) {
    tft.fillScreen(TFT_BLACK);
    forceScoreUpdate = true;
    // Draw Field
    for (int x = 0; x < nFieldWidth; x++) {
      for (int y = 0; y < nFieldHeight; y++) {
        uint16_t fieldColour = getFieldColour((y * nFieldWidth + x), isDeathScreen);
        if (fieldColour != myBLACK) {
          drawBlock(x, y, fieldColour);
        }
      }
    }
    redrawWorld = false;
  }

  // Clear Previous Movement of Piece
  if (clearPreviousPiece) {
    drawPiece(nPreviousX, nPreviousY, nPreviousRotation, nPreviousPiece, TFT_BLACK);
    clearPreviousPiece = false;
  }

  // This piece previously just locked in, no point redrawing the entire field just to get this.
  if (drawLockedPiece) {
    drawPiece(nLockedX, nLockedY, nLockedRotation, nLockedPiece, gameColours[nLockedPiece + 1]);
    drawLockedPiece = false;
  }

  drawPiece(nCurrentX, nCurrentY, nCurrentRotation, nCurrentPiece, gameColours[nCurrentPiece + 1]);

  if (lastDrawnScore != nScore || forceScoreUpdate) {
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    if (!isPaused){
      tft.drawCentreString(String(nScore), centerScreenX, 10, 4);
      lastDrawnScore = nScore;
    }
  }
}

void restartGame() {

  redrawWorld = true;
  bGameOver = false;
  pField = new char[nFieldWidth * nFieldHeight]; // Create play field buffer
  for (int x = 0; x < nFieldWidth; x++) // Board Boundary
    for (int y = 0; y < nFieldHeight; y++)
      pField[y * nFieldWidth + x] = (x == 0 || x == nFieldWidth - 1 || y == nFieldHeight - 1) ? 9 : 0;

  // Pick New Piece
  getNewPiece();

  nScore = 0;

}

void game_over_inscription(){
  if (show_game_over_inscr == false){
    tft.fillRect(0, 10, 240, 20, TFT_BLACK);
    tft.setTextSize(2);
    disp_centered_text_b_w("Score:" + String(nScore), 10, 0xffff, TFT_RED);
    disp_centered_text_b_w("Game Over", 136, 0xf7de, TFT_RED);
    print_custom_hebrew_font("sif hMSck", 156, 13, 0xffff);
    print_custom_hebrew_font("sif hMSck", 156, 15, 0xffff);
    print_custom_hebrew_font("sif hMSck", 155, 14, 0xffff);
    print_custom_hebrew_font("sif hMSck", 157, 14, 0xffff);
    print_custom_hebrew_font("sif hMSck", 156, 14, TFT_RED);
    tft.setTextSize(2);
    disp_centered_text_b_w("Press Start", 304, 0xffff, TFT_BLUE);
    show_game_over_inscr = true;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!bGameOver)
  {
    gameInput();
    if (!isPaused)
    {
      gameTiming();
      gameLogic();
    } else {
      delay(DELAY_BETWEEN_FRAMES); //stopping pulsing of LEDS
    }
    if (somethingMoved) {
      displayLogic();
    }
  } else {
    tft.setTextSize(1);
    game_over_inscription();
    delay(DELAY_BETWEEN_FRAMES);
    displayLogic(false);
    if (rec_data == true){
      rec_data = false;
      if (x == 13){
        show_lock_screen();
        restartGame();
      }
    }
  }
}

void receiveEvent(int howMany) {
  howMany = howMany;              // clear warning msg
  x = Wire.read();            // receive byte as an integer
  rec_data = true;
  //Serial.println(x);              // print the integer
}

// Functions for custom Hebrew font below

#define letter_spacing_pxls 6
#define space_between_letter 16
#define regular_shift_down 16
#define shift_down_for_mem 12
#define shift_down_for_shin 13
#define shift_down_for_tsadi 8
#define shift_down_for_dot_and_comma 38

void print_custom_hebrew_font(String text_to_print, int y, int offset_from_the_right, uint16_t font_color){
  int shift_right = 240 - offset_from_the_right;
  for (int s = 0; s < text_to_print.length(); s++){ // Traverse the string
    
    if (text_to_print.charAt(s) == ' '){ // Space
      shift_right -= space_between_letter;
    }
    
    if (text_to_print.charAt(s) == 'A'){ // Alef
      shift_right -= sizeof(Alef)/sizeof(Alef[0]);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 24; j++) {
          if (Alef[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '"'){ // Apostrophe
      shift_right -= sizeof(Apostrophe)/sizeof(Apostrophe[0]);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 10; j++) {
          if (Apostrophe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'a'){ // Ayin
      shift_right -= sizeof(Ayin)/sizeof(Ayin[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 24; j++) {
          if (Ayin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'b'){ // Bet
      shift_right -= sizeof(Bet)/sizeof(Bet[0]);
      for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 24; j++) {
          if (Bet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'c'){ // Chet
      shift_right -= sizeof(Chet)/sizeof(Chet[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 24; j++) {
          if (Chet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'd'){ // Dalet
      shift_right -= sizeof(Dalet)/sizeof(Dalet[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Dalet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'm'){ // ending mem
      shift_right -= sizeof(ending_mem)/sizeof(ending_mem[0]);
      for (int i = 0; i < 23; i++) {
        for (int j = 0; j < 24; j++) {
          if (ending_mem[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'n'){ // ending nun
      shift_right -= sizeof(ending_nun)/sizeof(ending_nun[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 39; j++) {
          if (ending_nun[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'f'){ // ending pe
      shift_right -= sizeof(ending_pe)/sizeof(ending_pe[0]);
      for (int i = 0; i < 23; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_pe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'q'){ // ending qaf
      shift_right -= sizeof(ending_qaf)/sizeof(ending_qaf[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_qaf[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'x'){ // ending tsadi
      shift_right -= sizeof(ending_tsadi)/sizeof(ending_tsadi[0]);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_tsadi[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'g'){ // Gimel
      shift_right -= sizeof(Gimel)/sizeof(Gimel[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 24; j++) {
          if (Gimel[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'h'){ // He
      shift_right -= sizeof(He)/sizeof(He[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 24; j++) {
          if (He[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'L'){ // Lamed
      shift_right -= sizeof(Lamed)/sizeof(Lamed[0]);
      if (s != 0)
        shift_right += 12;
      for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 40; j++) {
          if (Lamed[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'M'){ // Mem
      shift_right -= sizeof(Mem)/sizeof(Mem[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 29; j++) {
          if (Mem[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_mem, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'N'){ // Nun
      shift_right -= sizeof(Nun)/sizeof(Nun[0]);
      for (int i = 0; i < 14; i++) {
        for (int j = 0; j < 24; j++) {
          if (Nun[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'p'){ // Pe
      shift_right -= sizeof(Pe)/sizeof(Pe[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Pe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'C'){ // Qaf
      shift_right -= sizeof(Qaf)/sizeof(Qaf[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Qaf[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'k'){ // Qof
      shift_right -= sizeof(Qof)/sizeof(Qof[0]);
      for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 38; j++) {
          if (Qof[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'r'){ // Resh
      shift_right -= sizeof(Resh)/sizeof(Resh[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Resh[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 's'){ // Samekh
      shift_right -= sizeof(Samekh)/sizeof(Samekh[0]);
      for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 24; j++) {
          if (Samekh[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'S'){ // Shin
      shift_right -= sizeof(Shin)/sizeof(Shin[0]);
      for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 27; j++) {
          if (Shin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_shin, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'T'){ // Tev
      shift_right -= sizeof(Tav)/sizeof(Tav[0]);
      for (int i = 0; i < 33; i++) {
        for (int j = 0; j < 24; j++) {
          if (Tav[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 't'){ // Tet
      shift_right -= sizeof(Tet)/sizeof(Tet[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Tet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'X'){ // Tsadi
      shift_right -= sizeof(Tsadi)/sizeof(Tsadi[0]);
      for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 32; j++) {
          if (Tsadi[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_tsadi, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'i'){ // Vav
      shift_right -= sizeof(Vav)/sizeof(Vav[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 24; j++) {
          if (Vav[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '\''){ // Yod
      shift_right -= sizeof(Yod)/sizeof(Yod[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++) {
          if (Yod[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'z'){ // Zayin
      shift_right -= sizeof(Zayin)/sizeof(Zayin[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Zayin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '.'){ // Dot
      shift_right -= sizeof(dot)/sizeof(dot[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
          if (dot[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_dot_and_comma, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == ','){ // Comma
      shift_right -= sizeof(comma)/sizeof(comma[0]);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 7; j++) {
          if (comma[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_dot_and_comma, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '!'){ // Exclamation mark
      shift_right -= sizeof(excl)/sizeof(excl[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 24; j++) {
          if (excl[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '?'){ // Question mark
      shift_right -= sizeof(question_mark)/sizeof(question_mark[0]);
      for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 24; j++) {
          if (question_mark[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_color);
        }
      }
      shift_right -= letter_spacing_pxls;
    }
  }
}

void print_custom_multi_colored_hebrew_font(String text_to_print, int y, int offset_from_the_right, uint16_t font_colors[], int how_many_colors){
  int shift_right = 240 - offset_from_the_right;
  for (int s = 0; s < text_to_print.length(); s++){ // Traverse the string
    
    if (text_to_print.charAt(s) == ' '){ // Space
      shift_right -= space_between_letter;
    }
    
    if (text_to_print.charAt(s) == 'A'){ // Alef
      shift_right -= sizeof(Alef)/sizeof(Alef[0]);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 24; j++) {
          if (Alef[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '"'){ // Apostrophe
      shift_right -= sizeof(Apostrophe)/sizeof(Apostrophe[0]);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 10; j++) {
          if (Apostrophe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'a'){ // Ayin
      shift_right -= sizeof(Ayin)/sizeof(Ayin[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 24; j++) {
          if (Ayin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'b'){ // Bet
      shift_right -= sizeof(Bet)/sizeof(Bet[0]);
      for (int i = 0; i < 22; i++) {
        for (int j = 0; j < 24; j++) {
          if (Bet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'c'){ // Chet
      shift_right -= sizeof(Chet)/sizeof(Chet[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 24; j++) {
          if (Chet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'd'){ // Dalet
      shift_right -= sizeof(Dalet)/sizeof(Dalet[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Dalet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'm'){ // ending mem
      shift_right -= sizeof(ending_mem)/sizeof(ending_mem[0]);
      for (int i = 0; i < 23; i++) {
        for (int j = 0; j < 24; j++) {
          if (ending_mem[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'n'){ // ending nun
      shift_right -= sizeof(ending_nun)/sizeof(ending_nun[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 39; j++) {
          if (ending_nun[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'f'){ // ending pe
      shift_right -= sizeof(ending_pe)/sizeof(ending_pe[0]);
      for (int i = 0; i < 23; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_pe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'q'){ // ending qaf
      shift_right -= sizeof(ending_qaf)/sizeof(ending_qaf[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_qaf[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'x'){ // ending tsadi
      shift_right -= sizeof(ending_tsadi)/sizeof(ending_tsadi[0]);
      for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 38; j++) {
          if (ending_tsadi[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'g'){ // Gimel
      shift_right -= sizeof(Gimel)/sizeof(Gimel[0]);
      for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 24; j++) {
          if (Gimel[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'h'){ // He
      shift_right -= sizeof(He)/sizeof(He[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 24; j++) {
          if (He[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'L'){ // Lamed
      shift_right -= sizeof(Lamed)/sizeof(Lamed[0]);
      if (s != 0)
        shift_right += 12;
      for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 40; j++) {
          if (Lamed[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'M'){ // Mem
      shift_right -= sizeof(Mem)/sizeof(Mem[0]);
      for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 29; j++) {
          if (Mem[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_mem, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'N'){ // Nun
      shift_right -= sizeof(Nun)/sizeof(Nun[0]);
      for (int i = 0; i < 14; i++) {
        for (int j = 0; j < 24; j++) {
          if (Nun[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'p'){ // Pe
      shift_right -= sizeof(Pe)/sizeof(Pe[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Pe[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'C'){ // Qaf
      shift_right -= sizeof(Qaf)/sizeof(Qaf[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Qaf[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'k'){ // Qof
      shift_right -= sizeof(Qof)/sizeof(Qof[0]);
      for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 38; j++) {
          if (Qof[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'r'){ // Resh
      shift_right -= sizeof(Resh)/sizeof(Resh[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Resh[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 's'){ // Samekh
      shift_right -= sizeof(Samekh)/sizeof(Samekh[0]);
      for (int i = 0; i < 24; i++) {
        for (int j = 0; j < 24; j++) {
          if (Samekh[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'S'){ // Shin
      shift_right -= sizeof(Shin)/sizeof(Shin[0]);
      for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 27; j++) {
          if (Shin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_shin, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'T'){ // Tev
      shift_right -= sizeof(Tav)/sizeof(Tav[0]);
      for (int i = 0; i < 33; i++) {
        for (int j = 0; j < 24; j++) {
          if (Tav[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 't'){ // Tet
      shift_right -= sizeof(Tet)/sizeof(Tet[0]);
      for (int i = 0; i < 19; i++) {
        for (int j = 0; j < 24; j++) {
          if (Tet[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'X'){ // Tsadi
      shift_right -= sizeof(Tsadi)/sizeof(Tsadi[0]);
      for (int i = 0; i < 21; i++) {
        for (int j = 0; j < 32; j++) {
          if (Tsadi[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_tsadi, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'i'){ // Vav
      shift_right -= sizeof(Vav)/sizeof(Vav[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 24; j++) {
          if (Vav[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '\''){ // Yod
      shift_right -= sizeof(Yod)/sizeof(Yod[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 10; j++) {
          if (Yod[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == 'z'){ // Zayin
      shift_right -= sizeof(Zayin)/sizeof(Zayin[0]);
      for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 24; j++) {
          if (Zayin[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '.'){ // Dot
      shift_right -= sizeof(dot)/sizeof(dot[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
          if (dot[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_dot_and_comma, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == ','){ // Comma
      shift_right -= sizeof(comma)/sizeof(comma[0]);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 7; j++) {
          if (comma[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + shift_down_for_dot_and_comma, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '!'){ // Exclamation mark
      shift_right -= sizeof(excl)/sizeof(excl[0]);
      for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 24; j++) {
          if (excl[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }

    if (text_to_print.charAt(s) == '?'){ // Question mark
      shift_right -= sizeof(question_mark)/sizeof(question_mark[0]);
      for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 24; j++) {
          if (question_mark[i][j] == 0)
            tft.drawPixel(i + shift_right, j + y + regular_shift_down, font_colors[s % how_many_colors]);
        }
      }
      shift_right -= letter_spacing_pxls;
    }
  }
}
