/*
  Example: Portable Phone Detector - Version 1.0.0 - 2018/04/21
  Author: Manuel Ballesteros

  General Description: 
    System that detects a mobile call near in electromagnetic spectrum

  Circuit Description:
   * RF Explorer 3G+ IoT for Arduino. More information in:www.rf-explorer.com/IoT    
   * UHF Antenna on RF IN
   * Arduino DUE
   * USB cable for programming and monitoring
   * Speaker Input on Pin DAC0

*/

#include <RFExplorer_3GP_IoT.h>

//Constants of initial and end frequency. More info about gsm frequencies in: https://en.wikipedia.org/wiki/GSM_frequency_bands
//In these example I use Uplink frequencies (Mobile to base) of 900 Band for Vodafone Operator  - Uplink  904,9-914,9 MHz Downlink 949,9-959,9 MHz from: https://wiki.bandaancha.st/Frecuencias_telefon%C3%ADa_m%C3%B3vil
#define _START_FREQ_KHZ 905000
#define _STOP_FREQ_KHZ  915000       

#if _START_FREQ_KHZ>=_STOP_FREQ_KHZ
    #error PLEASE SPECIFY VALID FREQUENCY RANGE
#endif

#if ((_START_FREQ_KHZ < _MIN_FREQUENCY_KHZ) || (_STOP_FREQ_KHZ>_MAX_FREQUENCY_KHZ))
    #error PLEASE SPECIFY VALID FREQUENCY RANGE
#endif

RFExplorer_3GP_IoT g_objRF;                      //Global 3G+ object for access to all RF Explorer IoT functionality
unsigned short int g_nProcessResult=_RFE_IGNORE; //Global variable for result in method processReceivedString()

static int arrSinTable[120]=
{
    0x7ff, 0x86a, 0x8d5, 0x93f, 0x9a9, 0xa11, 0xa78, 0xadd, 0xb40, 0xba1,
    0xbff, 0xc5a, 0xcb2, 0xd08, 0xd59, 0xda7, 0xdf1, 0xe36, 0xe77, 0xeb4,
    0xeec, 0xf1f, 0xf4d, 0xf77, 0xf9a, 0xfb9, 0xfd2, 0xfe5, 0xff3, 0xffc,
    0xfff, 0xffc, 0xff3, 0xfe5, 0xfd2, 0xfb9, 0xf9a, 0xf77, 0xf4d, 0xf1f,
    0xeec, 0xeb4, 0xe77, 0xe36, 0xdf1, 0xda7, 0xd59, 0xd08, 0xcb2, 0xc5a,
    0xbff, 0xba1, 0xb40, 0xadd, 0xa78, 0xa11, 0x9a9, 0x93f, 0x8d5, 0x86a,
    0x7ff, 0x794, 0x729, 0x6bf, 0x655, 0x5ed, 0x586, 0x521, 0x4be, 0x45d,
    0x3ff, 0x3a4, 0x34c, 0x2f6, 0x2a5, 0x257, 0x20d, 0x1c8, 0x187, 0x14a,
    0x112, 0xdf,  0xb1,  0x87,  0x64,  0x45, 0x2c, 0x19, 0xb, 0x2,
    0x0, 0x2, 0xb, 0x19, 0x2c, 0x45, 0x64, 0x87, 0xb1, 0xdf,
    0x112, 0x14a, 0x187, 0x1c8, 0x20d, 0x257, 0x2a5, 0x2f6, 0x34c, 0x3a4,
    0x3ff, 0x45d, 0x4be, 0x521, 0x586, 0x5ed, 0x655, 0x6bf, 0x729, 0x794
 };
int i = 0;
int step1k=4; // 4 ms for 250Hz
long nxt_time = 0;
bool bSound = true;

