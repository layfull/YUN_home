// Possible commands are listed here:
//
// ir/0/aircon_on --> send aircon on command for ir cannel 0
// ir/2/catv_on   --> send catv on command for ir cannel 2

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Wire.h>

#define PIN_LED 13

// 温度センサー I2C アドレス
int I2CAdrs;

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

// 明るさセンサー接続ポート番号
byte cds[] = {1, 2};

// 赤外線 LED 接続ポート番号
byte ir_out[] = {10, 9, 6, 5};

#define IR_DATA_SIZE 18 /* 送信データ最大長 (エアコンで 18 バイト) */
int ir_t;   // 1 送信単位時間 T (μsec) ... 350〜650μsec
int ir_leader_on;
int ir_leader_off;
int ir_f;   // IR LED 点灯時間 ex) 38kHz ... 5、40kHz ... 4
int ir_f2;  // IR LED 消灯時間 ex) 38kHz ... 10 (デューティ比 1/3 なら、f の二倍)
int ir_len; // 送信コマンド長
byte ir_data[IR_DATA_SIZE]; // 送信コマンド (LSB first)

// Apple TV
// Reader 17T,8T
// Trailer T,74T
void setAppleTV(int cmd) {
  // cmd
  // 0x02 ... menu
  // 0x07 ... right
  // 0x08 ... left
  // 0x0d ... down
  // 0x0b ... up
  // 0x5e ... play/pause
  // 0x5d ... ok
  ir_t = 550;
  ir_leader_on = 17;
  ir_leader_off = 8;
  ir_f = 5;
  ir_f2 = 10;
  ir_len = 4;
  ir_data[0] = 0xee;
  ir_data[1] = 0x87;
  ir_data[2] = cmd;
  ir_data[3] = 0xb2;
}

// 5.1ch
// Reader 16T,8T
// Trailer T,71T
void set5_1ch(int cmd) {
  // cmd
  // 0x01 ... speaker mode
  // 0x02 ... balance
  // 0x03 ... volume up
  // 0x04 ... bass
  // 0x05 ... delay
  // 0x06 ... volume down
  // 0x08 ... input select digital/analog
  // 0x09 ... surround on/off
  // 0x0a ... pro logic auto on
  // 0x12 ... power on/off
  // 0x1a ... test
  // 0x1e ... mute
  // 0x1f ... reset
  ir_t = 562;
  ir_leader_on = 16;
  ir_leader_off = 8;
  ir_f = 5;
  ir_f2 = 10;
  ir_len = 4;
  ir_data[0] = 0x86;
  ir_data[1] = 0x6b;
  ir_data[2] = cmd;
  ir_data[3] = 0xff ^ cmd;
}

// iBUFFALO BSAK302 HDMI Switch
// Leader 16T,8T
// Trailer T,71T
void setHDMI(int cmd) {
  // cmd
  // 0x4 ... HDMI1
  // 0x8 ... HDMI2
  // 0xa ... HDMI3
  ir_t = 562;
  ir_leader_on = 16;
  ir_leader_off = 8;
  ir_f = 5;
  ir_f2 = 10;
  ir_len = 4;
  ir_data[0] = 0x00;
  ir_data[1] = 0xff;
  ir_data[2] = cmd;
  ir_data[3] = 0xf0 + (0xf ^ cmd);
}

// 三菱霧ヶ峰
void setAircon(int cmd) {
  // 家電協フォーマット
  ir_t = 425;
  ir_leader_on = 8;
  ir_leader_off = 4;
  ir_f = 5;
  ir_f2 = 10;
  ir_len = 18;
  ir_data[0] = 0x23;
  ir_data[1] = 0xcb;
  ir_data[2] = 0x26;
  ir_data[3] = 0x01;
  ir_data[4] = 0x00;
  if (cmd == 1) {
    ir_data[5] = 0x20;
  } else {
    ir_data[5] = 0x00;
  }
  ir_data[6] = 0x48;
  ir_data[7] = 0x0c;
  ir_data[8] = 0x00;
  ir_data[9] = 0x40;
  ir_data[10] = 0x00;
  ir_data[11] = 0x00;
  ir_data[12] = 0x00;
  ir_data[13] = 0x00;
  ir_data[14] = 0x10;
  ir_data[15] = 0x03;
  ir_data[16] = 0x02;
  if (cmd == 1) {
    ir_data[17] = 0xde;
  } else {
    ir_data[17] = 0xbe;
  }
}

