/*
  Example: Portable Phone Detector - Version 1.0.0 - 2018/04/11
  Author: Manuel Ballesteros

  General Description: 
    System that detects a mobile call near in electromagnetic spectrum

  Circuit Description:
   * RF Explorer 3G+ IoT for Arduino. More information in:www.rf-explorer.com/IoT    
   * UHF Antenna on RF IN
   * Arduino DUE
   * USB cable for programming and monitoring
   * Buzzer on pin A5

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

//Function to use buzzerr
void ToneBuzzer(byte pin, uint16_t frequency, uint16_t duration)
{ 
    unsigned long startTime=millis();
    unsigned long halfPeriod= 1000000L/frequency/2;
    pinMode(A5,OUTPUT);
    while (millis()-startTime< duration)
    {
        digitalWrite(A5,HIGH);
        delayMicroseconds(halfPeriod);
        digitalWrite(A5,LOW);
        delayMicroseconds(halfPeriod);
    }
    pinMode(A5,INPUT);
}

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
                    ToneBuzzer(A5,1000, 2000); // tone on pin-8 with 1000 Hz for 2000 milliseconds
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
}