void setup()
{ 
    digitalWrite(_RFE_GPIO2, LOW);      //Set _RFE_GPIO2 as output, LOW -> Device mode to 2400bps
    g_objRF.resetHardware();            //Reset 3G+ board
    
    delay(5000);                        //Wait for 3G+ to complete initialization routines
    g_objRF.init();                     //Initialize 3G+ library - Monitor SerialDebugger set 57600bps
    g_objRF.changeBaudrate(115200);     //Change baudrate to 115Kbps, max reliable in Arduino DUE.
    delay(1000);                        //Wait 1sec to stablish communication
    digitalWrite(_RFE_GPIO2, HIGH);     
    pinMode(_RFE_GPIO2, INPUT_PULLUP);  //Set _RFE_GPIO2 as a general port, no longer needed after start completed

    analogWriteResolution(12);  // set the analog output resolution to 12 bit (4096 levels)
    g_objRF.requestConfig();            //Request of current configuration to 3G+ -> Device starts to send it default setup and them SweepData
    
    //Wait for message received is Default Config from 3G+
    do
    {
        g_objRF.updateBuffer();
        g_nProcessResult = g_objRF.processReceivedString();  
    }
    while(!((g_nProcessResult == _RFE_SUCCESS) && (g_objRF.getLastMessage() == _CONFIG_MESSAGE)));
     
    //Send Command to change RF module configuration
    g_objRF.sendNewConfig(_START_FREQ_KHZ, _STOP_FREQ_KHZ);  

    //Wait for message received is User Config from 3G+
    do
    {
        g_objRF.updateBuffer();
        g_nProcessResult = g_objRF.processReceivedString();  
    }
    while(!((g_nProcessResult == _RFE_SUCCESS) && (g_objRF.getLastMessage() == _CONFIG_MESSAGE)));

    //Display on Serial Monitor new Start/Stop KHZ range here from the new configuration
    g_objRF.getMonitorSerial().println("New Config");
    g_objRF.getMonitorSerial().print("StartKHz: "); 
    g_objRF.getMonitorSerial().println(g_objRF.getConfiguration()->getStartKHZ());
    g_objRF.getMonitorSerial().print("StopKHz:  "); 
    g_objRF.getMonitorSerial().println(g_objRF.getConfiguration()->getEndKHZ()); 
}

void loop() 
{   
    //Call to these two functions to refresh library received data.
    g_objRF.updateBuffer();
    unsigned short int nProcessResult = g_objRF.processReceivedString(); 
    
    if (nProcessResult == _RFE_SUCCESS) 
    {
        if((g_objRF.getLastMessage() == _SWEEP_MESSAGE) && g_objRF.isValid()) 
        {
            //Sweep data
            unsigned long int nFreqPeakKHZ=0;                                      
            short int nPeakDBM=0;
            if (g_objRF.getPeak(&nFreqPeakKHZ, &nPeakDBM) ==_RFE_SUCCESS)           
            {
                //Display on Serial Monitor frequency and amplitude of the signal peak
                g_objRF.getMonitorSerial().print(nFreqPeakKHZ);
                g_objRF.getMonitorSerial().print(" KHz to ");
                g_objRF.getMonitorSerial().print(nPeakDBM);
                g_objRF.getMonitorSerial().println(" dBm"); 

                if(nPeakDBM > -75)
                {
                    bSound = true;
                }
            }
        }
    }
    else
    {
        // _RFE_IGNORE or _RFE_NOT_MESSAGE are not errors, it just mean a new message was not available
        if ((nProcessResult != _RFE_IGNORE) && (nProcessResult != _RFE_NOT_MESSAGE))
        {
            //Report error information
            g_objRF.getMonitorSerial().print("Error:");
            g_objRF.getMonitorSerial().println(nProcessResult);
        }
    }

  //Use Speaker to indicate Peak
    if( millis()>nxt_time && (bSound))
    {
        nxt_time=millis()+step1k;
        analogWrite(DAC0, arrSinTable[i]);  
        i++;
        if(i == 120) 
            i = 0;
    }
    else if (!bSound)
    {
        //analogWrite(DAC0, 0x00);  
        pinMode(DAC0, INPUT);
    }
}