// タキズミ照明
void setTakizumi() {
  ir_t = 562;
  ir_leader_on = 16;
  ir_leader_off = 8;
  ir_f = 5;
  ir_f2 = 10;
  ir_len = 4;
  ir_data[0] = 0x86;
  ir_data[1] = 0x23;
  ir_data[2] = 0x00;
  ir_data[3] = 0xff;
}

// Panasonic TZ-DCH2000
// Reader 8T,4T
void setPanasonic(int cmd) {
  ir_t = 425;
  ir_leader_on = 8;
  ir_leader_off = 4;
  ir_f = 4;
  ir_f2 = 7;
  ir_len = 6;
  ir_data[0] = 0x02;
  ir_data[1] = 0x20;
  ir_data[2] = 0x80;
  ir_data[3] = 0x26;
  ir_data[4] = cmd;
  ir_data[5] = ir_data[2] ^ ir_data[3] ^ ir_data[4];
}

// SHARP AQUOS 45V
// Reader 8T,4T
// Trailer T,11T
void setAquos(int cmd) {
  ir_t = 425;
  ir_leader_on = 8;
  ir_leader_off = 4;
  ir_f = 5;
  ir_f2 = 10;
  ir_len = 6;
  ir_data[0] = 0xaa;
  ir_data[1] = 0x5a;
  ir_data[2] = 0x8f;
  ir_data[3] = 0x12;
  ir_data[4] = cmd & 0xff;
  ir_data[5] = ((ir_data[2] & 0xf0)
    ^ (ir_data[3] & 0xf0) ^ ((ir_data[3] & 0xf) << 4)
    ^ (ir_data[4] & 0xf0) ^ ((ir_data[4] & 0xf) << 4))
    ^ ((cmd >> 4) & 0xf0) + (cmd >> 8);
}

void setup() {
  I2CAdrs = 0x48;
  Serial.begin(9600);
  pinMode(PIN_LED, OUTPUT);
  pinMode(ir_out[0], OUTPUT);
  pinMode(ir_out[1], OUTPUT);
  pinMode(ir_out[2], OUTPUT);
  pinMode(ir_out[3], OUTPUT);
  Wire.begin();       // マスタ初期化

  // Bridge startup
  digitalWrite(PIN_LED, LOW);
  Bridge.begin();
  digitalWrite(PIN_LED, HIGH);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  delay(50); // Poll every 50ms
}

void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');

       if (command == F("sensor")) { sensorCommand(client); }
  else if (command == F("temp"  )) { tempCommand(client);   }
  else if (command == F("cds"   )) { cdsCommand(client);    }
  else if (command == F("ir"    )) { irCommand(client);     }
  else {
    client.print(F("Error: "));
    client.print(command);
  }
}

float getTemp() {
  uint16_t val;
  float tmp;
  int ival;

  // ワンショット リトリガ再設定
  Wire.beginTransmission(I2CAdrs);  // S.C発行,CB送信
  Wire.write(0x03);                 // Configuration register 選択
  Wire.write(0x20);                 // one-shot mode 設定
  Wire.endTransmission();           // ストップ・コンディション
  delay(240);

  Wire.requestFrom(I2CAdrs, 2);     // S.C発行,CB送信
  val = (uint16_t)Wire.read() << 8; // データの読み出し(上位)
  val |= Wire.read();               // データの読み出し(下位)

  val >>= 3;

  ival = (int)val;
  if(val & (0x8000 >> 3)) {         // 符号判定
    // 負数
    ival = ival  - 8192;
  }

  tmp = (float)ival / 16.0;
  return tmp;
}

void sensorCommand(YunClient client) {
  client.parseInt();
  client.print(F("Temperature = "));
  client.print(getTemp());
  client.println(" C");

  // 明るさ
  int light = analogRead(cds[0]);
  client.print(F("Brightness = "));
  client.println(light);
  client.print(F(" (Aircon "));
  if (light > 750) {
    client.print(F("OFF)"));
  } else {
    client.print(F("ON)"));
  }
}

