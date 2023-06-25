#include <microLED.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <stdlib.h>
#include "palletes.h"

//НАСТРОЙКИ
//Параметры true или false имеют только два значения. true или 1 - включено: false или 0 - выключено
//Неиспользуемые параметры нужно закомментировать(двойной слеш в начале). Такие параметры ОБЫЧНО не имеют значения
//Некоторые параметры отключить нельзя, например, тип ленты
//Некоторые параметры исключают друг друга, например, яркость в процентах, яркость по току и яркость в ручном режиме

//-------Настройки подключения-------
#define NUM_LEDS 123                  //Количество светодиодов
#define DATA_PIN 13                   //Din пин
#define COLOR_ORDER ORDER_GRB         //Порядок передачи цветов на ленту
#define LED_TYPE LED_WS2812           //Тип ленты(прошивка проверялась только для WS2812)
#define BRIGHTNESS_FC_PIN 14          //Пин резистора/потенциометра для настройки яркости
#define MODES_PIN 15                  //Пин резистора/потенциометра для выбора режима


//------------Управление-------------
#define SWITCH_MODES_FORWARD_BTN 2    //Пин кнопки для выбора эффектов(перемещение вперед)
#define SWITCH_MODES_BACK_BTN 3       //Пин кнопки для выбора эффектов(перемещение назад)
#define INVERT_BRIGHTNESS_FC true     //Инвертировать настройку яркости(через потенциометр)
#define INVERT_MODES true             //Инвертировать порядок эффектов
#define ON_OFF_BTN 4                  //Пин кнопки выключения
#define ONE_CLICK_DELAY 150           //Минимальная задержка для одного клика(в миллисекундах)
#define LCD1602_I2C                   //Включить дисплей(LCD1602, подключение по I2C)

//--------------Яркость--------------
//#define BRIGHTNESS_C                //Яркость по току
  #define BRIGHTNESS_CmA 2500         //Ток для яркости(миллиамперы)
  #define BRIGHTNESS_PER_DIODE 20     //Ток в миллиамперах на каждый светодиод(имеется ввиду на один элемент)
  #define BRIGHTNESS 50               //Яркость вручную в процентах

#define BRIGHTNESS_FC                 //Регулировка яркости резистором/потенциометром

//----------Персонализация-----------
#define DEFAULT_COLOR mRGB(31, 0, 31) //Стандартный цвет
//Настройки палитры в файле palletes.h
//Палитра по умолчанию - Rainbow
//Другие палитры применяются только если можно выбрать режим
//Новые палитры создаются в массиве Palletes
//ВСЕ палитры должны иметь ровно 7 цветов в RGB формате
//Также в массиве palletesNames должны храниться названия палитр
//Названия палитр - не более 10 символов, а также не должны содержать спецсимволов(ограничения для сборок с дисплеем lcd16xx, иначе вывод будет некорректный)

//Настройка палитры для эффектов
#define MEGARAINBOW_SPEED 8           //Скорость эффекта большая радуга
#define BLINKCOLORS_SPEED 3           //Скорость эффекта мигающие цвета

byte effect = 0; //Эффект на старте

//Не трогать
#define BUILD "b130623a" //Beta A 13.06.23
#define effsCount 13

const String effNames[effsCount] = {"Light", "Confetti", "Filling", "Rainbow", "Blink Colors", "HUE Wheel", "Snake", 
                                    "Colored Snake", "Random Filling", "Epilepsy", "Colored Epilepsy", "Police", 
                                    "Double Police"}; //Названия эффектов на дисплее(только на латинице!)

#ifdef BRIGHTNESS_FC
  #define brightnessR analogRead(BRIGHTNESS_FC_PIN)/4
#elif
  #ifdef BRIGHTNESS_C
    #define brightnessR ceil(BRIGHTNESS_CmA/(NUM_LEDS/BRIGHTNESS_PER_DIODE)*100)
  #elif
    #define brightnessR ceil((BRIGHTNESS*255)/100)
  #endif
#endif

#ifdef LCD1602_I2C
  LiquidCrystal_I2C lcd(0x27,16,2);
#endif
microLED<NUM_LEDS, DATA_PIN, MLED_NO_CLOCK, LED_TYPE, COLOR_ORDER, CLI_LOW> strip;

boolean offStrip = false;
short HUEWheelcolor = 0;
boolean loaded = false;
uint32_t btnTimer = 0;
byte brightness = 0;
short mode = 0;
#ifdef LCD1602_I2C
  String modeTitle = "";
  String modeTitleR = "";
