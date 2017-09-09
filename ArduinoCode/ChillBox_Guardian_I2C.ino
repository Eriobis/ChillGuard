
#include <U8x8lib.h>
#include <Wire.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#define FLOW_METER_PIN          3
#define COOLER_ENABLE_PIN       7
#define SYS_RESET_BUTTON_PIN    4
#define ENCODER_A_PIN           5
#define ENCODER_B_PIN           6

#define FLOW_MIN                1.00l
#define FLOW_SAMPLE_PERIOD      1000    // 1 sec
#define DISPLAY_PERIOD          500     // 500 ms
#define ENCODER_READ_PERIOD     50      // 50 ms

#define PULSE_PER_MIN           3600*(60*(FLOW_SAMPLE_PERIOD/1000))
typedef enum __SYS_State_e
{
    SYS_STOPPED,
    SYS_STARTED,
    SYS_RESTART_ASKED,
}SYS_State_e;

//NOTE : 1 Liter = 5880 pulses

// Global variables
uint32_t DisplayRefreshTimer;
uint32_t FlowPulseCount;
uint32_t TotalPulseCount;
uint32_t FlowSoftTimer;
uint16_t FlowPPS;
float Flow;
char strBuff[16];
uint32_t EncoderReadTimer;
uint32_t EncoderPulseCount;
SYS_State_e SysState;
uint8_t I2C_RequestCommand;

//Oled Display Object
U8X8_SSD1306_128X64_NONAME_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

void setup() {
    FlowPulseCount = 0;
    TotalPulseCount = 0;
    pinMode(COOLER_ENABLE_PIN, OUTPUT);
    pinMode(FLOW_METER_PIN, INPUT);
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

    EnableCooler();
    delay(2000);        //Give 2 sec to the system to start 

    //Init I2C bus
    Wire.begin(8);                // join i2c bus with address #8
    Wire.onReceive(receiveEvent); // register event
    Wire.onRequest(i2cRequest);

    //Init software timers
    FlowSoftTimer = millis();
    DisplayRefreshTimer = millis();
    EncoderReadTimer = millis();

    //Attach pulse count from the flow meter
    attachInterrupt(digitalPinToInterrupt(FLOW_METER_PIN), FlowPulseHandler, RISING);

    //Init Serial debug
    Serial.begin(115200);
    
    //Init Display
    u8x8.begin();
    u8x8.setPowerSave(0);
}

void loop() {

    if ( millis() - EncoderPulseCount > ENCODER_READ_PERIOD )
    {
        int btnPressed = digitalRead(SYS_RESET_BUTTON_PIN);
        if ( btnPressed && (SysState == SYS_STOPPED ))
        {
            SysState = SYS_RESTART_ASKED;
        }
    }

    //Flow calculation from pulse to L/sec    
    if ( millis() - FlowSoftTimer > FLOW_SAMPLE_PERIOD )
    {
        //Flow here is pulse/seconds
        FlowPPS = ((float)FlowPulseCount/FLOW_SAMPLE_PERIOD)*1000;
        FlowPulseCount = 0;
        FlowSoftTimer = millis();
        
        //Flow here is liter/minute, as we need 98 pulses per sec to make 1L/sec
        Flow = (float)FlowPPS/(float)PULSE_PER_MIN;

        Serial.print("{");
        Serial.print("\"FlowPPS\" : ");
        Serial.print(FlowPPS);
        Serial.print(", \"Flow\" : ");
        Serial.print(Flow);
        Serial.print(", \"State\" : ");
        Serial.print(SysState);
        Serial.println("}");
        
        dtostrf(Flow,4,2,strBuff);
        u8x8.setFont(u8x8_font_victoriabold8_r);
        u8x8.drawString(0,2,strBuff);
        u8x8.drawString(6,2,"L/min");

    }

    //Flow Monitoring
    if ( Flow < FLOW_MIN )
    {
        u8x8.drawString(0,1,"System FAILURE!");
        SysState = SYS_STOPPED;
        DisableCooler();
    }
    else{
        u8x8.drawString(0,1,"System OKAY");
        //Serial.println("Restarting system ...");
        SysState = SYS_STARTED;
        EnableCooler();
    }

    //Show Temperatures on display
    if ( millis() - DisplayRefreshTimer > DISPLAY_PERIOD )
    {
        u8x8.drawString(0,3,"Hot:");
        u8x8.drawString(7,3,"°C");
        u8x8.drawString(0,4,"Amb:");
        u8x8.drawString(7,4,"°C");
        u8x8.drawString(0,5,"");
        
        u8x8.drawString(0,7,"Line 7");
        DisplayRefreshTimer = millis();
    }
}

void EnableCooler()
{
    digitalWrite(COOLER_ENABLE_PIN, HIGH);
}

void DisableCooler()
{
    digitalWrite(COOLER_ENABLE_PIN, LOW);
}

//Pulse Interrupt
void FlowPulseHandler()
{
    FlowPulseCount ++;
    TotalPulseCount ++;
}

// function that executes whenever data is received from master
void receiveEvent(int howMany) {
    uint8_t I2C_Command;

    while (Wire.available()) 
    {   // loop through all but the last
        I2C_Command = Wire.read();
    }

    if (I2C_Command > 0) { // We have indeed received a valid command
        switch (I2C_Command){
     
        case 0x01:
            Serial.println("Setting Infos Register");
            // Store the actual request so that the onRequest handler knows
            // what to do
            I2C_RequestCommand = I2C_Command;
            break;
        
        case 0x02:
            Serial.println("Setting Flow Register");
            // Store the actual request so that the onRequest handler knows
            // what to do
            I2C_RequestCommand = I2C_Command;
            break;

        case 0x03:
            Serial.println("Setting Temperature Register");
            // Store the actual request so that the onRequest handler knows
            // what to do
            I2C_RequestCommand = I2C_Command;
            break;

        case 0x04:
            Serial.println("Setting Cooler Register");
            // Store the actual request so that the onRequest handler knows
            // what to do
            I2C_RequestCommand = I2C_Command;
            break;
            default:
            break;
        }
     
        I2C_Command = 0;
      }
}

void i2cRequest(void){

    Serial.println("OnRequestHandler");
    if (I2C_RequestCommand > 0) 
    { // We have indeed received a valid command
        switch (I2C_RequestCommand){
     
        case 0x01:
            Serial.println("Sending Infos");

            break;
        
        case 0x02:
            Serial.println("Sending Flow");
            //Wire.write(0x50);
            Wire.print(Flow,2);
            break;

        case 0x03:
            Serial.println("Sending Temperature");

            break;

            default:
            break;
        }
    }
}