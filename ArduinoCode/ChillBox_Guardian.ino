
#include <U8x8lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

#define FLOW_METER_PIN          3
#define COOLER_ENABLE_PIN       7
#define SYS_RESET_BUTTON_PIN    4
#define ENCODER_A_PIN           5
#define ENCODER_B_PIN           6
#define PUMP_ENABLE_PIN         4

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

//Screen variables
uint8_t cursorPos;
uint8_t currentPage;

//Timers
uint32_t DisplayRefreshTimer;
uint32_t EncoderReadTimer;
uint32_t FlowSoftTimer;

//Counters
uint32_t FlowPulseCount;
uint32_t TotalPulseCount;
uint32_t EncoderPulseCount;

//Sensor Variables
uint16_t FlowPPS;
float Flow;
char strBuff[16];
SYS_State_e SysState;

//Sys status variables
bool PumpStatus;
bool CoolerStatus;

//Oled Display Object
U8X8_SSD1306_128X64_NONAME_4W_SW_SPI u8x8(/* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

void setup() {
    FlowPulseCount = 0;
    TotalPulseCount = 0;
    pinMode(COOLER_ENABLE_PIN, OUTPUT);
    pinMode(FLOW_METER_PIN, INPUT);
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

    EnablePump();
    EnableCooler();
    delay(2000);        //Give 2 sec to the system to start 

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

        // JSON encoding of our values
        //Start of json object
        Serial.print("{");

        Serial.print("\"FlowPPS\" : ");
        Serial.print(FlowPPS);
        Serial.print(", \"Flow\" : ");
        Serial.print(Flow);
        Serial.print(", \"State\" : ");
        Serial.print(SysState);
        Serial.print(", \"Pump\" : ");
        Serial.print(PumpStatus);
        Serial.print(", \"Cooler\" : ");
        Serial.print(CoolerStatus);

        //End of json object
        Serial.println("}");
        
    }

    //Flow Monitoring
    if ( Flow < FLOW_MIN )
    {
        u8x8.drawString(0,1,"System FAILURE!");
        SysState = SYS_STOPPED;
        DisableCooler();
    }
    else{
        u8x8.drawString(0,1,"System OKAY!    ");
        //Serial.println("Restarting system ...");
        SysState = SYS_STARTED;
        EnableCooler();
    }

    //Show Temperatures on display
    if ( millis() - DisplayRefreshTimer > DISPLAY_PERIOD )
    {
        dtostrf(Flow,4,2,strBuff);
        u8x8.setFont(u8x8_font_victoriabold8_r);
        //Flow
        u8x8.drawString(0,2,strBuff);
        u8x8.drawString(6,2,"L/min");
        //Rad hot side temp
        u8x8.drawString(0,3,"Hot:");
        u8x8.drawString(7,3,"°C");
        //Ambient temp
        u8x8.drawString(0,4,"Amb:");
        u8x8.drawString(7,4,"°C");
        //Pump Status
        u8x8.drawString(0,5,"Pump: ");
        dtostrf(PumpStatus,4,2,strBuff);
        u8x8.drawString(7,5,strBuff);
        //Spare line
        u8x8.drawString(0,6,"Cooler:");
        dtostrf(CoolerStatus,4,2,strBuff);
        u8x8.drawString(8,6,strBuff);
        u8x8.drawString(0,7,"-");
        DisplayRefreshTimer = millis();
    }
}

void EnableCooler()
{
    CoolerStatus = true;
    digitalWrite(COOLER_ENABLE_PIN, HIGH);
}

void DisableCooler()
{
    CoolerStatus = false;
    digitalWrite(COOLER_ENABLE_PIN, LOW);
}

void EnablePump()
{
    PumpStatus = true;
    digitalWrite(PUMP_ENABLE_PIN, HIGH);
}

void DisablePump()
{
    PumpStatus = false;
    digitalWrite(PUMP_ENABLE_PIN, LOW);
}

//Pulse Interrupt
void FlowPulseHandler()
{
    FlowPulseCount ++;
    TotalPulseCount ++;
}