void ir_print(YunClient client) {
  client.print(F("T = "));
  client.print(ir_t);
  client.print(F("usec, Leader = "));
  client.print(ir_leader_on);
  client.print(F("T/"));
  client.print(ir_leader_off);
  client.print(F("T, IR = "));
  client.print(ir_f);
  client.print(F("/"));
  client.print(ir_f2);
  client.print(F("usec, length = "));
  client.println(ir_len);
  for (int i = 0; i < ir_len; i++) {
    client.print(ir_data[i], HEX);
    client.print(" ");
  }
}

void tempCommand(YunClient client) {
  client.print(getTemp());
}

void cdsCommand(YunClient client) {
  client.println(analogRead(cds[client.parseInt()]));
}

void rawCommand(YunClient client) {
  for (int i = 0; i < IR_DATA_SIZE; i++) {
    ir_data[i] = client.parseInt();
	if (client.read() == '\r') {
      break;
	}
  }
  for (; i < IR_DATA_SIZE; i++) {
    ir_data[i] = 0;
  }
  for (i = 0; i < IR_DATA_SIZE; i++) {
    client.print(ir_data[i]);
	if (ir_data[i]) == 0) break;
	client.print(",");
  }
}

void irCommand(YunClient client) {
  int i;

  // Read remote command number
  int irn = ir_out[client.parseInt()];
  if (client.read() != '/') {
    client.print(F("Error: no '/'"));
    return;
  }
  String mode = client.readStringUntil('/');
  if (mode == F("raw")) {
  	// arduino/ir/0/raw/3200,1600,400,1200,400,400...400,0
	rawCommand(YunClient client);
	ir_write_raw(irn);
	return;
  }
  String param = client.readStringUntil('\r');
 
  if (mode == F("hdmi")) {
    // iBUFFALO HDMI Switch
         if (param == F("1"      )) setHDMI(0x4);
    else if (param == F("2"      )) setHDMI(0x8);
    else if (param == F("3"      )) setHDMI(0xa);
    else return;
  } else if (mode == F("aircon")) {
    // 三菱エアコン霧ヶ峰
         if (param == F("on"     )) setAircon(1);
    else if (param == F("off"    )) setAircon(0);
    else return;
  } else if (mode == F("51ch")) {
         if (param == F("spmode" )) set5_1ch(0x01);
//  else if (param == F("balance")) set5_1ch(0x02);
    else if (param == F("volup"  )) set5_1ch(0x03);
//  else if (param == F("bass"   )) set5_1ch(0x04);
//  else if (param == F("delay"  )) set5_1ch(0x05);
    else if (param == F("voldown")) set5_1ch(0x06);
    else if (param == F("input"  )) set5_1ch(0x08);
    else if (param == F("surround")) set5_1ch(0x09);
//  else if (param == F("prologic")) set5_1ch(0x0a);
    else if (param == F("power"  )) set5_1ch(0x12);
//  else if (param == F("test"   )) set5_1ch(0x1a);
//  else if (param == F("mute"   )) set5_1ch(0x1e);
//  else if (param == F("reset"  )) set5_1ch(0x1f);
    else return; 
  } else if (mode == F("tv")) {
    // SHARP AQUOS 45V
         if (param == F("power"  )) setAquos(0x116);
    else if (param == F("on"     )) setAquos(0x24a);
    else if (param == F("off"    )) setAquos(0x24b);
    else if (param == F("size"   )) setAquos(0x1d5);
    else if (param == F("volup"  )) setAquos(0x114);
    else if (param == F("voldown")) setAquos(0x115);
    else if (param == F("chup"   )) setAquos(0x111);
    else if (param == F("chdown" )) setAquos(0x112);
    else if (param == F("tv"     )) setAquos(0x279);
    else if (param == F("input1" )) setAquos(0x132);
    else if (param == F("catv2"  )) setAquos(0x132);
    else if (param == F("input2" )) setAquos(0x133);
    else if (param == F("catv"   )) setAquos(0x133);
    else if (param == F("input3" )) setAquos(0x134);
    else if (param == F("input4" )) setAquos(0x1d3);
    else if (param == F("wii"    )) setAquos(0x1d3);
    else if (param == F("input5" )) setAquos(0x24c);
    else if (param == F("xbox"   )) setAquos(0x24c);
    else if (param == F("input6" )) setAquos(0x292);
    else if (param == F("apple"  )) setAquos(0x292);
    else if (param == F("input7" )) setAquos(0x295);
    else if (param == F("blue"   )) setAquos(0x280);
    else if (param == F("red"    )) setAquos(0x281);
    else if (param == F("green"  )) setAquos(0x282);
    else if (param == F("yellow" )) setAquos(0x283);
    else if (param == F("1"      )) setAquos(0x24e);
//  else if (param == F("2"      )) setAquos(0x24f);
//  else if (param == F("3"      )) setAquos(0x250);
//  else if (param == F("4"      )) setAquos(0x251);
//  else if (param == F("5"      )) setAquos(0x252);
//  else if (param == F("6"      )) setAquos(0x253);
//  else if (param == F("7"      )) setAquos(0x254);
//  else if (param == F("8"      )) setAquos(0x255);
//  else if (param == F("9"      )) setAquos(0x256);
//  else if (param == F("10"     )) setAquos(0x257);
//  else if (param == F("11"     )) setAquos(0x258);
//  else if (param == F("12"     )) setAquos(0x259);
    else if (param == F("guide"  )) setAquos(0x260);
    else return;
  } else if (mode == F("light")) {
    // タキズミ照明
         if (param == F("power")) setTakizumi();
    else return;
  } else if (mode == F("apple")) {
    // Apple TV
         if (param == F("menu"   )) setAppleTV(0x02);
    else if (param == F("right"  )) setAppleTV(0x07);
    else if (param == F("left"   )) setAppleTV(0x08);
    else if (param == F("down"   )) setAppleTV(0x0d);
    else if (param == F("up"     )) setAppleTV(0x0b);
    else if (param == F("play"   )) setAppleTV(0x5e);
    else if (param == F("ok"     )) setAppleTV(0x5d);
    else return;
  } else if (mode == F("catv")) {
    // Panasonic TZ-DCH2000
         if (param == F("back"   )) setPanasonic(0x52);
    else if (param == F("ok"     )) setPanasonic(0x53);
    else if (param == F("menu"   )) setPanasonic(0x54);
    else if (param == F("guide"  )) setPanasonic(0x55);
    else if (param == F("up"     )) setPanasonic(0x5a);
    else if (param == F("down"   )) setPanasonic(0x5b);
    else if (param == F("left"   )) setPanasonic(0x5d);
    else if (param == F("right"  )) setPanasonic(0x5e);
//  else if (param == F("1"      )) setPanasonic(0x60);
//  else if (param == F("2"      )) setPanasonic(0x61);
//  else if (param == F("3"      )) setPanasonic(0x62);
//  else if (param == F("4"      )) setPanasonic(0x63);
//  else if (param == F("5"      )) setPanasonic(0x64);
//  else if (param == F("6"      )) setPanasonic(0x65);
//  else if (param == F("7"      )) setPanasonic(0x66);
//  else if (param == F("8"      )) setPanasonic(0x67);
//  else if (param == F("9"      )) setPanasonic(0x68);
//  else if (param == F("10"     )) setPanasonic(0x69);
    else if (param == F("chidegi")) setPanasonic(0x6a);
//  else if (param == F("bs"     )) setPanasonic(0x6b);
    else if (param == F("catv"   )) setPanasonic(0x6c);
//  else if (param == F("11"     )) setPanasonic(0x6f);
//  else if (param == F("12"     )) setPanasonic(0x70);
    else if (param == F("chup"   )) setPanasonic(0x74);
    else if (param == F("chdown" )) setPanasonic(0x75);
    else if (param == F("lang"   )) setPanasonic(0x80);
    else if (param == F("help"   )) setPanasonic(0x85);
    else if (param == F("submenu")) setPanasonic(0x87);
    else if (param == F("on_off" )) setPanasonic(0x8d);
    else if (param == F("power"  )) setPanasonic(0x8d);
    else if (param == F("blue"   )) setPanasonic(0x91);
    else if (param == F("red"    )) setPanasonic(0x92);
    else if (param == F("green"  )) setPanasonic(0x93);
    else if (param == F("yellow" )) setPanasonic(0x94);
    else if (param == F("reclist")) setPanasonic(0xde);
    else if (param == F("navi"   )) setPanasonic(0xde);
    else if (param == F("stop"   )) setPanasonic(0xe0);
    else if (param == F("puase"  )) setPanasonic(0xe1);
    else if (param == F("play"   )) setPanasonic(0xe2);
    else if (param == F("30sec"  )) setPanasonic(0xe8);
    else return;
  } else if (mode == F("byte")) {
    // arduino/ir/0/byte/tttl1lff2xxxxxxxxxxxx
    // 0 ..... 赤外線チャネル
    // ttt ... T (usec)
    // l1 .... リーダー ON
    // l ..... リーダー OFF
    // f ..... 発光時間 (usec)
    // f2 .... 消灯時間 (usec)
    // xx .... 送信データ
    ir_t = (param[0] - '0') * 100 + (param[1] - '0') * 10 + (param[2] - '0');
    ir_leader_on = (param[3] - '0') * 10 + (param[4] - '0');
    ir_leader_off = (param[5] - '0');
    ir_f = (param[6] - '0');
    ir_f2 = (param[7] - '0') * 10 + (param[8] - '0');
    ir_len = (param.length() - 9) / 2;
    client.println(F("Custom Data"));
    for (int i = 0; i < ir_len; i++) {
      int j = param[i * 2 + 9] - '0';
      if (j > 9) {
        j = param[i * 2 + 9] - 'a' + 10;
      }
      ir_data[i] = j * 16;
      j = param[i * 2 + 10] - '0';
      if (j > 9) {
        j = param[i * 2 + 10] - 'a' + 10;
      }
      ir_data[i] += j;
    }
  } else {
    client.print(F("Error: "));
    client.println(mode);
    return;
  }
  ir_print(client);
  ir_write(irn);
}