#endif
byte page = 0;

void setup(){
  #ifdef BRIGHTNESS_FC
    pinMode(BRIGHTNESS_FC_PIN, INPUT);
  #endif
  #ifdef SWITCH_MODES_FORWARD_BTN
    pinMode(SWITCH_MODES_FORWARD_BTN, INPUT_PULLUP);
  #endif
  #ifdef SWITCH_MODES_BACK_BTN
    pinMode(SWITCH_MODES_BACK_BTN, INPUT_PULLUP);
  #endif
  #ifdef ON_OFF_BTN
    pinMode(ON_OFF_BTN, INPUT_PULLUP);
  #endif
  strip.setBrightness(brightnessR);
  brightness = brightnessR;
  fillColor(showPage());
  strip.show();
  #ifdef LCD1602_I2C
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("WS2812-FXv2");
    lcd.setCursor(4,1);
    lcd.print(BUILD);
  #endif
  
  delay(1500);
  #ifdef LCD1602_I2C
    lcd.clear();
    writeEffName(effNames[0]);
    writeBrightness();
    writeModeTitle();
  #endif
  loaded = true;
  on();
}

void loop() {
  showEffect();
}

void yield(){
  #ifdef BRIGHTNESS_FC
    strip.setBrightness(brightnessR);
  #endif
  //if(digitalRead(SWITCH_MODES_BACK_BTN) == LOW) onLeftClick();
  if (millis() - btnTimer > ONE_CLICK_DELAY) {
    #ifdef SWITCH_MODES_BACK_BTN
      if(digitalRead(SWITCH_MODES_BACK_BTN) == LOW) onLeftClick();
    #endif
    #ifdef SWITCH_MODES_FORWARD_BTN
      if(digitalRead(SWITCH_MODES_FORWARD_BTN) == LOW) onRightClick();
      #ifdef SWITCH_MODES_BACK_BTN
        if(digitalRead(SWITCH_MODES_FORWARD_BTN) == LOW)
          if(digitalRead(SWITCH_MODES_BACK_BTN) == LOW)
            on2Click();
      #endif
    #endif
    if(digitalRead(ON_OFF_BTN) == LOW){
      if(offStrip) enable();
      else disable();
    }
    btnTimer = millis();
  }
  if(brightness != brightnessR && loaded){
    brightness = brightnessR;
    #ifdef LCD1602_I2C
      writeBrightness();
    #endif
  }
  #ifdef MODES_PIN
    short modeR = analogRead(MODES_PIN);
    if(int(mode/4) != int(modeR/4) && loaded){
      mode = modeR;
    }
    if(modeTitle != modeTitleR){
      modeTitle = modeTitleR;
      writeModeTitle();
    }
  #endif
}

void off(){
  strip.fill(mRGB(0,0,0));
  strip.show();
  delay(1);
}

void move(short index){
  mData leds[NUM_LEDS] = strip.leds;
  for(short i = index; i < NUM_LEDS; i++) strip.leds[i] = leds[i-index];
}

mData showPage(){
  switch(page){
    case 0: return mRGB(0, 0, 100);
    case 1: return mRGB(0, 100, 0);
  }
  return mRGB(0, 0, 0);
}

//Эффекты 
void on(){//0
  #ifdef MODES_PIN
    if(mode * 1.525 < 35){
      strip.fill(mRGB(255, 255, 255));
      #ifdef LCD1602_I2C
        modeTitleR = "White";
      #endif
    }else{
      #ifdef LCD1602_I2C
        modeTitleR = "HUE " + String(int(mode*1.525-35));
      #endif
      strip.fill(mWheel(mode*1.525-35, 255));
    }
  #else
    strip.fill(mRGB(255, 255, 255));
  #endif
  strip.show();
  delay(1);
}

void confetti(){ //1
  for(int i = 0; i < NUM_LEDS; i++){
    byte r = rand()%7;
    #ifdef MODES_PIN
      setPixel(i, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][r][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][r][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][r][2]);
      #ifdef LCD1602_I2C
        modeTitleR = PalletesNames[short(mode/(1023/(PALLETES_COUNT-1)))];
      #endif
    #else
      setPixel(i, Palletes[0][r][0], Palletes[0][r][1], Palletes[0][r][2]);
    #endif
  }
  strip.show();
  delay(50);
}

