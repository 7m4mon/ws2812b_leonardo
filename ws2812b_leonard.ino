/*
 * ws2812b_leonard
 * author : 7m4mon
 * date   : 2018-oct-20
 * SDカードに保存されたBMPファイルをWS2812B LEDマトリクスに表示します。
 * SDカードの最大容量は32GBです。
 * 16x16で、データ転送は9ms程度、計算は幅128dotで80ms程度、幅64dotで60ms程度
 * 
 * 最大28,672バイトのフラッシュメモリのうち、スケッチが17,942バイト（62%）を使っています。
 * 最大2,560バイトのRAMのうち、グローバル変数が1,868バイト（72%）を使っていて、ローカル変数で692バイト使うことができます。
 * 
 * PololuLedStrip ライブラリを使用しています。
 * https://github.com/pololu/pololu-led-strip-arduino
 */

#include <PololuLedStrip.h>

#define PIN_SPEED   1    //A1
#define PIN_BRIGHT  0    //A0
#define PIN_WS2812  9    //D9
#define PIN_SDCS    10   //D10

#define BM_TYPE     0x4d42  // "BM"のエンディアン逆"MB"
#define BM_OFFSET   0x36    // defined Windows Bitmap
#define BM_DEPTH    0x20    // 32bit
#define BM_ELEM     4       // A,R,G,B each 8bits

// Create an ledStrip object and specify the pin it will use.
PololuLedStrip<PIN_WS2812> ledStrip;

#define LED_X       16
#define LED_Y       16
#define LED_COUNT   (LED_X*LED_Y)

#define VAR_HYST    4
#define DIMMER_LIMIT 2

/*
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10 (for MKRZero SD: SDCARD_SS_PIN)
*/

#include <SPI.h>
#include <SD.h>

File root, dataFile;

rgb_color colors[LED_COUNT];
int16_t biWidth;
int16_t biHeight;

uint8_t dimmer;

rgb_color ReadPixcel(uint16_t pos, uint8_t dimmer){
    uint8_t r = 0, g = 0, b = 0;

    dataFile.seek(pos);
    // 1byte読む。bgrの順
    b = (dataFile.read() >> dimmer);
    g = (dataFile.read() >> dimmer);
    r = (dataFile.read() >> dimmer);
    return rgb_color(r, g, b);
}


// エンディアンの都合でショート型を読むのにMSB,LSBの考慮が必要
uint16_t read_short(uint16_t index){
    dataFile.seek(index);
    return ( (uint16_t)dataFile.read() | (uint16_t)dataFile.read() << 8);
}

bool check_file(){
    //dump_dataFile();
    print_header();
    if (read_short(0x00) == BM_TYPE &&
        read_short(0x0a) == BM_OFFSET && //bfOffBists オフセット
        read_short(0x1c) == BM_DEPTH){    //32bitのビット深さ
        Serial.println("Format OK");
        return true;
    }else{
        Serial.println("Unsupported Format");
        return false;
    }
}



void print_header(){
    Serial.print(" BM_TYPE :");
    Serial.println(read_short(0x00), HEX);
    Serial.print(" BM_OFFSET :");
    Serial.println(read_short(0x0a), HEX);
    Serial.print(" BM_DEPTH :");
    Serial.println(read_short(0x1c), HEX);
    Serial.print(" GR_STEP :");
    Serial.println(read_short(0x06) + 1, HEX);
    Serial.print(" GR_SPEED :");
    Serial.println(read_short(0x08), HEX);
}


bool check_bmp(){
    //ビットマップの縦幅チェック
    biHeight = read_short(0x16);
    Serial.print("biHeight: ");
    Serial.println(biHeight);
    if(biHeight != LED_Y){
        Serial.println("Error: Missmatch Height");
        return false;
    }
    
    //ビットマップの横幅チェック
    biWidth = read_short(0x12);
    Serial.print("biWidth: ");
    Serial.println(biWidth);
    if(biWidth < LED_X){
        Serial.println("Error: Short Width");
        return false;
    }
    return true;
}

void set_dimmer(uint8_t var){
    if((var & 0x1f) > VAR_HYST){
        dimmer = (var >>5);
    }else{
        //変更しない
    }

    if (dimmer < DIMMER_LIMIT){
        dimmer = DIMMER_LIMIT;
    }  
}

void set_led_buff(uint16_t gx){
    uint16_t pos,i,j,k;
    uint8_t var;
    
    var = (uint8_t) (analogRead(PIN_BRIGHT) >> 2);  //とりあえず、10bit→8bit
    set_dimmer(var);

    // LEDストリップのバッファの中に溜めていく
    for (i = 0; i < LED_Y; i++){
        pos = gx * BM_ELEM + BM_OFFSET;
        pos += BM_ELEM * biWidth * i;
        
        for (j = 0; j < LED_X; j++){
            
            k = (i&1) ? (i+1) * LED_X - j - 1 : i * LED_X + j;
            
            if (dataFile.seek(pos)){
                colors[k] = ReadPixcel(pos,dimmer);
            } else {
                // Fileが終わっている
                colors[k] = rgb_color(0, 0, 0);
            }
            pos += BM_ELEM;
        }
    }
}


void draw(){
  uint16_t gx, gx_step, pause_time, scroll_speed;
  gx_step = read_short(0x06) + 1;       //bfReserved1を画像のスクロールステップとする。
  scroll_speed = read_short(0x08);      //bfReserved2を画像のスクロールスピードとする。
  gx = 0;
  //画像の幅
    while( gx <= (biWidth - LED_X)){
        
        set_led_buff(gx);
        
        // Write the colors to the LED strip.
        ledStrip.write(colors, LED_COUNT);
    
        // スクロール停止時間を待つ
        // ADCからスクロールスピードを読む (右回しきりで最小電圧、最速スクロール）
        pause_time = analogRead(PIN_SPEED);
        
        //初回は5倍の時間だけポーズ。最低500ms待つ。
        if( gx == 0 && gx_step == 1){
            pause_time *= 5;
            pause_time += 500;
        }
        delay(pause_time);
        delay(scroll_speed);
        gx += gx_step;
    }
}

void setup(){
    Serial.begin(9600);
    
    // SDカードを開く
    pinMode(PIN_SDCS, OUTPUT);
    SD.begin(PIN_SDCS);
    root = SD.open("/");
    Serial.println("Opened root Directory");

    dimmer = (uint8_t) (analogRead(PIN_BRIGHT) >> 7);
}

void loop(){
    // ファイルを開く
    dataFile = root.openNextFile();
    if (!dataFile){
        //最後のファイルまで読み取った
        root.rewindDirectory(); 
        Serial.println("rewindDirectory"); 
    }else if(!dataFile.isDirectory()){
        Serial.print("Filename:");
        Serial.println(dataFile.name());
        
        if(check_file() && check_bmp()){
            //ここに処理
            draw();
        }else{
            //何もしない
        }
    }
    //ファイルを閉じる
    dataFile.close();
    Serial.println("File closed");
    Serial.println("***********");
}