void ir_write(byte ir_pin) {
  unsigned int i, j, m;
  unsigned long ed;

  ed = micros() + ir_t * ir_leader_on;
  while (micros() < ed) {
    digitalWrite(ir_pin, HIGH);
    delayMicroseconds(ir_f);
    digitalWrite(ir_pin, LOW);
    delayMicroseconds(ir_f2);
  }
  delayMicroseconds(ir_t * ir_leader_off);

  for (i = 0; i < ir_len; i++) {
    m = 1;
    for (j = 0; j < 8; j++) {
      ed = micros() + ir_t;
      while (micros() < ed) {
        digitalWrite(ir_pin, HIGH);
        delayMicroseconds(ir_f);
        digitalWrite(ir_pin, LOW);
        delayMicroseconds(ir_f2);
      }
      if ((ir_data[i] & m) == 0) {
        delayMicroseconds(ir_t);
      } else {
        delayMicroseconds(ir_t * 3);
      }
      m <<= 1;
    }
  }

  ed = micros() + ir_t;
  while (micros() < ed) {
    digitalWrite(ir_pin, HIGH);
    delayMicroseconds(ir_f);
    digitalWrite(ir_pin, LOW);
    delayMicroseconds(ir_f2);
  }
  delayMicroseconds(ir_t * 80);
}

void ir_write_raw(byte ir_pin){
  unsigned int i;
  unsigned long interval_sum, start_at;
  interval_sum = 0;
  start_at = micros();
  for(i = 0; ir_data[i] > 0; i++){
    interval_sum += ir_data[i];
    if(i % 2 == 0){
      while(micros() - start_at < interval_sum){
        digitalWrite(ir_pin, HIGH);
        delayMicroseconds(6);
        digitalWrite(ir_pin, LOW);
        delayMicroseconds(8);
      }
    }
    else{
      while(micros() - start_at < interval_sum);
    }
  }
}