void rainbow(){ //2
  for(int i = 0; i < NUM_LEDS; i++){
    #ifdef MODES_PIN
      setPixel(i, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][i%7][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][i%7][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][i%7][2]);
      #ifdef LCD1602_I2C
        modeTitleR = PalletesNames[short(mode/(1023/(PALLETES_COUNT-1)))];
      #endif
    #else
      setPixel(i, Palletes[0][i%7][0], Palletes[0][i%7][1], Palletes[0][i%7][2]);
    #endif
    strip.show();
    if(effect != 2) {strip.fill(mRGB(0,0,0));break;}
    delay(20);
  }
  for(int i = 0; i < NUM_LEDS; i++){
    strip.leds[i] = DEFAULT_COLOR;
    strip.show();
    if(effect != 2) {strip.fill(mRGB(0,0,0));break;}
    delay(20);
  }
}

void megaRainbow(){ //3
  int h = 0;
  for(int _ = 0; _ < 255/MEGARAINBOW_SPEED; _++){
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.leds[i] = mHSV(h+i*255/NUM_LEDS, 255, 255);
    }
    h+=MEGARAINBOW_SPEED;
    strip.show();
    if(effect != 3) {strip.fill(mRGB(0,0,0));break;}
    delay(20);
  }
}

void blinkColors(){ //4
  for(int color = 0; color < 7; color++){
    for(int i = 255/BLINKCOLORS_SPEED; i > 0; i--){
      #ifdef MODES_PIN
        strip.fill(mRGB(Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][color][0]*i/(255/BLINKCOLORS_SPEED), Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][color][1]*i/(255/BLINKCOLORS_SPEED), Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][color][2]*i/(255/BLINKCOLORS_SPEED)));
        #ifdef LCD1602_I2C
          modeTitleR = PalletesNames[short(mode/(1023/(PALLETES_COUNT-1)))];
        #endif
      #else
        strip.fill(i, Palletes[0][color][0]*i/(255/BLINKCOLORS_SPEED), Palletes[0][color][1]*i/(255/BLINKCOLORS_SPEED), Palletes[0][color][2]*i/(255/BLINKCOLORS_SPEED));
      #endif
      strip.show();
      delay(1/BLINKCOLORS_SPEED);
      yield();
      if(effect != 4) {strip.fill(mRGB(0,0,0));break;}
    }
    yield();
    if(effect != 4) {strip.fill(mRGB(0,0,0));break;}
  }
}

void HUE(){//5
  short speed = 16;
  byte iColor = rand()%7;
  for(int i = 0; i < NUM_LEDS; i++){
    while(HUEWheelcolor > 1530)
      HUEWheelcolor -= 1500;
    strip.leds[i] = mWheel(HUEWheelcolor, 255);
    strip.show();
    if(effect != 5) {strip.fill(mRGB(0,0,0));break;}
    #ifdef MODES_PIN
      #ifdef LCD1602_I2C
        modeTitleR = "Speed " + String(int(mode/32));
      #endif
      speed = ceil(mode/32)+1;
    #endif
    delayMicroseconds(16000/speed);
  }
  HUEWheelcolor += 90;
  for(int i = NUM_LEDS-1; i >= 0; i--){
    while(HUEWheelcolor > 1530)
      HUEWheelcolor -= 1500;
    strip.leds[i] = mWheel(HUEWheelcolor, 255);
    strip.show();
    if(effect != 5) {strip.fill(mRGB(0,0,0));break;}
    delay(1);
  }
  HUEWheelcolor += 90;
}

void snake(){//6
  for(int i = 0; i < NUM_LEDS; i++){
    strip.leds[i] = mRGB(0,255,0);
    if(i > 0)setPixel(i-1,0,230,0);
    if(i > 1)setPixel(i-2,0,205,0);
    if(i > 2)setPixel(i-3,0,180,0);
    if(i > 3)setPixel(i-4,0,155,0);
    if(i > 4)setPixel(i-5,0,130,0);
    if(i > 5)setPixel(i-6,0,105,0);
    if(i > 6)setPixel(i-7,0,80,0);
    if(i > 7)setPixel(i-8,0,55,0);
    if(i > 8)setPixel(i-9,0,30,0);
    if(i > 9)setPixel(i-10,0,5,0);
    if(i > 10)setPixel(i-11,0,0,0);
    strip.show();
    if(effect != 6) {strip.fill(mRGB(0,0,0));yield();break;}
    delay(30);
    
  }
}

