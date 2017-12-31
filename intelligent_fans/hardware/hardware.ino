#include <Adafruit_NeoPixel.h>//led
#include <IRremote.h>//红外
#include <U8glib.h>//oled
#include <Wire.h>//调用收发数据所使用的库函数
#include <SHT2x.h>//请使用老版温度传感器代码
#include "ESP8266.h"
#include "I2Cdev.h"
#include <SoftwareSerial.h>
#define INTERVAL_LCD 20 //定义OLED刷新时间间隔 
unsigned long lcd_time = millis(); //OLED刷新时间计时器
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE); //设置OLED型号 
#define setFont_L u8g.setFont(u8g_font_7x13)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, 12, NEO_GRB + NEO_KHZ800); 
//该函数第一个参数控制串联灯的个数，第二个是控制用哪个pin脚输出，第三个显示颜色和变化闪烁频率
int RECV_PIN = 10;   //红外线接收器OUTPUT端接在pin 10
IRrecv irrecv(RECV_PIN);  //定义IRrecv对象来接收红外线信号
decode_results results;   //解码结果放在decode_results构造的对象results里
int wake=0;
int up=0;
int mode=0;
int value=0;
int tem1=20;//档位
int tem2=23;
int tem3=25;
int tem4=28;
int tem5=30;
SoftwareSerial mySerial(2, 3);  // 对于Core必须使用软串口进行WIFI模块通信
#define esp8266Serial mySerial   // 定义WIFI模块通信串口
#define SSID        "FS"//WiFi名称
#define PASSWORD    "qwertyui"//WiFi密码
ESP8266 wifi(mySerial);
char serialbuffer[1000];  //url储存
String dataToSend;  //AT指令储存
String startcommand;
String sendcommand;
char buf[10];
String dataToRead=""; //指令读取
char cmd='a';
String jsonToSend;
float sensor_tem, sensor_hum;
char  sensor_tem_c[7], sensor_hum_c[7] ;
char *postArray;
String postString;
int motorcontrol()    //手动切换电机速度
{
   if (irrecv.decode(&results)) //红外控制
  {
    if(results.value==0x1FEA05F)
    {
      value += 50;
      if (value > 250)
      value = 250;
    }
    else if(results.value==0x1FED827)
    {
       value -= 50;
    if (value < 0)
      value = 0;
    }
      }else if(cmd=='1')value=50;//WiFi信号控制
      else if(cmd=='2')value=100;
      else if(cmd=='3')value=150;
      else if(cmd=='4')value=200;
      else if(cmd=='5')value=250;
      analogWrite(6, value);
    digitalWrite(8, LOW);
}
int automoto()    //自动切换电机速度
{
  if(sensor_tem>=tem1&&sensor_tem<tem2)value=50;
  if(sensor_tem>=tem2&&sensor_tem<tem3)value=100;
  if(sensor_tem>=tem3&&sensor_tem<tem4)value=150;    
  if(sensor_tem>=tem4&&sensor_tem<tem5)value=200;
  if(sensor_tem>=tem5)value=250;
    analogWrite(6, value);
    digitalWrite(8, LOW);
}
void setup(){
  Serial.begin(9600);
  Wire.begin();
  strip.begin();   //准备对灯珠进行数据发送
  irrecv.enableIRIn(); // 启动红外解码
  pinMode(6, OUTPUT);//电机端口
  pinMode(8, OUTPUT);
  //
 esp8266Serial.begin(9600);//connection to ESP8266
  Serial.begin(115200);//rial debug
  while (!Serial); // wait for Leonardo enumeration, others continue immediately
  
wifi.setOprToStationSoftAP();//WiFi初始化
wifi.joinAP(SSID, PASSWORD);
wifi.enableMUX();
wifi.startTCPServer(8090);
wifi.setTCPServerTimeout(10);

  }
  

void loop()   //无返回值loop函数
 {
uint8_t buffer[128] = {0};
  uint8_t mux_id;
  uint32_t len = wifi.recv(&mux_id, buffer, sizeof(buffer), 100);//接受WiFi控制信号
    cmd=(char)buffer[11];
delay(500);
if (irrecv.decode(&results))if(results.value==0x1FE48B7){wake=1;up++;}//红外唤醒
if(cmd=='i'){wake=1;}//WiFi唤醒
if(up%2==0)if(results.value==0x1FE48B7)wake=0;//红外睡眠
if(cmd=='o')wake=0;//WiFi睡眠
irrecv.resume();//红外刷新
    if(!wake){
     strip.setPixelColor(0, strip.Color(255, 0, 0));//红光
  strip.show();   //LED显示
  analogWrite(6, 0);
    digitalWrite(8, LOW);
  u8g.firstPage();
 do {
 setFont_L;
 u8g.setPrintPos(4, 16);
 u8g.print("OFF");
 }while( u8g.nextPage() );
  }
   if(wake){
   strip.setPixelColor(0, strip.Color(0, 255, 0));//绿光
  strip.show();  //LED显示
 sensor_tem = SHT2x.readT();//测温度
  delay(500);
  if(cmd=='a'){mode=0;}//WiFi切换模式
  if (irrecv.decode(&results))if(results.value==0x1FE807F)mode=0;//红外切换模式
   if (irrecv.decode(&results))if(results.value==0x1FE40BF)mode=1;
  u8g.firstPage();
 do {
 setFont_L;
 u8g.setPrintPos(4, 16);//液晶显示开关
 u8g.print("ON");
 u8g.setPrintPos(4, 32);//液晶显示温度
  u8g.print("TEM:");
  u8g.setPrintPos(40, 32);
  u8g.print(sensor_tem);
   u8g.setPrintPos(4, 48);//液晶显示速度
  u8g.print("SPEED:");
  u8g.setPrintPos(50, 48);
  u8g.print(value);
if(mode==0){
u8g.setPrintPos(4, 64);
  u8g.print("AUTO");  
}
if(mode==1){
u8g.setPrintPos(4, 64);
  u8g.print("BYHAND");
}
 
   }while( u8g.nextPage() );
   
 if(mode==0){
  automoto();
   }
   if(mode==1){ 
    motorcontrol();
    }
        /*sensor_hum =SHT2x.readRH();  //温度湿度回传
    dtostrf(sensor_tem, 2, 1, sensor_tem_c);
    dtostrf(sensor_hum, 2, 1, sensor_hum_c);

    
    jsonToSend="success_jsonpCallback({\n\"Temperature\":";
    dtostrf(sensor_tem,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+=",\n\"Humidity\":";
    dtostrf(sensor_hum,1,2,buf);
    jsonToSend+="\""+String(buf)+"\"";
    jsonToSend+=",\n\"Light\":\"263.73\"";
    jsonToSend+="\n})";


    postString="HTTP/1.1 200 OK";
    postString+="\r\n";
    postString+="Connection:close\r\n";
    postString+="Server:";
    postString+=wifi.getIPStatus().c_str();
    postString+="\r\n";
    postString+="Date:Mon,6Oct2003 13:23:42 GMT";
    postString+="\r\n";
    postString+="Content-Length:";
    postString+=jsonToSend.length();
    postString+="\r\n";
    postString+="contentType:application/json";
    postString+="\r\n";
    postString+="\r\n";
    postString+=jsonToSend;
    postString+="\r\n";
    postString+="\r\n";

  postArray = postString.c_str();                 //将str转化为char数组
   wifi.send(mux_id, postArray, strlen(postArray));
    postArray=NULL; //清空数组，等待下次传输数据
 wifi.releaseTCP(mux_id);*/
 }
}





  

  





