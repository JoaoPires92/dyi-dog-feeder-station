  /* EEPROM Memory --> Size 4096 bytes (Escreve até posição 4096)
   *  
   * 0:Contador Doses
   * 1: LastDayOn
   * 2-8: Delay sequencia de dosagem
   * 9: RegDosePointer
   * 10-111: Registo Doses
   * 115-126: Auto Dose Control
  */

/* ************************+*** Guia de Ecras ********************************************
   * 0= Ecra Info
   * 1= Ecra Principal 
   * 2= Ecra de Configurar
   * 3= Configurar - Seq Atuador Linear
   * 4= Reserva
   * 5= Configurar - Doseador Automático - Dias da semana
   * 6= Configurar - Doseador Automático - Hora
   * 7=  Reserva
   * 8= Configurar - Acerto da data : Dia da semana & dia do mês
   * 9= Acerto da data : Mês & Ano
   * 10= Acerto do Tempo : Hora & Minuto
   * 11= Registo Doses 1
   * 12= Registo Doses 2
   * 13= Registo Doses 3
   * 14= Registo Doses 4
   * 15= Reserva
*/

//*********************Librarias************************* 
#include <Elegoo_GFX.h>    
#include <Elegoo_TFTLCD.h>  
#include <TouchScreen.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <ezButton.h>
#include "EasyBuzzer.h"


//********************************** Ecra Info: Definir numero de Serie do aparelho e data de fabricação ************************* 

String DeviceModel = "DogFeeder"; //Tipo de aparelho --> nome nao deve ser mais longo que 18 caracteres
String DeviceSN = "2023010001"; // Numero de Serie 10 caracteres 
int DeviceMonth = 1; // Mes de Fabrico
int DeviceYear = 2023; // Ano de Fabrico
String DeviceDeveloper = "Joao Pires"; // Desenvolvido Por
// Descrever os meses para ecra info
char devicemonth[12][12] = {"Janeiro", "Fevereiro", "Marco", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"};


//******Parametros para o LCD Elegoo ********
#define LCD_CS A3 
#define LCD_CD A2 
#define LCD_WR A1 
#define LCD_RD A0 
#define LCD_RESET A4 

// **********Parametros para Cartão SD ***********
#define SD_SCK 13
#define SD_MISO 12
#define SD_MOSI 11
#define SD_CS 10 

// Pressão do toque para o LCD
//#define PENRADIUS 3
#define MINPRESSURE 10
#define MAXPRESSURE 1000

// Valores de Calibração para o LCD Elegoo
#define TS_MINX 128
#define TS_MINY 77
#define TS_MAXX 903
#define TS_MAXY 892

// Parametros para touch Elegoo
#define YP A3 
#define XM A2  
#define YM 9   
#define XP 8  

// Definição de Cores para o LCD

#define BLACK 0x0000 
#define NAVY 0x000F 
#define DARKGREEN 0x03E0 
#define DARKCYAN 0x03EF 
#define MAROON 0x7800 
#define PURPLE 0x780F 
#define OLIVE 0x7BE0 
#define GREY 0xC618 
#define DARKGREY 0x7BEF 
#define BLUE 0x001F 
#define GREEN 0x07E0 
#define CYAN 0x07FF 
#define RED 0xF800 
#define MAGENTA 0xF81F 
#define YELLOW 0xFFE0 
#define WHITE 0xFFFF 
#define ORANGE 0xFD20 
#define GREENYELLOW 0xAFE5 
#define PINK 0xF81F

//*************Declaração LCD & Touch Screen************

  Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
  TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); // Chamada para Touch

//************* Declaração RTC DS1307****************
  RTC_DS1307 rtc;
  
//**************Variaveis Globais*******************

// RTC Variaveis
char daysOfTheWeek[7][12] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"}; // Defines RTC DaysOfWeek
String DaysOfTheWeek;
int Aux_DayOfTheWeek;
int DayOfWeek;
int Minute_old;
int Day;
int Month;
int Year;
int Hour;
int Minute;
int Second;
int Old_Minute=60;
int LastDayOn;
boolean Flg_RTCAlarm=false;
boolean Flg_RTCAlarmLastStatus=false;
boolean DayOfWeekPressed = false;
boolean DayPressed=false;
boolean MonthPressed=false;
boolean YearPressed=false;
boolean HourPressed=false;
boolean MinutePressed=false;

// 1s Pulse
boolean boPulse_1s = false;
unsigned long CntPreviousPulse_1s = 0;  
int iPulse_1s=1000;

int CurrentScreen;

boolean ButtonNext = false;
boolean ButtonBack = false;
boolean ButtonUp = false;
boolean ButtonDown = false;

// Inputs
//ezButton LA_Extended = 29; // RL1 - Linear Actuator Extended 
ezButton LA_Retracted = 27; // RL2 - Linear Actuator Retracted
//boolean boExtended; // Linear Actuator Extended State
boolean boRetracted; // Linear Actuator Retracted State

// Outputs
int buzzer_pin = 29;// RL1 - Buzzer
int LA_ENA = 31; // SNS - Linear Actuator - Enable
int LA_1 = 25; // RL3 - Linear Actuator - In1
int LA_2 = 23; // RL4 - Linear Actuator - In2

int iStep; // Sequence step for LA Control
int DoseCounter; // Contador de doses
boolean boGiveDose; // Flag para iniciar sequencia de dosagem
float LA_Seq_Delay; // Delay sequencia de dosagem
int RegDosePointer; // Apontador Registo Doses


int ConfigMon; // 0= Off 1= On
int ConfigTue;
int ConfigWed;
int ConfigThu;
int ConfigFri;
int ConfigSat;
int ConfigSun;
int AutoDoseOff;// 0= off 1=on
int ConfigHour1;
int ConfigMinute1;
int ConfigHour2;
int ConfigMinute2;
boolean Hour1Pressed = false;
boolean Minute1Pressed = false;
boolean Hour2Pressed = false;
boolean Minute2Pressed = false;

unsigned long TimeBetweenDoses;
unsigned long LastDoseTimeCounter;

boolean boWarningBeep_1pulse = false;
boolean boAlarmBeep_1pulse = false;