void snakeRainbow(){//7
  strip.fill(mRGB(0,0,0));
  for(int i = 0; i < NUM_LEDS; i++){
    #ifdef MODES_PIN
      setPixel(i, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][0][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][0][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][0][2]);
      if(i > 0)setPixel(i-1, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][0][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][0][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][0][2]);
      if(i > 1)setPixel(i-2, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][1][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][1][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][1][2]);
      if(i > 2)setPixel(i-3, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][1][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][1][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][1][2]);
      if(i > 3)setPixel(i-4, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][2][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][2][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][2][2]);
      if(i > 4)setPixel(i-5, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][2][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][2][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][2][2]);
      if(i > 5)setPixel(i-6, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][3][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][3][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][3][2]);
      if(i > 6)setPixel(i-7, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][3][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][3][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][3][2]);
      if(i > 7)setPixel(i-8, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][4][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][4][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][4][2]);
      if(i > 8)setPixel(i-9, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][4][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][4][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][4][2]);
      if(i > 9)setPixel(i-10, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][5][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][5][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][5][2]);
      if(i > 10)setPixel(i-11, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][5][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][5][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][5][2]);
      if(i > 11)setPixel(i-12, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][6][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][6][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][6][2]);
      if(i > 12)setPixel(i-13, Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][6][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][6][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][6][2]);
      if(i > 13)setPixel(i-14,0,0,0);
      #ifdef LCD1602_I2C
        modeTitleR = PalletesNames[short(mode/(1023/(PALLETES_COUNT-1)))];
      #endif
    #else
      setPixel(i,Palletes[0][0][0],Palletes[0][0][1],Palletes[0][0][2]);
      if(i > 0)setPixel(i-1,Palletes[0][0][0],Palletes[0][0][1],Palletes[0][0][2]);
      if(i > 1)setPixel(i-2,Palletes[0][1][0],Palletes[0][1][1],Palletes[0][1][2]);
      if(i > 2)setPixel(i-3,Palletes[0][1][0],Palletes[0][1][1],Palletes[0][1][2]);
      if(i > 3)setPixel(i-4,Palletes[0][2][0],Palletes[0][2][1],Palletes[0][2][2]);
      if(i > 4)setPixel(i-5,Palletes[0][2][0],Palletes[0][2][1],Palletes[0][2][2]);
      if(i > 5)setPixel(i-6,Palletes[0][3][0],Palletes[0][3][1],Palletes[0][3][2]);
      if(i > 6)setPixel(i-7,Palletes[0][3][0],Palletes[0][3][1],Palletes[0][3][2]);
      if(i > 7)setPixel(i-8,Palletes[0][4][0],Palletes[0][4][1],Palletes[0][4][2]);
      if(i > 8)setPixel(i-9,Palletes[0][4][0],Palletes[0][4][1],Palletes[0][4][2]);
      if(i > 9)setPixel(i-10,Palletes[0][5][0],Palletes[0][5][1],Palletes[0][5][2]);
      if(i > 10)setPixel(i-11,Palletes[0][5][0],Palletes[0][5][1],Palletes[0][5][2]);
      if(i > 11)setPixel(i-12,Palletes[0][6][0],Palletes[0][6][1],Palletes[0][6][2]);
      if(i > 12)setPixel(i-13,Palletes[0][6][0],Palletes[0][6][1],Palletes[0][6][2]);
      if(i > 13)setPixel(i-14,0,0,0);
    #endif
    strip.show();
    if(effect != 7) {strip.fill(mRGB(0,0,0));yield();break;}
    delay(25);
  }
}

void randomFilling(){//8
  move(1);
  if(rand()%2){
    strip.leds[0] = mBlack;
  }
  else{
    #ifdef MODES_PIN
      if(mode * 1.525 < 35){
        strip.leds[0] = mWhite;
        #ifdef LCD1602_I2C
          modeTitleR = "White";
        #endif
      }else{
        #ifdef LCD1602_I2C
          modeTitleR = "HUE " + String(int(mode*1.525-35));
        #endif
        strip.leds[0] = mWheel(mode*1.525-35, 255);
      }
    #else
      strip.leds[0] = mWhite;
    #endif
  }
  strip.show();
  delay(10);
  if(effect != 8) {strip.fill(mRGB(0,0,0));}
}

void epilepsy(){//9
  strip.fill(mRGB(255,255,255));
  strip.show();
  delay(25);
  strip.fill(mRGB(0,0,0));
  strip.show();
  delay(25);
  if(effect != 9) {strip.fill(mRGB(0,0,0));}
}

