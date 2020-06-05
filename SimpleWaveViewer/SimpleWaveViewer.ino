#include <M5StickC.h>

/**
 * M5Stick-C 用 AD 入力(G26 ポート) の波形表示
 * 
 * @see https://github.com/botofancalin/M5Stack-ESP32-Oscilloscope/blob/master/M5Stack_Oscilloscope/M5Stack_Oscilloscope.ino
 */
 
// サンプリングレート (us) 1000000(us)=1(s) , 6(ms) = 6000
const int SAMPLING_RATE = 6000;
// サンプリングするポ０と
const int AD_CHANNEL = 26;// G26
// LCD のサイズ
const int16_t LCD_WIDTH = 80;
const int16_t LCD_HEIGHT = 160;
// 画面の表示位置
const int16_t START_X = 10;
const int16_t START_Y = 5;
const int16_t END_X = LCD_HEIGHT;
const int16_t END_Y = LCD_WIDTH;
const int16_t START_Y_ZERO = 0;
const int16_t MAX_Y = END_Y - (START_Y + START_Y_ZERO);
// データバッファサイズ
const int16_t bufferSize = END_X-START_X;
// データバッファ
int16_t adBuffers[bufferSize];
// データバッファのリードポインタ
int16_t readBufferPointer = 0;
// データバッファのライトポインタ
int16_t writeBufferPointer = 0;

// 割り込み関係
// @see https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
// @see http://marchan.e5.valueserver.jp/cabin/comp/jbox/arc202/doc21105.html
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

/**
 * AD 入力のデータを取得してグラフ表示用に正規化をする
 * 正規化したデータを返す
 * 
 */
int16_t normailizeAnalogRead()
{
  int16_t value = analogRead(AD_CHANNEL);
  return ((value * MAX_Y) / 1024);
}

/**
 * タイマ割り込み処理
 * 
 */
void IRAM_ATTR onTimer() 
{
  portENTER_CRITICAL_ISR(&timerMux);
  adBuffers[writeBufferPointer++] = normailizeAnalogRead();
  if( writeBufferPointer >= bufferSize ) 
  {
      writeBufferPointer = 0;
  }
  portEXIT_CRITICAL_ISR(&timerMux);
}

/**
 * 縦軸の破線描写処理
 * @var y
 */
void drwaDashLineY(int16_t y)
{
  const int16_t t = 3;
  for(  int16_t x = START_X ; x <= END_X ; x=x+t*2 )
  {
    M5.Lcd.drawLine(y,x,y,x+t,WHITE);
  }
}

/**
 * 横軸の破線描写処理
 * @var x
 */
void drwaDashLineX(int16_t x)
{
  const int16_t t = 3;
  for(  int16_t y = START_Y ; y <= END_Y ; y=y+t*2 )
  {
    M5.Lcd.drawLine(y,x,y+t,x,WHITE);
  }
}

/**
 * グリッドの描写処理
 * 
 */
void drawGrid() 
{
  const int16_t divX = 10,divY = (END_Y - (START_Y + START_Y_ZERO)) / 3;
  short x,y;
  // Zero line
  M5.Lcd.drawLine(START_Y+START_Y_ZERO,START_X,START_Y+START_Y_ZERO,END_X,WHITE);
  // 垂直線
  M5.Lcd.drawLine(START_Y,START_X,LCD_WIDTH,START_X,WHITE);
  //
  for( x = START_X + divX; x <= END_X ; x=x+divX )
  {
    drwaDashLineX(x);
  }
  for( y = START_Y + divY ; y <= END_Y ; y=y+divY )
  {
    drwaDashLineY(y);
  }
}

/**
 * データの描写処理
 * 
 */
void drawData() 
{
  int16_t y,size;
  if( (writeBufferPointer - readBufferPointer) != 0 )
  {
    if( writeBufferPointer > readBufferPointer )
    {
      size = writeBufferPointer - readBufferPointer;
    }
    else
    {
      size = (bufferSize - readBufferPointer) + writeBufferPointer;
      
    }
      for( int16_t count = 0 ; count < size ; count++ )
      {
        y = adBuffers[readBufferPointer];
        M5.Lcd.drawPixel( START_Y+START_Y_ZERO+y, START_X + readBufferPointer, RED);
        readBufferPointer++;
        if( readBufferPointer >= bufferSize )
        {
          M5.Lcd.fillScreen(BLACK); // 画面全体を黒塗り
          drawGrid();
          readBufferPointer = 0;
        }
      }
  }
  
}

/**
 * 初期化処理
 * 
 */
void setup() {
  // put your setup code here, to run once:
  // 
  M5.begin();
  M5.Lcd.fillScreen(BLACK); // 画面全体を黒塗り

  memset(&adBuffers[0],0,bufferSize);
  
  timer = timerBegin(0, 80, true); // 1us 毎 
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, SAMPLING_RATE, true); // SAMPLING_RATE us毎にonTimer 関数を呼び出す設定
  timerAlarmEnable(timer);

  M5.Lcd.fillScreen(BLACK); // 画面全体を黒塗り
  drawGrid();
}

/**
 * ループ処理
 * 
 */
void loop() {
  drawData();
}