void setup() {

  //LA_Extended.setDebounceTime(50); // set debounce time to 50 milliseconds
  LA_Retracted.setDebounceTime(50); // set debounce time to 50 milliseconds
  
  EasyBuzzer.setPin(buzzer_pin); // RL1 Pin 29
  
  //Declarar Outputs
  pinMode(LA_ENA,OUTPUT);
  pinMode(LA_1,OUTPUT);
  pinMode(LA_2,OUTPUT);

// Iniciar Outputs
  digitalWrite(LA_ENA,HIGH);
  digitalWrite(LA_1,LOW);
  digitalWrite(LA_2,LOW);
  
 //********************************  Iniciar LCD  ***************************************
  tft.reset();
  uint16_t identifier = tft.readID();
  identifier=0x9341;
  tft.begin(identifier);
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.fillScreen(BLACK);
  tft.setTextColor(WHITE);

  //*********************************** Iniciar Porta Série********************************
  Serial.begin(9600);

// **********************************Iniciar RTC***************************

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
     
// ***********Acertar a hora e data do RTC para Primeira Programação ou em caso do RTC parar ****************

  if (!rtc.isrunning()){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // ************************************* Iniciar variaveis ************************************ 
 //Enviar na primeira programação
  /*EEPROM.write(0,0); // Contador Doses
    EEPROM.write(1,0); // Last Day On
    EEPROM.write(2,0); // Delay Sequencia Doseador
    EEPROM.write(3,0);
    EEPROM.write(4,0);
    EEPROM.write(5,0);
    EEPROM.write(6,0);
    EEPROM.write(7,0);
    EEPROM.write(8,0);
    EEPROM.write(9,0); // Pointer
    for (int i=10; i<=111; i++) EEPROM.write(i,0); // Registo Doses
    EEPROM.write(115,1);
    EEPROM.write(116,1);
    EEPROM.write(117,1);
    EEPROM.write(118,1);
    EEPROM.write(119,1);
    EEPROM.write(120,1);
    EEPROM.write(121,1);
    EEPROM.write(122,1);
    EEPROM.write(123,8);
    EEPROM.write(124,30);
    EEPROM.write(125,19);
    EEPROM.write(126,30); 
   */
 
  // Carregar Valores de memória
  DoseCounter = EEPROM.read(0);
  LastDayOn = EEPROM.read(1);
  EEPROM.get(2 + sizeof(int), LA_Seq_Delay);
  RegDosePointer = EEPROM.read(9);
  ConfigMon=EEPROM.read(115);
  ConfigTue=EEPROM.read(116);
  ConfigWed=EEPROM.read(117);
  ConfigThu=EEPROM.read(118);
  ConfigFri=EEPROM.read(119);
  ConfigSat=EEPROM.read(120);
  ConfigSun=EEPROM.read(121);
  AutoDoseOff=EEPROM.read(122); 
  ConfigHour1=EEPROM.read(123);
  ConfigMinute1=EEPROM.read(124);
  ConfigHour2=EEPROM.read(125);
  ConfigMinute2=EEPROM.read(126);

// Chamar rotina RTC para obter valores
  ReadRTC();

 // ************************ Ultimo dia de uso do Controlador para fazer reset ao Contador **********************
  if (LastDayOn != Day){
      DoseCounter=0;
      LastDayOn=Day;
      EEPROM.write(0,DoseCounter);
      EEPROM.write(1,LastDayOn);    
  }
 
//***************** Iniciar Ecra de Apresentação e Principal ****************
  CurrentScreen =1;
  Screen1();


//***************** Iniciar Variaveis Globais ****************
  iStep=0;
  boGiveDose=false; 
  TimeBetweenDoses = 43200; // 43200=12 hours
  LastDoseTimeCounter =0;

} // Fim Setup



void loop() {

//**************** Chamada de Funções Loop Principal **********************

// Chamar rotina para obter coordenadas touch para botões. (Apenas para setup)
    //TouchButtonTeach();

// Chamar rotina para identificar botão pressionado no ecra touch 
    AnyButtonPressed();
    
//----------------------------------------------------------------------------------
//************ Rotina para gerar Pulse 1s ****************
  unsigned long CntPulse_1s = millis();
  if (CntPulse_1s - CntPreviousPulse_1s >= iPulse_1s) {   
      CntPreviousPulse_1s = CntPulse_1s;
      boPulse_1s=true;
    }
  else{
      boPulse_1s=false;
    }

//**************** Chamada de Funções a cada segundo **********************
  if (boPulse_1s==true){

 //  Reset do contador de doses quando novo dia começar
    if (Hour==0 && Minute==0 && LastDayOn != Day){
        DoseCounter=0;
        boAlarmBeep_1pulse=false;
        LastDayOn=Day;
        EEPROM.write(0,DoseCounter);
        EEPROM.write(1,LastDayOn);
        if(CurrentScreen==1){DoseCounterUpdate();}         
    }
    
     GiveDoseWithoutRTC(); // Call Dose Counter With no RTC
    
  } // Fim Loop 1s

//*********************Main Loop*******************

 if(CurrentScreen!=8 && CurrentScreen!=9 && CurrentScreen!=10){ReadRTC();} // Chamada para RTC
 LimitSwitchStatus();
 LA_Control(LA_Seq_Delay*1000);      
 if(Flg_RTCAlarm == false){AutoDoseControl();}
 DoseControl_Alarm();
 EasyBuzzer.update();
 AutoStopLA();
       
} // Fim Loop


// *************************************** FUNCÇÕES *****************************

// :::::::::::::::::::::::: Ecras :::::::::::::::::::::::::::

  void Screen0(){

    iStep=0;
    EasyBuzzer.stopBeep();
    
    PrintScreenLayout1();
    tft.setTextSize(2);

    //Modelo
    tft.setTextColor(YELLOW);
    tft.setCursor(10,13);
    tft.print("Modelo:");
    tft.setTextColor(WHITE);
    tft.setCursor(95,13);
    tft.print(DeviceModel); 
    tft.drawLine(5,40,315,40,WHITE);
    tft.drawLine(5,41,315,41,WHITE);

    //Numero Serie
    tft.setTextColor(YELLOW);
    tft.setCursor(10,53);
    tft.print("Numero Serie:");
    tft.setTextColor(WHITE);
    tft.setCursor(170,53);
    tft.print(DeviceSN);
    tft.drawLine(5,80,315,80,WHITE);
    tft.drawLine(5,81,315,81,WHITE);

    //Mes de Fabríco
    tft.setTextColor(YELLOW);    
    tft.setCursor(10,93);
    tft.print("Mes Fabricado:");
    tft.setTextColor(WHITE);    
    tft.setCursor(179,93);
    tft.print(devicemonth[DeviceMonth -1]);
    tft.drawLine(5,120,315,120,WHITE);
    tft.drawLine(5,121,315,121,WHITE);

    //Ano de Fabríco
    tft.setTextColor(YELLOW);       
    tft.setCursor(10,133);
    tft.print("Ano Fabricado:");
    tft.setTextColor(WHITE);
    tft.setCursor(179,133);
    tft.print(DeviceYear); 
    tft.drawLine(5,160,315,160,WHITE);
    tft.drawLine(5,161,315,161,WHITE);

    //Desonvolvido Por
    tft.setTextColor(YELLOW); 
    tft.setCursor(10,173);
    tft.print("CopyRight  :");
    tft.drawCircle(128,175,9,YELLOW);
    tft.setCursor(124,167);
    tft.print("c");
    tft.setTextColor(WHITE);
    tft.setCursor(163,173);
    tft.print(DeviceDeveloper);
  
  }

  void Screen1(){  
    PrintScreenLayout2();
    
     tft.setCursor(10,23);
     tft.setTextColor(ORANGE);
     tft.setTextSize(3);
     tft.print("::::::DUQUE::::::");   
    //Linha Cima
    tft.drawLine(5,61,315,61,WHITE);
    tft.drawLine(5,62,315,62,WHITE);
    //Linha Baixo
    tft.drawLine(5,156,315,156,WHITE);
    tft.drawLine(5,157,315,157,WHITE);
    //Botão
     tft.fillRect(18,75,186,70,GREEN);
     tft.drawRect(18,75,186,70,GREY);
     tft.drawRect(17,76,186,68,GREY);    
     tft.setCursor(27,102);
     tft.setTextColor(BLACK);
     tft.setTextSize(2);
     tft.print("DISPENSAR DOSE");
     //Linha vertical
     tft.drawLine(215,158,215,62,WHITE); 
     tft.drawLine(216,158,216,62,WHITE);
     DoseCounterUpdate();
     PrintDateTime();
  }

  void DoseCounterUpdate(){
     //Contador de Doses
     pinMode(XM, OUTPUT);
     pinMode(YP, OUTPUT);
     tft.fillRect(225,68,78,77,BLACK);
     if (DoseCounter<9){tft.setCursor(250,90);}
     if (DoseCounter>9){tft.setCursor(228,90);}
     tft.setTextColor(YELLOW);
     tft.setTextSize(6);
     tft.print(DoseCounter);        
   }

void Screen2(){
  
    iStep=0;
    EasyBuzzer.stopBeep();
        
    PrintScreenLayout1();

    // Linhas para escrever titulos
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE); 

    tft.setCursor(100,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Configurar"); 

    tft.fillRect(35,45,250,40,DARKGREY);
    tft.drawRect(35,45,250,40,WHITE);
    tft.drawRect(34,46,250,38,WHITE);    
    tft.setCursor(50,57);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Sequencia Doseadora");

    tft.fillRect(35,95,250,40,DARKGREY);
    tft.drawRect(35,95,250,40,WHITE);
    tft.drawRect(34,96,250,38,WHITE);    
    tft.setCursor(57,108);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Programar Doseador");

    tft.fillRect(35,145,250,40,DARKGREY);
    tft.drawRect(35,145,250,40,WHITE);
    tft.drawRect(34,146,250,38,WHITE);    
    tft.setCursor(57,157);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Acertar Data/Hora");
        
  }   

  void Screen3(){

   PrintScreenLayout();
    
// Linhas para escrever titulos
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE); 

    tft.setCursor(55,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Configurar Dosagem");

    tft.drawLine(5,85,315,85,WHITE);// Linha para titulos
    tft.drawLine(5,86,315,86,WHITE);

 //Escrever Temperatura Directa  
    tft.setCursor(20,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Tempo do Doseador Aberto");        

 //Escrever Valor Temperatura Directa
    Screen3UpdateTempInterval();
    tft.setCursor(195,130);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print("Seg");

    tft.setCursor(50,182);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.print("Valor Minimo:0.1s      Valor Maximo:9.9s");             
  }  

  void Screen3UpdateTempInterval(){
    tft.fillRect(105,112,85,50,BLACK);
    tft.setTextColor(WHITE);
    tft.setCursor(115,125);
    tft.setTextSize(4);
    tft.print(LA_Seq_Delay,1);    
  }


void Screen5(){

    PrintScreenLayout();
    
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE);
    tft.setCursor(35,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Sel. Dias Para Dosear");

    tft.drawLine(159,36,159,193,WHITE); // Linha Vertical
    tft.drawLine(160,36,160,193,WHITE);


    tft.drawLine(5,75,315,75,WHITE);
    tft.drawLine(5,76,315,76,WHITE); 

    tft.drawLine(5,115,315,115,WHITE);
    tft.drawLine(5,116,315,116,WHITE); 

    tft.drawLine(5,155,315,155,WHITE);
    tft.drawLine(5,156,315,156,WHITE);

    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.setCursor(95,200);
    tft.print("Selecionar para ativar");
    tft.setTextColor(WHITE);
    tft.fillCircle(103,223,3,GREEN);
    tft.setCursor(111,220);
    tft.print("Ativo |");
    tft.fillCircle(162,223,3,RED);
    tft.setCursor(170,220);
    tft.print("Desativo");

    Screen5Update(); 
       
  }

 void Screen5Update(){
    
   tft.setTextSize(2);

   if(ConfigMon==0){tft.setTextColor(RED);} else if(ConfigMon==1){tft.setTextColor(GREEN);}
   tft.setCursor(45,50);
   tft.print("SEGUNDA");

   if(ConfigTue==0){tft.setTextColor(RED);} else if(ConfigTue==1){tft.setTextColor(GREEN);}
   tft.setCursor(200,50);
   tft.print("TERCA");

   if(ConfigWed==0){tft.setTextColor(RED);} else if(ConfigWed==1){tft.setTextColor(GREEN);}  
   tft.setCursor(45,90);
   tft.print("QUARTA");

   if(ConfigThu==0){tft.setTextColor(RED);} else if(ConfigThu==1){tft.setTextColor(GREEN);}
   tft.setCursor(200,90);
   tft.print("QUINTA");

   if(ConfigFri==0){tft.setTextColor(RED);} else if(ConfigFri==1){tft.setTextColor(GREEN);}
   tft.setCursor(45,130);
   tft.print("SEXTA");

   if(ConfigSat==0){tft.setTextColor(RED);} else if(ConfigSat==1){tft.setTextColor(GREEN);}
   tft.setCursor(200,130);
   tft.print("SABADO");

   if(ConfigSun==0){tft.setTextColor(RED);} else if(ConfigSun==1){tft.setTextColor(GREEN);}
   tft.setCursor(45,170);
   tft.print("DOMINGO");

   if(AutoDoseOff==0){tft.setTextColor(RED);} else if(AutoDoseOff==1){tft.setTextColor(GREEN);}
   tft.setCursor(190,170);
   tft.print("DESLIGADO");
 }

 void Screen6(){
   
    PrintScreenLayout();

    tft.drawLine(158,36,158,192,WHITE); // Linha vertical a dividir 
    tft.drawLine(159,36,159,192,WHITE); 
     
// Linhas para escrever titulos
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE);
    tft.setCursor(37,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Sel. Horas Para Dosear");  


    tft.drawLine(5,65,315,65,WHITE);
    tft.drawLine(5,66,315,66,WHITE);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(50,45);   
    tft.print("DOSE 1");
    tft.setCursor(201,45);   
    tft.print("DOSE 2"); 
    
    tft.drawLine(5,125,315,125,WHITE);
    tft.drawLine(5,126,315,126,WHITE);

    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.setCursor(16,184);    
    tft.print("Pressionar para Editar");          
     
    tft.setCursor(172,184);
    tft.print("Pressionar para Editar");

    tft.setCursor(16,116);
    tft.print("Pressionar para Editar");          
     
    tft.setCursor(172,116);
    tft.print("Pressionar para Editar");

    Screen6UpdateHour1();
    Screen6UpdateMinute1();
    Screen6UpdateHour2();
    Screen6UpdateMinute2();    

 }

 void Screen6UpdateHour1(){
  
   tft.fillRect(10,70,145,42,BLACK);
   if(Hour1Pressed==true){tft.setTextColor(YELLOW);}else{tft.setTextColor(WHITE);}
   tft.setTextSize(3);
   tft.setCursor(25,82);
   if(ConfigHour1 < 10){tft.print("0"); tft.print(ConfigHour1);}else{tft.print(ConfigHour1);}
   tft.setTextSize(2);
   tft.setCursor(80,82);
   tft.print("Horas");

 }
 void Screen6UpdateMinute1(){

   tft.fillRect(10,130,145,50,BLACK);
   if(Minute1Pressed==true){tft.setTextColor(YELLOW);}else{tft.setTextColor(WHITE);}
   tft.setTextSize(3);
   tft.setCursor(25,147);
   if(ConfigMinute1 < 10){tft.print("0"); tft.print(ConfigMinute1);}else{tft.print(ConfigMinute1);}
   tft.setTextSize(2);
   tft.setCursor(71,147);
   tft.print("Minutos");
 
 }
 void Screen6UpdateHour2(){

   tft.fillRect(165,70,145,42,BLACK);
   if(Hour2Pressed==true){tft.setTextColor(YELLOW);}else{tft.setTextColor(WHITE);}
   tft.setTextSize(3);
   tft.setCursor(179,82);
   if(ConfigHour2 < 10){tft.print("0"); tft.print(ConfigHour2);}else{tft.print(ConfigHour2);}
   tft.setTextSize(2);
   tft.setCursor(234,82);
   tft.print("Horas");
 
 }
 void Screen6UpdateMinute2(){

   tft.fillRect(165,130,145,50,BLACK);
   if(Minute2Pressed==true){tft.setTextColor(YELLOW);}else{tft.setTextColor(WHITE);}
   tft.setTextSize(3);
   tft.setCursor(179,147);
   if(ConfigMinute2 < 10){tft.print("0"); tft.print(ConfigMinute2);}else{tft.print(ConfigMinute2);}
   tft.setTextSize(2);
   tft.setCursor(225,147);
   tft.print("Minutos");
   
 }
 
  void Screen8(){

   PrintScreenLayout();
    
// Linhas para escrever titulos
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE); 

    tft.setCursor(33,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Relogio: Acertar Data");

    tft.drawLine(158,36,158,192,WHITE); // Linha a dividir 
    tft.drawLine(159,36,159,192,WHITE);

 //Escrever Dia da Semana  
    tft.setCursor(15,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Semana: Dia");        
 
 //Escrever Mês : Dia
    tft.setCursor(190,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Mes: Dia");        

    tft.drawLine(5,85,315,85,WHITE);// Linha para titulos
    tft.drawLine(5,86,315,86,WHITE);    

 //Escrever Valor do dia da semana 
    Screen8UpdateDayOfWeek();
    
//Escrever Valor do dia do Mês 
    Screen8UpdateDay();
    tft.setCursor(172,182);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.print("Pressionar para Editar");
  }

 void Screen8UpdateDayOfWeek(){
//Escrever Valor Dia da Semana
    tft.fillRect(15,105,130,65,BLACK);  
    tft.setCursor(45,125);   
    if(DayOfWeekPressed==true){tft.setTextColor(YELLOW);}
    if(DayOfWeekPressed==false){tft.setTextColor(WHITE);}
    tft.setTextSize(4);
    tft.print(DaysOfTheWeek);  
 }

 void Screen8UpdateDay(){  
//Escrever Valor Diado Mês 
    tft.fillRect(170,105,130,65,BLACK);
    tft.setCursor(215,125);
    if(DayPressed==true){tft.setTextColor(YELLOW);}
    if(DayPressed==false){tft.setTextColor(WHITE);}
    tft.setTextSize(4);
    if(Day<10){tft.print("0");
    tft.print(Day);}
    if(Day>=10){tft.print(Day);} 
 }

  void Screen9(){

   PrintScreenLayout();
    
// Linhas para escrever titulos
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE); 

    tft.setCursor(33,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Relogio: Acertar Data");

    tft.drawLine(158,36,158,192,WHITE); // Linha a dividir 
    tft.drawLine(159,36,159,192,WHITE);

//Escrever MÊs 
    tft.setCursor(65,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Mes");        
 
 //Escrever Ano
    tft.setCursor(220,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Ano");        

    tft.drawLine(5,85,315,85,WHITE);// Linha para titulos
    tft.drawLine(5,86,315,86,WHITE); 
    
 //Escrever Valor do Mês
    Screen9UpdateMonth();
    tft.setCursor(16,182);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.print("Pressionar para Editar");          
    
//Escrever Valor do Ano
    Screen9UpdateYear();
    tft.setCursor(172,182);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.print("Pressionar para Editar");
  }

 void Screen9UpdateMonth(){
//Escrever Valor Dia da Semana
    tft.fillRect(15,105,130,65,BLACK); 
    tft.setCursor(60,125);   
    if(MonthPressed==true){tft.setTextColor(YELLOW);}
    if(MonthPressed==false){tft.setTextColor(WHITE);}
    tft.setTextSize(4);
    if(Month<10){tft.print("0");
    tft.print(Month);}
    if(Month>=10){tft.print(Month);}
 }

 void Screen9UpdateYear(){  
//Escrever Valor Diado Mês 
    tft.fillRect(170,105,130,65,BLACK);
    tft.setCursor(190,125); 
    if(YearPressed==true){tft.setTextColor(YELLOW);}
    if(YearPressed==false){tft.setTextColor(WHITE);}
    tft.setTextSize(4);
    tft.print(Year);
 }

 void Screen10(){

   PrintScreenLayout();
    
// Linhas para escrever titulos
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE); 

    tft.setCursor(33,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    tft.print("Relogio: Acertar Tempo");

    tft.drawLine(158,36,158,192,WHITE); // Linha a dividir 
    tft.drawLine(159,36,159,192,WHITE);

//Escrever Hora
    tft.setCursor(58,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Hora");        
 

 //Escrever Minuto
    tft.setCursor(197,55);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Minutos");        

    tft.drawLine(5,85,315,85,WHITE);// Linha para titulos
    tft.drawLine(5,86,315,86,WHITE); 
    
//Escrever Valor da Hora
    tft.setCursor(60,125);   
    tft.setTextColor(WHITE);
    tft.setTextSize(4);
    if(Hour<10){tft.print("0");
    tft.print(Hour);}
    if(Hour>=10){tft.print(Hour);}

    tft.setCursor(16,182);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.print("Pressionar para Editar");          
    
//Escrever Valor do Minuto
    tft.setCursor(215,125); 
    tft.setTextColor(WHITE);
    tft.setTextSize(4);
    if(Minute<10){tft.print("0");
    tft.print(Minute);}
    if(Minute>=10){tft.print(Minute);}

    tft.setCursor(172,182);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);
    tft.print("Pressionar para Editar");
  }

 void Screen10UpdateHour(){
 //Escrever Valor Dia da Semana
    tft.fillRect(15,105,130,65,BLACK);
    tft.setCursor(60,125);   
    if(HourPressed==true){tft.setTextColor(YELLOW);}
    if(HourPressed==false){tft.setTextColor(WHITE);}
    tft.setTextSize(4);
    if(Hour<10){tft.print("0");
    tft.print(Hour);}
    if(Hour>=10){tft.print(Hour);}
 }

 void Screen10UpdateMinute(){  
//Escrever Valor Diado Mês 
    tft.fillRect(170,105,130,65,BLACK);
    tft.setCursor(215,125); 
    if(MinutePressed==true){tft.setTextColor(YELLOW);}
    if(MinutePressed==false){tft.setTextColor(WHITE);}
    tft.setTextSize(4);
    if(Minute<10){tft.print("0");
    tft.print(Minute);}
    if(Minute>=10){tft.print(Minute);}
 }
 
 void Screen11(){

    Screen11_14_Layout();

    int OffsetY=74;
    int OffsetPos=0;
    
  for (int i=1; i<=4; i++){
    
    int day=EEPROM.read(10+OffsetPos);
    int month=EEPROM.read(11+OffsetPos);
    int year=EEPROM.read(12+OffsetPos);
    int hour=EEPROM.read(13+OffsetPos);
    int minute=EEPROM.read(14+OffsetPos);
    int dose=EEPROM.read(15+OffsetPos);
    
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(38,OffsetY);
    if(day<10){tft.print("0");}
    tft.print(day);
    tft.print("/");
    if(month<10){tft.print("0");}
    tft.print(month);
    tft.print("/");
    tft.print(year);
    tft.setCursor(180,OffsetY);
    if(hour<10){tft.print("0");}
    tft.print(hour);
    tft.print(":");
    if(minute<10){tft.print("0");}
    tft.print(minute);
    if(DoseCounter<9){tft.setCursor(278,OffsetY);}
    if(DoseCounter>9){tft.setCursor(272,OffsetY);}
    tft.print(dose);

    OffsetY=OffsetY + 31;
    OffsetPos=OffsetPos + 6;
 }
       
 }

 void Screen12(){

    Screen11_14_Layout();

    int OffsetY=74;
    int OffsetPos=0;
    
  for (int i=1; i<=4; i++){
    
    int day=EEPROM.read(34+OffsetPos);
    int month=EEPROM.read(35+OffsetPos);
    int year=EEPROM.read(36+OffsetPos);
    int hour=EEPROM.read(37+OffsetPos);
    int minute=EEPROM.read(38+OffsetPos);
    int dose=EEPROM.read(39+OffsetPos);
    
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(38,OffsetY);
    if(day<10){tft.print("0");}
    tft.print(day);
    tft.print("/");
    if(month<10){tft.print("0");}
    tft.print(month);
    tft.print("/");
    tft.print(year);
    tft.setCursor(180,OffsetY);
    if(hour<10){tft.print("0");}
    tft.print(hour);
    tft.print(":");
    if(minute<10){tft.print("0");}
    tft.print(minute);
    if(DoseCounter<9){tft.setCursor(278,OffsetY);}
    if(DoseCounter>9){tft.setCursor(272,OffsetY);}
    tft.print(dose);

    OffsetY=OffsetY + 31;
    OffsetPos=OffsetPos + 6;
 }
       
 }

 void Screen13(){

    Screen11_14_Layout();

    int OffsetY=74;
    int OffsetPos=0;
    
  for (int i=1; i<=4; i++){
    
    int day=EEPROM.read(58+OffsetPos);
    int month=EEPROM.read(59+OffsetPos);
    int year=EEPROM.read(60+OffsetPos);
    int hour=EEPROM.read(61+OffsetPos);
    int minute=EEPROM.read(62+OffsetPos);
    int dose=EEPROM.read(63+OffsetPos);
    
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(38,OffsetY);
    if(day<10){tft.print("0");}
    tft.print(day);
    tft.print("/");
    if(month<10){tft.print("0");}
    tft.print(month);
    tft.print("/");
    tft.print(year);
    tft.setCursor(180,OffsetY);
    if(hour<10){tft.print("0");}
    tft.print(hour);
    tft.print(":");
    if(minute<10){tft.print("0");}
    tft.print(minute);
    if(DoseCounter<9){tft.setCursor(278,OffsetY);}
    if(DoseCounter>9){tft.setCursor(272,OffsetY);}
    tft.print(dose);

    OffsetY=OffsetY + 31;
    OffsetPos=OffsetPos + 6;
 }
       
 }

 void Screen14(){

    Screen11_14_Layout();

    int OffsetY=74;
    int OffsetPos=0;
    
  for (int i=1; i<=4; i++){
    
    int day=EEPROM.read(82+OffsetPos);
    int month=EEPROM.read(83+OffsetPos);
    int year=EEPROM.read(84+OffsetPos);
    int hour=EEPROM.read(85+OffsetPos);
    int minute=EEPROM.read(86+OffsetPos);
    int dose=EEPROM.read(87+OffsetPos);
    
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(38,OffsetY);
    if(day<10){tft.print("0");}
    tft.print(day);
    tft.print("/");
    if(month<10){tft.print("0");}
    tft.print(month);
    tft.print("/");
    tft.print(year);
    tft.setCursor(180,OffsetY);
    if(hour<10){tft.print("0");}
    tft.print(hour);
    tft.print(":");
    if(minute<10){tft.print("0");}
    tft.print(minute);
    if(DoseCounter<9){tft.setCursor(278,OffsetY);}
    if(DoseCounter>9){tft.setCursor(272,OffsetY);}
    tft.print(dose);

    OffsetY=OffsetY + 31;
    OffsetPos=OffsetPos + 6;
 }
       
 }

 void Screen11_14_Layout(){
    
    PrintScreenLayout();
  
    tft.drawLine(5,36,315,36,WHITE);
    tft.drawLine(5,37,315,37,WHITE);

    tft.setCursor(20,12);
    tft.setTextColor(GREENYELLOW);
    tft.setTextSize(2);
    if(CurrentScreen==11){tft.print("Registo de Doses - Pag.1");}
    if(CurrentScreen==12){tft.print("Registo de Doses - Pag.2");}
    if(CurrentScreen==13){tft.print("Registo de Doses - Pag.3");}
    if(CurrentScreen==14){tft.print("Registo de Doses - Pag.4");}

    tft.drawLine(5,66,315,66,WHITE);
    tft.drawLine(5,67,315,67,WHITE); 

    tft.drawLine(165,36,165,193,WHITE); // Linha vertical a separar hora e temperatura
    tft.drawLine(166,36,166,193,WHITE);

    tft.drawLine(250,36,250,193,WHITE); // Linha vertical a separar temperatura e Humidade
    tft.drawLine(251,36,251,193,WHITE);

    tft.setTextColor(YELLOW);
    tft.setCursor(63,43);
    tft.setTextSize(2);
    tft.print("Data");
    tft.setCursor(183,43);
    tft.print("Hora");
    tft.setCursor(261,43);
    tft.print("Dose");    

    tft.drawLine(5,97,315,97,WHITE);
    tft.drawLine(5,98,315,98,WHITE);

    tft.drawLine(5,128,315,128,WHITE);
    tft.drawLine(5,129,315,129,WHITE);

    tft.drawLine(5,159,315,159,WHITE);
    tft.drawLine(5,160,315,160,WHITE);
 } 

// :::::::::::::::::::::::: Ecras de Layout :::::::::::::::::::::::::::

void PrintScreenLayout(){

    tft.fillScreen(BLACK);
// Draw Rectangle frame
    tft.drawRect(5,3,312,192,WHITE);
    tft.drawRect(4,4,312,190,WHITE);

// Button Back
 if (CurrentScreen==11 || CurrentScreen==12 || CurrentScreen==13 || CurrentScreen==14){
    tft.fillRect(5,198,74,40,DARKCYAN);
    tft.drawRect(5,198,74,40,WHITE);
    tft.drawRect(4,199,74,38,WHITE);       
 }
 else{
    tft.fillRect(5,198,74,40,DARKCYAN);
    tft.drawRect(5,198,74,40,WHITE);
    tft.drawRect(4,199,74,38,WHITE);       
    tft.setCursor(22,208);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print("<<");
 }
    
// Button Down
  if(CurrentScreen!=5){
    tft.fillRect(84,198,74,40,DARKCYAN);
    tft.drawRect(84,198,74,40,WHITE);
    tft.drawRect(83,199,74,38,WHITE);
  }
    if(CurrentScreen==999){
      tft.setCursor(86,210);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.print("Inver.");       
    }
    else if(CurrentScreen==999){
      tft.setCursor(95,208);   
      tft.setTextSize(3);
       tft.print("OFF");      
    }
    else if(CurrentScreen==11 || CurrentScreen==12 || CurrentScreen==13 || CurrentScreen==14){
      tft.setCursor(110,208);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.print("<");        
    }
   else if(CurrentScreen==3 || CurrentScreen==6){
      tft.setCursor(113,208);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.print("-");        
    }

// Button UP
  if(CurrentScreen!=5){
    tft.fillRect(163,198,74,40,DARKCYAN);
    tft.drawRect(163,198,74,40,WHITE);
    tft.drawRect(162,199,74,38,WHITE);
  }
    if(CurrentScreen==999){
      tft.setCursor(173,210);
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.print("Norm.");       
    }
    else if(CurrentScreen==999){
      tft.setCursor(183,208);
      tft.setTextSize(3);
      tft.print("ON");
    }
    else if(CurrentScreen==11 || CurrentScreen==12 || CurrentScreen==13 || CurrentScreen==14){
      tft.setCursor(192,208);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.print(">");   
    }
    else if(CurrentScreen==3 || CurrentScreen==6){
      tft.setCursor(192,208);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.print("+");   
    }

// Button NEXT
    tft.fillRect(242,198,75,40,DARKCYAN);
    tft.drawRect(242,198,75,40,WHITE);
    tft.drawRect(241,199,75,38,WHITE);
  if(CurrentScreen==999){
    tft.setCursor(251,211);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Reset");    
  }
  else{     
    tft.setCursor(263,208);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print(">>");
  }
    
  }  
 
void PrintScreenLayout1(){

    tft.fillScreen(BLACK);
// Draw Rectangle frame
    tft.drawRect(5,3,312,192,WHITE);
    tft.drawRect(4,4,312,190,WHITE);

// Button Back
    tft.fillRect(5,198,74,40,DARKCYAN);
    tft.drawRect(5,198,74,40,WHITE);
    tft.drawRect(4,199,74,38,WHITE);
   if (CurrentScreen==11){
      tft.setCursor(22,208);
      tft.setTextColor(WHITE);
      tft.setTextSize(3);
      tft.print("<<");
   }   

   
// Button NEXT
    tft.fillRect(242,198,75,40,DARKCYAN);
    tft.drawRect(242,198,75,40,WHITE);
    tft.drawRect(241,199,75,38,WHITE);   
    tft.setCursor(263,208);
    tft.setTextColor(WHITE);
    tft.setTextSize(3);
    tft.print(">>");
  

// Button Down
    tft.fillRect(84,198,74,40,DARKCYAN);
    tft.drawRect(84,198,74,40,WHITE);
    tft.drawRect(83,199,74,38,WHITE);

// Button UP
    tft.fillRect(163,198,74,40,DARKCYAN);
    tft.drawRect(163,198,74,40,WHITE);
    tft.drawRect(162,199,74,38,WHITE);
      
 }

void PrintScreenLayout2(){

   tft.fillScreen(BLACK);
// Draw Rectangle frame
   tft.drawRect(5,3,312,192,WHITE);
   tft.drawRect(4,4,312,190,WHITE);

// Button NEXT
    tft.fillRect(242,198,75,40,DARKCYAN);
    tft.drawRect(242,198,75,40,WHITE);
    tft.drawRect(241,199,75,38,WHITE);
    tft.setCursor(253,211);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Doses"); 

    tft.fillRect(5,198,74,40,DARKCYAN);
    tft.drawRect(5,198,74,40,WHITE);
    tft.drawRect(4,199,74,38,WHITE);
    tft.setCursor(15,211);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Info");


    tft.fillRect(84,198,153,40,DARKCYAN);
    tft.drawRect(84,198,153,40,WHITE);
    tft.drawRect(83,199,153,38,WHITE);
    tft.setCursor(100,211);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.print("Configurar"); 
    
}


// :::::::::::::::::::::::: Rotina Controlo do Actuador Linear :::::::::::::::::::::::::::

  void LimitSwitchStatus(){

     //LA_Extended.loop(); // MUST call the loop() function first
     LA_Retracted.loop(); // MUST call the loop() function first
    
     //if(LA_Extended.isPressed()){boExtended=true;}
     //if(LA_Extended.isReleased()){boExtended=false;}
    
     if(LA_Retracted.isPressed()){boRetracted=true;}
     if(LA_Retracted.isReleased()){boRetracted=false;}
   
  }

  void LA_Control(int iDelay){

    if (iStep==0){ // Inicializar e garantir que doseador está fechado
        CloseDoser();
        boAlarmBeep_1pulse=false;
        EasyBuzzer.stopBeep();
        iStep=1;        
    }

    if (iStep==1){ // Esperar Pelo Trigger
      if (boGiveDose==true){
        boGiveDose=false;
        iStep=2;
      }          
    }

    if (iStep==2){ // Abertura para dar Dose     
      OpenDoser();  
      if (boRetracted==true){
        StopLA();
        iStep=3;
      }          
    }

    if (iStep==3){ // Esperar Por Delay      
       delay(iDelay);
       iStep=4;
    } 

    if (iStep==4){ // Fechar Doseador
        CloseDoser();    
        iStep=5; 
    }
    
    if (iStep==5){ // Atualizar Contador + Registo Doses         
       DoseCounter++;
       boAlarmBeep_1pulse=false;
       EEPROM.write(0,DoseCounter);          
       if(CurrentScreen==1){DoseCounterUpdate();}
       pinMode(XM, OUTPUT);
       pinMode(YP, OUTPUT);
       if(Flg_RTCAlarm==false){RegDoses();}
       LastDoseTimeCounter=0;
       iStep=1;
    }
      
  }

  void CloseDoser(){
     digitalWrite(LA_1,HIGH); // Fechar Doseador
     digitalWrite(LA_2,LOW);
  }

  void OpenDoser(){
     digitalWrite(LA_1,LOW); // Abir Doseador
     digitalWrite(LA_2,HIGH);
  }

  void StopLA(){
     digitalWrite(LA_1,LOW); // Parar Atuador Linear
     digitalWrite(LA_2,LOW);
  }  


void AutoStopLA(){
  if((iStep==1 && Second == 0 && Hour == ConfigHour1 && Minute == ConfigMinute1+1) || iStep==1 && Second == 0 && Hour == ConfigHour2 && Minute == ConfigMinute2+1){
    StopLA();
  }    
}

  void RegDoses(){
  
    if (RegDosePointer > 95){RegDosePointer=0;}
     
     EEPROM.write(10+RegDosePointer,Day);
     EEPROM.write(11+RegDosePointer,Month);
     EEPROM.write(12+RegDosePointer,Year-2000);
     EEPROM.write(13+RegDosePointer,Hour);
     EEPROM.write(14+RegDosePointer,Minute);
     EEPROM.write(15+RegDosePointer,DoseCounter);

    RegDosePointer = RegDosePointer +6;
    EEPROM.write(9,RegDosePointer);      
  }

 void AutoDoseControl(){
    
  if((iStep==1 && boWarningBeep_1pulse==false && Second == 0 && Hour == ConfigHour1 && Minute == ConfigMinute1) || iStep==1 && boWarningBeep_1pulse==false && Second == 0 && Hour == ConfigHour2 && Minute == ConfigMinute2){
    
         switch (Aux_DayOfTheWeek) 
      {
         case 0:
             if(ConfigSun==1){WarningBeforeDose();}      
         break;          
         case 1:
             if(ConfigMon==1){WarningBeforeDose();}    
         break;
         case 2:
             if(ConfigTue==1){WarningBeforeDose();} 
         break;
         case 3:
             if(ConfigWed==1){WarningBeforeDose();}        
         break;
         case 4:
             if(ConfigThu==1){WarningBeforeDose();}
         break;
         case 5:
             if(ConfigFri==1){WarningBeforeDose();}      
         break;
         case 6:
             if(ConfigSat==1){WarningBeforeDose();}        
         break;                                                     
      } 
  }
      
}

void WarningBeforeDose(){
    boWarningBeep_1pulse=true;
    EasyBuzzer.beep(
    1000,    // Frequency in hertz(HZ).
    300,     // On Duration in milliseconds(ms).
    300,    // Off Duration in milliseconds(ms).
    6,    // The number of beeps per cycle.
    300,  // Pause duration.
    1,     // The number of cycle.
    GiveDoseAfterBeep    // [Optional] Function to call when done.
  );
}

 void GiveDoseAfterBeep(){
     boWarningBeep_1pulse=false;
     boGiveDose=true;
  }  


 void DoseControl_Alarm(){

  if (boAlarmBeep_1pulse==false && Flg_RTCAlarm==true){DoseAlarmON();}  
  
  if((boAlarmBeep_1pulse==false && Hour == ConfigHour1 && Minute == ConfigMinute1+1 && DoseCounter<1) || boAlarmBeep_1pulse==false && Hour == ConfigHour2 && Minute == ConfigMinute2+1 && DoseCounter<2){
    
         switch (Aux_DayOfTheWeek) 
      {
         case 0:
             if(ConfigSun==1){DoseAlarmON();}      
         break;          
         case 1:
             if(ConfigMon==1){DoseAlarmON();}    
         break;
         case 2:
             if(ConfigTue==1){DoseAlarmON();} 
         break;
         case 3:
             if(ConfigWed==1){DoseAlarmON();}        
         break;
         case 4:
             if(ConfigThu==1){DoseAlarmON();}
         break;
         case 5:
             if(ConfigFri==1){DoseAlarmON();}      
         break;
         case 6:
             if(ConfigSat==1){DoseAlarmON();}        
         break;                                                     
      } 
  }
      
}

void DoseAlarmON(){
    boAlarmBeep_1pulse=true;
    EasyBuzzer.beep(
    1000,    // Frequency in hertz(HZ).
    1000,     // On Duration in milliseconds(ms).
    2000,    // Off Duration in milliseconds(ms).
    12,    // The number of beeps per cycle.
    2000,  // Pause duration.
    180,     // The number of cycle.
    ResetAlarmONFlag   // [Optional] Function to call when done.
  );
}

void ResetAlarmONFlag(){
  boAlarmBeep_1pulse=false;
}

// :::::::::::::::::::::::: Rotina RTC - Obter Data e Hora :::::::::::::::::::::::::::
  void ReadRTC(){

    DateTime now = rtc.now();
    Year=now.year();
    Month=now.month();
    Day=now.day();
    DaysOfTheWeek=(daysOfTheWeek[now.dayOfTheWeek()]);
    Hour=now.hour();
    Minute=now.minute();
    Second=now.second();
    Aux_DayOfTheWeek= now.dayOfTheWeek();

    if(!rtc.isrunning() || Minute==165 || Day==0 ){
      Flg_RTCAlarm=true;
    }
    else{
      Flg_RTCAlarm=false;     
    }

    if (Minute!=Minute_old && CurrentScreen==1){
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      PrintDateTime();
      Minute_old=Minute;    
    }  
  }

// :::::::::::::::::::::::: Pedido para Escrever Data e Hora no Ecra Principal :::::::::::::::::::::::::::
  void PrintDateTime(){

   if (Flg_RTCAlarm==true && CurrentScreen==1){
      tft.fillRect(10,165,300,25,BLACK);
      tft.setTextColor(RED);
      tft.setTextSize(2);
      tft.setCursor(20,170);
      tft.print("!!!Erro!!!");
      //Linha Vertical
      tft.drawLine(155,158,155,192,WHITE); 
      tft.drawLine(156,158,156,192,WHITE);   
      //Linha vertical
      tft.drawLine(215,158,215,192,WHITE); 
      tft.drawLine(216,158,216,192,WHITE);
      tft.setCursor(169,170);
      tft.print("!!!");
      tft.print("   "); 
      tft.print("!!!!!");     
    }

   else if(Flg_RTCAlarm==false && CurrentScreen==1){
      tft.fillRect(10,165,300,25,BLACK);
      //Linha Vertical
      tft.drawLine(155,158,155,192,WHITE); 
      tft.drawLine(156,158,156,192,WHITE); 
      tft.setTextColor(WHITE);
      tft.setTextSize(2);
      tft.setCursor(20,170);
      if(Day<10){tft.print("0");
      tft.print(Day);}
      if(Day>=10){tft.print(Day);}
      tft.print("/");
      if(Month<10){tft.print("0");
      tft.print(Month);}
      if(Month>=10){tft.print(Month);}
      tft.print("/");
      tft.print(Year);
      tft.setCursor(169,170);
      //Linha vertical
      tft.drawLine(215,158,215,192,WHITE); 
      tft.drawLine(216,158,216,192,WHITE);
      tft.print(DaysOfTheWeek);
      tft.print("   ");
      if(Hour<10){tft.print("0");
      tft.print(Hour);}
      if(Hour>=10){tft.print(Hour);}
      tft.print(":");
      if(Minute<10){tft.print("0");
      tft.print(Minute);}
      if(Minute>=10){tft.print(Minute);}
    } 
  }


// *************************** Função para Touch *****************************
 void AnyButtonPressed(){

     TSPoint p = ts.getPoint();
   
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE){
    
    if(p.x>385 && p.x<580 && p.y>145 && p.y<570 && CurrentScreen==1){ // Botao Dispensar Dose.
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      if(iStep==1){
        EasyBuzzer.stopBeep();
        WarningBeforeDose();
        }
    }

    if(p.x>805 && p.x<875 && p.y>318 && p.y<669 && CurrentScreen==1){ // Botao Configigurar.
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      CurrentScreen=2;
      Screen2(); 
    }
    
   if(p.x>805 && p.x<875 && p.y>737 && p.y<883){ // Botão next
      ButtonNext=true;
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      ButtonNextPressed();  
    }
   if(p.x>805 && p.x<875 && p.y>113 && p.y<250){ // Botao Back
      ButtonBack=true;
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      ButtonBackPressed();    
    }

    if(p.x>805 && p.x<875 && p.y>531 && p.y<669 && CurrentScreen!=1){ // Botao up
      ButtonUp=true;
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT); 
      ButtonUpPressed();  
    }
    
   if(p.x>805 && p.x<875 && p.y>318 && p.y<453 && CurrentScreen!=1){ // Botao Down
      ButtonDown=true;
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      ButtonDownPressed();    
    }
    if(p.x>470 && p.x<717 && p.y>533 && p.y<860 && CurrentScreen==8){ // Clicar dia do mês
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      DayPressed=true;
      DayOfWeekPressed=false;
      Screen8UpdateDayOfWeek();
      Screen8UpdateDay();
    }
     if(p.x>470 && p.x<717 && p.y>141 && p.y<439 && CurrentScreen==9){ // Clicar em Mês
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      MonthPressed=true;
      YearPressed=false;
      Screen9UpdateMonth();
      Screen9UpdateYear();
    }
    if(p.x>470 && p.x<717 && p.y>533 && p.y<860 && CurrentScreen==9){ // Clicar em Ano
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      YearPressed=true;
      MonthPressed=false;
      Screen9UpdateMonth();
      Screen9UpdateYear();
    }
     if(p.x>470 && p.x<717 && p.y>141 && p.y<439 && CurrentScreen==10){ // Clicar em Hora
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      HourPressed=true;
      MinutePressed=false;
      Screen10UpdateHour();
      Screen10UpdateMinute();
    }
    if(p.x>470 && p.x<717 && p.y>533 && p.y<860 && CurrentScreen==10){ // Clicar em Minutos
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      MinutePressed=true;
      HourPressed=false;
      Screen10UpdateHour();
      Screen10UpdateMinute();
    }    

    if(p.x>305 && p.x<372 && p.y>187 && p.y<800 && CurrentScreen==2){ //Configurar: botao 1
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      CurrentScreen=3;
      Screen3();           
    }

    if(p.x>466 && p.x<548 && p.y>187 && p.y<800 && CurrentScreen==2){ //Configurar: botao 2
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      CurrentScreen=5;
      Screen5();
    }

    if(p.x>634 && p.x<712 && p.y>187 && p.y<800 && CurrentScreen==2){ //Configurar: botao 3
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      CurrentScreen=8;
      Screen8();           
    }

    if(p.x>298 && p.x<333 && p.y>141 && p.y<437 && CurrentScreen==5){ // Programar: Segunda
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigMon==0){
          ConfigMon=1;
          EEPROM.write(115,ConfigMon);         
        }
        else{
          ConfigMon=0;
          EEPROM.write(115,ConfigMon);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>298 && p.x<333 && p.y>551 && p.y<847 && CurrentScreen==5){ // Programar: Terça
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigTue==0){
          ConfigTue=1;
          EEPROM.write(116,ConfigTue);         
        }
        else{
          ConfigTue=0;
          EEPROM.write(116,ConfigTue);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>421 && p.x<471 && p.y>133 && p.y<439 && CurrentScreen==5){ // Programar: Quarta
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigWed==0){
          ConfigWed=1;
          EEPROM.write(117,ConfigWed);         
        }
        else{
          ConfigWed=0;
          EEPROM.write(117,ConfigWed);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>421 && p.x<471 && p.y>557 && p.y<850 && CurrentScreen==5){ // Programar: Quinta
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigThu==0){
          ConfigThu=1;
          EEPROM.write(118,ConfigThu);         
        }
        else{
          ConfigThu=0;
          EEPROM.write(118,ConfigThu);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>558 && p.x<600 && p.y>136 && p.y<430 && CurrentScreen==5){ // Programar: Sexta
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigFri==0){
          ConfigFri=1;
          EEPROM.write(119,ConfigFri);         
        }
        else{
          ConfigFri=0;
          EEPROM.write(119,ConfigFri);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>558 && p.x<600 && p.y>547 && p.y<845 && CurrentScreen==5){ // Programar: Sábado
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigSat==0){
          ConfigSat=1;
          EEPROM.write(120,ConfigSat);         
        }
        else{
          ConfigSat=0;
          EEPROM.write(120,ConfigSat);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>682 && p.x<732 && p.y>136 && p.y<444 && CurrentScreen==5){ // Programar: Domingo
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        
        if(AutoDoseOff==1){
          AutoDoseOff=0;
          EEPROM.write(122,AutoDoseOff);
        }

        if(ConfigSun==0){
          ConfigSun=1;
          EEPROM.write(121,ConfigSun);         
        }
        else{
          ConfigSun=0;
          EEPROM.write(121,ConfigSun);
        }
        
        if(ConfigMon==0 && ConfigTue==0 && ConfigWed==0 && ConfigThu==0 && ConfigFri==0 && ConfigSat==0 && ConfigSun==0 && AutoDoseOff==0){
          AutoDoseOff=1;
          EEPROM.write(122,AutoDoseOff);
        }
        
        Screen5Update();
    }

    if(p.x>682 && p.x<732 && p.y>537 && p.y<842 && CurrentScreen==5){ // Programar: Desligado
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      
      if(AutoDoseOff==0){
          ConfigMon=0;
          ConfigTue=0;
          ConfigWed=0;
          ConfigThu=0;
          ConfigFri=0;
          ConfigSat=0;
          ConfigSun=0;     
          AutoDoseOff=1;
  
          EEPROM.write(115,ConfigMon);
          EEPROM.write(116,ConfigTue);
          EEPROM.write(117,ConfigWed);
          EEPROM.write(118,ConfigThu);
          EEPROM.write(119,ConfigFri);
          EEPROM.write(120,ConfigSat);
          EEPROM.write(121,ConfigSun);  
          EEPROM.write(122,AutoDoseOff);
       }
        
        Screen5Update();              
    }

    if(p.x>385 && p.x<503 && p.y>131 && p.y<436 && CurrentScreen==6){ // Hora Dose 1
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      Hour1Pressed = true;
      Minute1Pressed = false;
      Hour2Pressed = false;
      Minute2Pressed = false;
      Screen6UpdateHour1();
      Screen6UpdateMinute1();
      Screen6UpdateHour2();
      Screen6UpdateMinute2();           
    }

    if(p.x>586 && p.x<714 && p.y>126 && p.y<443 && CurrentScreen==6){ // Minuto Dose 1
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      Hour1Pressed = false;
      Minute1Pressed = true;
      Hour2Pressed = false;
      Minute2Pressed = false;
      Screen6UpdateHour1();
      Screen6UpdateMinute1();
      Screen6UpdateHour2();
      Screen6UpdateMinute2();           
    }    

    if(p.x>389 && p.x<495 && p.y>531 && p.y<858 && CurrentScreen==6){ // Hora Dose 2
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      Hour1Pressed = false;
      Minute1Pressed = false;
      Hour2Pressed = true;
      Minute2Pressed = false;
      Screen6UpdateHour1();
      Screen6UpdateMinute1();
      Screen6UpdateHour2();
      Screen6UpdateMinute2();           
    }

    if(p.x>588 && p.x<722 && p.y>526 && p.y<855 && CurrentScreen==6){ // Minuto Dose 2
      pinMode(XM, OUTPUT);
      pinMode(YP, OUTPUT);
      Hour1Pressed = false;
      Minute1Pressed = false;
      Hour2Pressed = false;
      Minute2Pressed = true;
      Screen6UpdateHour1();
      Screen6UpdateMinute1();
      Screen6UpdateHour2();
      Screen6UpdateMinute2();           
    }
    
    delay(250); 
  }

 }

  void ButtonNextPressed(){
    
// Caso Botão next seja Selecionado
  if(ButtonNext==true){  
     switch (CurrentScreen) 
      {
       case 0:
            CurrentScreen=1;
            Screen1();
       break;                   
       case 1: 
           CurrentScreen=11;
           Screen11();         
       break;              
       case 2:
            CurrentScreen=1;
            Screen1();
       break;                
       case 3:
             CurrentScreen=1;
             Screen1();
       break;                
       case 4:

       break;               
       case 5:
             CurrentScreen=6;
             Screen6();
       break;
       case 6:
             CurrentScreen=1;
             Screen1();
             Hour1Pressed = false;
             Minute1Pressed = false;
             Hour2Pressed = false;
             Minute2Pressed = false;     
       break;
       case 7:
      
       break;
       case 8:
           DayOfWeekPressed=false;
           DayPressed=false;
           CurrentScreen=9;
           Screen9();        
       break;
       case 9:
           MonthPressed=false;
           YearPressed=false;
           CurrentScreen=10;
           Screen10();        
       break;
       case 10:
           HourPressed=false;
           MinutePressed=false;
           CurrentScreen=1;
           Screen1();        
       break;   
       case 11:
           CurrentScreen=1;
           Screen1();        
       break; 
        case 12:
           CurrentScreen=1;
           Screen1();        
       break;                    
        case 13:
           CurrentScreen=1;
           Screen1();        
       break; 
       case 14:
           CurrentScreen=1;
           Screen1();        
       break;                                                                                                
      }          
          ButtonNext=false;                     
  }  
 }

  void ButtonBackPressed(){

// Caso Botão seja for Selecionado
  if(ButtonBack==true){  
     switch (CurrentScreen) 
      {
       case 0:
       break;                           
       case 1:
          CurrentScreen=0;
          Screen0();
       break;              
       case 2:
       break;                
       case 3:
           CurrentScreen=2;
           Screen2();
       break;                
       case 4:
       break;               
       case 5:
           CurrentScreen=2;
           Screen2();            
       break;
       case 6:            
           CurrentScreen=5;
           Screen5();
           Hour1Pressed = false;
           Minute1Pressed = false;
           Hour2Pressed = false;
           Minute2Pressed = false;            
       break;
       case 7:
       break;
       case 8:
          DayOfWeekPressed=false;
          DayPressed=false;
          CurrentScreen=2;
          Screen2();
       break;
       case 9:
          MonthPressed=false;
          YearPressed=false;       
          CurrentScreen=8;
          Screen8();
       break;
       case 10:
          HourPressed=false;
          MinutePressed=false;       
          CurrentScreen=9;
          Screen9();
       break;
          case 11:      

       break;      
                                                                                       
      }          
          ButtonBack=false;
  } 
 }

void ButtonUpPressed(){

// Caso Botão Up seja Selecionado
  if(ButtonUp==true){  
     switch (CurrentScreen) 
      {                   
       case 1:              
       break;              
       case 2:
       break;                
       case 3:
           if(LA_Seq_Delay < 9.9){
              LA_Seq_Delay = LA_Seq_Delay+0.1;
              EEPROM.put(2 + sizeof(int) , LA_Seq_Delay);
              Screen3UpdateTempInterval();
           }  
       break;                
       case 4:            
       break;               
       case 5:
       break;
       case 6:
            if(Hour1Pressed == true){
              if(ConfigHour1<23){ConfigHour1++;}else{ConfigHour1=0;}
              EEPROM.write(123,ConfigHour1);
              Screen6UpdateHour1();
            }
            else if(Minute1Pressed == true){
              if(ConfigMinute1<59){ConfigMinute1++;}else{ConfigMinute1=0;}
              EEPROM.write(124,ConfigMinute1);
              Screen6UpdateMinute1();              
            }            
            else if(Hour2Pressed == true){          
              if(ConfigHour2<23){ConfigHour2++;}else{ConfigHour2=0;}         
              EEPROM.write(125,ConfigHour2);
              Screen6UpdateHour2();              
            }
            else if(Minute2Pressed == true){
              if(ConfigMinute2<59){ConfigMinute2++;}else{ConfigMinute2=0;}         
              EEPROM.write(126,ConfigMinute2);
              Screen6UpdateMinute2();               
            }            
       break;
       case 7:                  
       break;
       case 8: 
          if(DayPressed==true){
            Day++;
            if(Day>=32){Day=1;}
            Screen8UpdateDay();
          }
          if(DayPressed==true){rtc.adjust(DateTime(Year,Month,Day,Hour,Minute,30));}
       break;
       case 9: 
          if(MonthPressed==true){
            Month++;
            if(Month>=13){Month=1;}
            Screen9UpdateMonth();
          }
          if(YearPressed==true){
            Year++;
            Screen9UpdateYear();            
          } 
          if(MonthPressed==true || YearPressed==true){rtc.adjust(DateTime(Year,Month,Day,Hour,Minute,30));}
       break;
       case 10:
          if(HourPressed==true){
            Hour++;
            if(Hour>=24){Hour=0;}
            Screen10UpdateHour();
          }
          if(MinutePressed==true){
            Minute++;
            if(Minute>=60){Minute=0;}
            Screen10UpdateMinute();            
          } 
          if(HourPressed==true || MinutePressed==true){rtc.adjust(DateTime(Year,Month,Day,Hour,Minute,30));}
       break;
       case 11:
          CurrentScreen=12;
          Screen12();
       break;
       case 12:
          CurrentScreen=13;
          Screen13();       
       break;
       case 13:
          CurrentScreen=14;
          Screen14(); 
       break;
       case 14:
          CurrentScreen=11;
          Screen11();       
       break;                                  
      }          
      ButtonUp=false;                
  }  
}


 void ButtonDownPressed(){

// Caso Botão Down seja Selecionado
  if(ButtonDown==true){  
     switch (CurrentScreen) 
      {
       case 1:        
       break;         
       case 2: 
       break;                            
       case 3:
           if(LA_Seq_Delay > 0){
              LA_Seq_Delay = LA_Seq_Delay-0.1;
              EEPROM.put(2 + sizeof(int) , LA_Seq_Delay);
              Screen3UpdateTempInterval();
           } 
       break;                
       case 4:
       break;               
       case 5:
       break;
       case 6:
            if(Hour1Pressed == true){
              if(ConfigHour1>0){ConfigHour1--;}else{ConfigHour1=23;}
              EEPROM.write(123,ConfigHour1);
              Screen6UpdateHour1();
            }
            else if(Minute1Pressed == true){
              if(ConfigMinute1>0){ConfigMinute1--;}else{ConfigMinute1=59;}
              EEPROM.write(124,ConfigMinute1);
              Screen6UpdateMinute1();              
            }            
            else if(Hour2Pressed == true){          
              if(ConfigHour2>0){ConfigHour2--;}else{ConfigHour2=23;}         
              EEPROM.write(125,ConfigHour2);
              Screen6UpdateHour2();              
            }
            else if(Minute2Pressed == true){
              if(ConfigMinute2>0){ConfigMinute2--;}else{ConfigMinute2=59;}         
              EEPROM.write(126,ConfigMinute2);
              Screen6UpdateMinute2();               
            }               
       break; 
       case 7:
       break;
       case 8: 
          if(DayPressed==true){
            Day--;
            if(Day<=-1){Day=31;}
            Screen8UpdateDay();
          }
          if(DayPressed==true){rtc.adjust(DateTime(Year,Month,Day,Hour,Minute,30));}
       break;
       case 9:      
         if(MonthPressed==true){
            Month--;
            if(Month<=-1){Month=12;}
            Screen9UpdateMonth();
         }
         if(YearPressed==true){
           Year--;
           Screen9UpdateYear();            
         } 
         if(MonthPressed==true || YearPressed==true){rtc.adjust(DateTime(Year,Month,Day,Hour,Minute,30));}
       break;
       case 10:
         if(HourPressed==true){
           Hour--;
           if(Hour<=-1){Hour=23;}
           Screen10UpdateHour();
         }
         if(MinutePressed==true){
           Minute--;
           if(Minute<=-1){Minute=59;}
           Screen10UpdateMinute();            
         } 
         if(HourPressed==true || MinutePressed==true){rtc.adjust(DateTime(Year,Month,Day,Hour,Minute,30));}
       break;
       case 11:
          CurrentScreen=14;
          Screen14();
       break;
       case 12:
          CurrentScreen=11;
          Screen11();       
       break;
       case 13:
          CurrentScreen=12;
          Screen12(); 
       break;
       case 14:
          CurrentScreen=13;
          Screen13();       
       break;                                                                                       
      }          
     ButtonDown=false;               
  }  
} 

void GiveDoseWithoutRTC(){
      LastDoseTimeCounter = LastDoseTimeCounter+1;
      if( LastDoseTimeCounter >= TimeBetweenDoses){
         LastDoseTimeCounter=0;
         if(Flg_RTCAlarm == true){boGiveDose = true;}     
      }   
}
   
//************************************************* Função Apenas Chamada Programar Cordenadas para Touch Screen *********************************************
 void TouchButtonTeach(){
      TSPoint p = ts.getPoint();
        if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
          Serial.print("X= ");      
          Serial.println(p.x);
          Serial.print("Y= ");
          Serial.println(p.y); 
          delay(1000);
        }
  }

 