void epilepsyRGB(){//10
  byte r = rand()%7;
  #ifdef MODES_PIN
    strip.fill(mRGB(Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][r][0], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][r][1], Palletes[short(mode/(1023/(PALLETES_COUNT-1)))][r][2]));
    #ifdef LCD1602_I2C
      modeTitleR = PalletesNames[short(mode/(1023/(PALLETES_COUNT-1)))];
    #endif 
  #else
    strip.fill(mRGB(Palletes[0][r][0], Palletes[0][r][1], Palletes[0][r][2]));
  #endif
  strip.show();
  delay(25);
  strip.fill(mRGB(0,0,0));
  strip.show();
  delay(25);
  if(effect != 10) {off();strip.fill(mRGB(0,0,0));}
}

void police(){//11
  strip.fill(mRGB(255,0,0));
  if(effect != 11) {strip.fill(mRGB(0,0,0));return;}
  strip.show();
  delay(50);
  strip.fill(mRGB(0,0,255));
  strip.show();
  if(effect != 11) {strip.fill(mRGB(0,0,0));return;}
  delay(50);
}

void Dpolice(){//12
  strip.fill(mRGB(255,0,0));
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  strip.show();
  delay(25);
  off();
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  delay(25);
  strip.fill(mRGB(255,0,0));
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  strip.show();
  delay(25);
  off();
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  delay(25);
  strip.fill(mRGB(0,0,255));
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  strip.show();
  delay(25);
  off();
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  delay(25);
  strip.fill(mRGB(0,0,255));
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  strip.show();
  delay(25);
  off();
  if(effect != 12) {strip.fill(mRGB(0,0,0));return;}
  delay(25);
}

//Утилиты
void fillColor(mData color){
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.leds[i] = color;
  }
}
void setPixel(short pos,byte r,byte g,byte b){strip.leds[pos]=mRGB(r,g,b);}
  

void showEffect(){
  if(offStrip) off();
  else{
    if(page == 0){
      switch(effect){
        case 0: on();            break;
        case 1: confetti();      break;
        case 2: rainbow();       break;
        case 3: megaRainbow();   break;
        case 4: blinkColors();   break;
        case 5: HUE();           break;
        case 6: snake();         break;
        case 7: snakeRainbow();  break;
        case 8: randomFilling(); break;
        case 9: epilepsy();      break;
       case 10: epilepsyRGB();   break;
       case 11: police();        break;
       case 12: Dpolice();       break;
      }
    }
    if(page == 1){
      strip.fill(showPage());
    }
  }
  yield();
  strip.show();
}

#ifdef LCD1602_I2C
  void writeEffName(String effName){
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print(effName);
  }

  void writeBrightness(){
    lcd.setCursor(0, 1);
    lcd.print("    ");
    lcd.setCursor(0, 1);
    lcd.print(String(brightnessR*100/255));
    lcd.print("%");
  }

  void writeModeTitle(){
    lcd.setCursor(6, 1);
    lcd.print("          ");
    lcd.setCursor(6, 1);
    lcd.print(modeTitle);
  }
#endif

void onLeftClick(){
  if(page == 0){
    if(effect == 0) effect=effsCount-1;
    else effect--;
    #ifdef MODES_PIN
    #ifdef LCD1602_I2C
      modeTitleR = "";
      writeModeTitle();
    #endif
    #endif
    #ifdef LCD1602_I2C
      writeEffName(effNames[effect]);
    #endif
    strip.fill(mRGB(0,0,0));
  }
}

void onRightClick(){
  if(page == 0){
    if(effect == effsCount-1) effect=0;
    else effect++;
    #ifdef MODES_PIN
    #ifdef LCD1602_I2C
      modeTitleR = "";
      writeModeTitle();
    #endif
    #endif
    #ifdef LCD1602_I2C
      writeEffName(effNames[effect]);
    #endif
    strip.fill(mRGB(0,0,0));
  }
}

void on2Click(){
  if(page == 1) page=0;
  else page++;
  loadPage(page);
}

void loadPage(byte pageI){
  fillColor(showPage());
  #ifdef LCD1602_I2C
    lcd.clear();
    lcd.backlight();
  #endif
  page = pageI;
  switch(pageI){
    case 0:
      #ifdef LCD1602_I2C
        writeEffName(effNames[effect]);
        writeBrightness();
        writeModeTitle();
      #endif
      break;
    case 1: settingsPage(); break;
  }
}

void settingsPage(){
  #ifdef LCD1602_I2C
    lcd.setCursor(0,0);
    lcd.print("Settings");
  #endif
}

void disable(){
  offStrip = true;
  #ifdef LCD1602_I2C
    lcd.noBacklight();
  #endif
}

void enable(){
  offStrip = false;
  #ifdef LCD1602_I2C
    lcd.backlight();
  #endif
}
