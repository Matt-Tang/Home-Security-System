#include "main.h"
#include "driverlib/driverlib.h"
#include "hal_LCD.h"
#include <msp430.h>
#include <stdio.h>
#include <stdlib.h>
/*
 * This project contains some code samples that may be useful.
 *
 */

char ADCState = 0; //Busy state of the ADC
int16_t ADCResult = 0; //Storage for the ADC conversion result
int RTC_COUNTER = 1;

#define ARMED 1
#define NOT_ARMED 0

#define ZONE_OK 'G'
#define ZONE_NOT_OK 'B'

/* Armed = 1, Not Armed = 0 */
int ZONE_1_ARMED_STATUS = ARMED;
int ZONE_2_ARMED_STATUS = ARMED;
int ZONE_3_ARMED_STATUS = ARMED;
int ZONE_4_ARMED_STATUS = ARMED;

/* Status is ZONE_OK or ZONE_NOT_OK */
int ZONE_A_SOUND_STATUS = 0;
int ZONE_1_STATUS = 0;
int ZONE_2_STATUS = 0;
int ZONE_3_STATUS = 0;
int ZONE_4_STATUS = 0;

/* Status to be displayed on LCD */
char ZONE_1_STATUS_DISPLAY = ZONE_OK;
char ZONE_2_STATUS_DISPLAY = ZONE_OK;
char ZONE_3_STATUS_DISPLAY = ZONE_OK;
char ZONE_4_STATUS_DISPLAY = ZONE_OK;

/* Enable/Disable Alarm Flag, initially enabled */
int ALARM_FLAG = 1; // 1 = enable, 0 = disable

/* Lock for LCD Display */
int ZONE_1_LOCK_STATUS = 0;
int ZONE_2_LOCK_STATUS = 0;
int ZONE_3_LOCK_STATUS = 0;
int ZONE_4_LOCK_STATUS = 0;

/* Display LCD Flag */
int DISPLAY_LCD_FLAG = 0;

/* MIC Flag */
int MIC_FLAG = 0;

char command[50] = {0};
int commandPosition = 0;
char currentTime[4] = {0};
int noiseLevel[30] = {0};
int noiseLevelCounter = 0;
int armTime[4] = {5000, 5000, 5000, 5000};
int disarmTime[4] = {6000, 6000, 6000, 6000};
int MEASURE_FLAG = 0;


void main(void)
    {
    __disable_interrupt();
    WDT_A_hold(WDT_A_BASE); // stop watch dog

    // Initializations - see functions for more detail
    Init_GPIO();    //Sets all pins to output low as a default
    Init_PWM();     //Sets up a PWM output
    Init_ADC();     //Sets up the ADC to sample
    Init_Clock();   //Sets up the necessary system clocks
    Init_UART();    //Sets up an echo over a COM port
    Init_LCD();     //Sets up the LaunchPad LCD display
    Init_RTC();
   
    PMM_unlockLPM5(); //Disable the GPIO power-on default high-impedance mode to activate previously configured port settings

    // Initialize Button Interrupt
    P1IE |= GPIO_PIN2;
    P1IFG &= ~GPIO_PIN2;

    P2IE |= GPIO_PIN6;
    P2IFG &= ~GPIO_PIN6;

    //All done initializations - turn interrupts back on.
    __enable_interrupt();

    displayScrollText("ECE 298");
    displayZoneStatusConditions(ZONE_1_STATUS_DISPLAY, ZONE_2_STATUS_DISPLAY, ZONE_3_STATUS_DISPLAY, ZONE_4_STATUS_DISPLAY);

    //Start Timer
    RTC_start(RTC_BASE, RTC_CLOCKSOURCE_XT1CLK);

    /* Initialize Enable bit as Output Port */
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4);
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5);

    /* Initialize Port 8.1 as MIC INPUT */
    GPIO_setAsInputPin(GPIO_PORT_P8, GPIO_PIN1);

    /* Declare variables */
    int counter = 0;
    int enableBits [3];
    int i = 0;
    int MIC_THRESHOLD = 480;
    ADCResult = 430;
    
    while(1)
    {
        /* Reset variables */
        counter = 0;
        enableBits[2] = 0;
        enableBits[1] = 0;
        enableBits[0] = 0;

        //----------------------- MIC -----------------------------
        //Start an ADC conversion (if it's not busy) in Single-Channel, Single Conversion Mode
        if (ADCState == 0)
        {
            ADCState = 1; //Set flag to indicate ADC is busy - ADC ISR (interrupt) will clear it
            ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
        }


        if(noiseLevelCounter < 30)
        {
            noiseLevel[noiseLevelCounter] = ADCResult;
            noiseLevelCounter++;
        }
        else // take an average of 30
        {
            int k;
            for(k = 0; k < 30; k++)
            {
                MIC_THRESHOLD += noiseLevel[k];
            }
            for(k = 0; k < 30; k++)
            {
                noiseLevel[k] = 0;
            }
            MIC_THRESHOLD = MIC_THRESHOLD / 30;
            MEASURE_FLAG = 1;
            noiseLevelCounter = 0;
        }

        // if (ADCResult < MIC_THRESHOLD)
        // {
        //     MIC_FLAG = 1;
        //     MEASURE_FLAG = 0;
        // }

        if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) > 150))
        {
            // if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) < 150))
            // {
            //     MEASURE_FLAG = 0;
            // }

            if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) > 175))
            {
                MIC_FLAG = 1;
                MEASURE_FLAG = 0;
            }
          
        }
    
        // if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) > 200))
        // {
        //     // if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) < 60))
        //     // {
        //     //     MEASURE_FLAG = 0;
        //     // }
        //     // if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) < 70))
        //     // {
        //     //     MEASURE_FLAG = 0;
        //     // }
        //     // if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) < 40))
        //     // {
        //     //     MEASURE_FLAG = 0;
        //     // }
        //     // if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) < 45))
        //     // {
        //     //     MEASURE_FLAG = 0;
        //     // }
        //     if (MEASURE_FLAG == 1 && (abs(MIC_THRESHOLD - ADCResult) > 200))
        //     {
        //         MIC_FLAG = 1;
        //         MEASURE_FLAG = 0;
        //     }
        // }

        for (i = 0; i <= 7; i++)
        {
            
            if (MIC_FLAG == 1)
            {
                if (ALARM_FLAG == 0 || MIC_FLAG == 0) // MIC_FLAG == 0 from UART INTERRUPT COMMAND (BUZZEROFF)
                {
                    MIC_FLAG = 0; //SET MIC FLAG to ZERO, SO CONDITION NOT MET ANYMORE
                    //ALARM_FLAG = 1;
                }else if(ZONE_2_ARMED_STATUS == ARMED) {
                    ZONE_2_STATUS = 1; // there was a problem
                    ZONE_2_STATUS_DISPLAY = ZONE_NOT_OK;
                    ZONE_2_LOCK_STATUS = 1;
                }
            }

            enableBits[2] = (counter>>2)&0x1;
            enableBits[1] = (counter>>1)&0x1;
            enableBits[0] =  counter&0x1;
            counter++;

            if(enableBits[2] == 0) /* LEDS */
            {
                GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN7);
            }else{
                GPIO_setAsInputPin(GPIO_PORT_P2, GPIO_PIN7);
            }
            
            setEnableBitOutput(enableBits[2], enableBits[1], enableBits[0]);

            __delay_cycles(1000);

            /* LED 3 */
            if (enableBits[2] == 0 && enableBits[1] == 0 && enableBits[0] == 0)
            {
                if(ZONE_3_ARMED_STATUS == ARMED)
                {
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
                }
                else if(ZONE_3_ARMED_STATUS == NOT_ARMED)
                {
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
                }
            }
            /* LED 2 */
            if (enableBits[2] == 0 && enableBits[1] == 0 && enableBits[0] == 1)
            {
                if(ZONE_2_ARMED_STATUS == ARMED)
                {
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
                }
                else if(ZONE_2_ARMED_STATUS == NOT_ARMED)
                {
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
                }
            }
            /* LED 1 */
            // if (enableBits[2] == 0 && enableBits[1] == 1 && enableBits[0] == 0)
            // {
            //     if(ZONE_1_ARMED_STATUS == ARMED)
            //     {
            //         GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
            //     }
            //     else if(ZONE_1_ARMED_STATUS == NOT_ARMED)
            //     {
            //         GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
            //     }
            // }
            /* LED 4 */
            if (enableBits[2] == 0 && enableBits[1] == 1 && enableBits[0] == 1)
            {
                if(ZONE_4_ARMED_STATUS == ARMED)
                {
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN7);
                }
                else if(ZONE_4_ARMED_STATUS == NOT_ARMED)
                {
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
                }
            }

            if(enableBits[2] == 1 && enableBits[1] == 0 && enableBits[0] == 0 && ZONE_1_LOCK_STATUS == 0 && ZONE_1_ARMED_STATUS == ARMED) /* Reed 1 */
            {
                ZONE_1_STATUS = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN7);
                if(ZONE_1_STATUS == 1) // Status = 1 means ZONE_NOT_OK
                {
                    ZONE_1_STATUS_DISPLAY = ZONE_NOT_OK;
                    ZONE_1_LOCK_STATUS = 1;
                }
                else // Status = 0 means ZONE_OK
                {
                    ZONE_1_STATUS_DISPLAY = ZONE_OK;
                } 
            }

            if(enableBits[2] == 1 && enableBits[1] == 0 && enableBits[0] == 1 && ZONE_4_LOCK_STATUS == 0 && ZONE_4_ARMED_STATUS == ARMED) /* Reed 4 */
            {
                ZONE_4_STATUS = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN7);
                if(ZONE_4_STATUS == 1) // Status = 1 means ZONE_NOT_OK
                {
                    ZONE_4_STATUS_DISPLAY = ZONE_NOT_OK;
                    ZONE_4_LOCK_STATUS = 1;
                }
                else // Status = 0 means ZONE_OK
                {
                    ZONE_4_STATUS_DISPLAY = ZONE_OK;
                }
            }

            if(enableBits[2] == 1 && enableBits[1] == 1 && enableBits[0] == 0 && ZONE_2_LOCK_STATUS == 0 && ZONE_2_ARMED_STATUS == ARMED) /* Reed 2 */
            {
                ZONE_2_STATUS = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN7);
                if(ZONE_2_STATUS == 1) // Status = 1 means ZONE_NOT_OK
                {
                    ZONE_2_STATUS_DISPLAY = ZONE_NOT_OK;
                    ZONE_2_LOCK_STATUS = 1;
                }
                else // Status = 0 means ZONE_OK
                {
                    ZONE_2_STATUS_DISPLAY = ZONE_OK;
                }
            }

            if(enableBits[2] == 1 && enableBits[1] == 1 && enableBits[0] == 1 && ZONE_3_LOCK_STATUS == 0 && ZONE_3_ARMED_STATUS == ARMED) /* Reed 3 */
            {
                ZONE_3_STATUS = GPIO_getInputPinValue(GPIO_PORT_P2, GPIO_PIN7);
                if(ZONE_3_STATUS == 1) // Status = 1 means ZONE_NOT_OK
                {
                    ZONE_3_STATUS_DISPLAY = ZONE_NOT_OK;
                    ZONE_3_LOCK_STATUS = 3;
                }
                else // Status = 0 means ZONE_OK
                {
                    ZONE_3_STATUS_DISPLAY = ZONE_OK;
                }
            }

            if(DISPLAY_LCD_FLAG == 0) // LCD is displaying STATUS CONDITIONS
            {
                displayZoneStatusConditions(ZONE_1_STATUS_DISPLAY, ZONE_2_STATUS_DISPLAY, ZONE_3_STATUS_DISPLAY, ZONE_4_STATUS_DISPLAY);
            }
        }

        if((ZONE_1_STATUS || ZONE_2_STATUS || ZONE_3_STATUS || ZONE_4_STATUS) && ALARM_FLAG){
            Timer_A_outputPWM(TIMER_A0_BASE, &param); /* Play Alarm By Turning On PWM */
            // __delay_cycles(1000);
            // Timer_A_stop(TIMER_A0_BASE); //Shut off PWM signal
        } else if(ZONE_1_STATUS == 0 && ZONE_2_STATUS == 0 && ZONE_3_STATUS == 0 && ZONE_4_STATUS == 0){
            Timer_A_stop(TIMER_A0_BASE); //Shut off PWM signal
        }
    }

    //------------------------------------- MIC Code -----------------------------------
    while (1)
    {
        //read MIC inputs
        //Start an ADC conversion (if it's not busy) in Single-Channel, Single Conversion Mode
        if (ADCState == 0)
        {
            ADCState = 1; //Set flag to indicate ADC is busy - ADC ISR (interrupt) will clear it
            ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
        }

        if (ADCResult < MIC_THRESHOLD)
        {
            MIC_FLAG = 1;
        }

        if (MIC_FLAG == 1)
        {
            //sound (planning on using the pwm module;
            while (1)
            {
                if (ALARM_FLAG == 0 || MIC_FLAG == 0) // MIC_FLAG == 0 from UART INTERRUPT COMMAND (BUZZEROFF)
                {
                    MIC_FLAG = 0;
                    //ALARM_FLAG = 1;
                    break;
                }
                Timer_A_outputPWM(TIMER_A0_BASE, &param); /* Play Alarm By Turning On PWM */
                __delay_cycles(10000);
                Timer_A_stop(TIMER_A0_BASE);
            }
        }

    }
}

/*
 * Real Time Clock counter Initialization
 */
void Init_RTC()
{
    // RTC runs at a speed of 32768 Hz, so
    // Set RTC modulo to 32764 to trigger interrupt every ~1s
    RTC_init(RTC_BASE, 32764, RTC_CLOCKPREDIVIDER_1);
    RTC_enableInterrupt(RTC_BASE, RTC_OVERFLOW_INTERRUPT);
}


/*
 * RTC Interrupt Service Routine
 * Wakes up every ~10 milliseconds to update stowatch
 */
#pragma vector = RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    RTC_COUNTER += 1;
    if(DISPLAY_LCD_FLAG == 1)
    {
        showChar((RTC_COUNTER/100000)%10 + '0', pos1);
        showChar((RTC_COUNTER/10000)%10 + '0', pos2);
        showChar((RTC_COUNTER/1000)%10 + '0', pos3);
        showChar((RTC_COUNTER/100)%10 + '0', pos4);
        showChar((RTC_COUNTER/10)%10 + '0', pos5);
        showChar(RTC_COUNTER%10 + '0', pos6);
    }

    // if(RTC_COUNTER % armTime[0] == 0){ // Arm zone 1 at a specific time
    //     ZONE_1_ARMED_STATUS = ARMED;
    // } 

    if(RTC_COUNTER % armTime[1] == 0){ // Arm zone 2 at a specific time
        ZONE_2_ARMED_STATUS = ARMED;
    } 

    if(RTC_COUNTER % armTime[2] == 0){ // Arm zone 3 at a specific time
        ZONE_3_ARMED_STATUS = ARMED;
    } 

    if(RTC_COUNTER % armTime[3] == 0){ // Arm zone 4 at a specific time
        ZONE_4_ARMED_STATUS = ARMED;
    } 

    // if(RTC_COUNTER % disarmTime[0] == 0){ // Disarm zone 1 at a specific time
    //     ZONE_1_ARMED_STATUS = NOT_ARMED;
    // }

    if(RTC_COUNTER % disarmTime[1] == 0){ // Disarm zone 2 at a specific time
        ZONE_2_ARMED_STATUS = NOT_ARMED;
    } 

    if(RTC_COUNTER % disarmTime[2] == 0){ // Disarm zone 3 at a specific time
        ZONE_3_ARMED_STATUS = NOT_ARMED;
    } 

    if(RTC_COUNTER % disarmTime[3] == 0){ // Disarm zone 4 at a specific time
        ZONE_4_ARMED_STATUS = NOT_ARMED;
    }  
    RTC_clearInterrupt(RTC_BASE, RTC_OVERFLOW_INTERRUPT);
}

//Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
    DISPLAY_LCD_FLAG ^= 1;
    clearLCD();
    __delay_cycles(2000);
    P1IFG &= ~GPIO_PIN2;
}

//Port 2 interrupt service routine, for shutting down pwm
#pragma vector = PORT2_VECTOR
__interrupt void Port_2(void)
{
    P2IFG &= ~GPIO_PIN6;
    ALARM_FLAG ^= 1;
    if (ALARM_FLAG == 0) // means that alarm flag is initially 1
    {
        //shut off pwm signal
        Timer_A_stop(TIMER_A0_BASE); //Shut off PWM signal
    }
}

void displayZoneStatusConditions(char zoneA, char zoneB, char zoneC, char zoneD){
    showChar(zoneA,pos1);
    showChar(zoneB,pos2);
    showChar(zoneC,pos3);
    showChar(zoneD,pos4);
}

void Init_GPIO(void)
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);

    //Set LaunchPad switches as inputs - they are active low, meaning '1' until pressed
    GPIO_setAsInputPinWithPullUpResistor(SW1_PORT, SW1_PIN);
    GPIO_setAsInputPinWithPullUpResistor(SW2_PORT, SW2_PIN);

    //Set LED1 and LED2 as outputs
    //GPIO_setAsOutputPin(LED1_PORT, LED1_PIN); //Comment if using UART
    GPIO_setAsOutputPin(LED2_PORT, LED2_PIN);
}

/* Clock System Initialization */
void Init_Clock(void)
{
    /*
     * The MSP430 has a number of different on-chip clocks. You can read about it in
     * the section of the Family User Guide regarding the Clock System ('cs.h' in the
     * driverlib).
     */

    /*
     * On the LaunchPad, there is a 32.768 kHz crystal oscillator used as a
     * Real Time Clock (RTC). It is a quartz crystal connected to a circuit that
     * resonates it. Since the frequency is a power of two, you can use the signal
     * to drive a counter, and you know that the bits represent binary fractions
     * of one second. You can then have the RTC module throw an interrupt based
     * on a 'real time'. E.g., you could have your system sleep until every
     * 100 ms when it wakes up and checks the status of a sensor. Or, you could
     * sample the ADC once per second.
     */
    //Set P4.1 and P4.2 as Primary Module Function Input, XT_LF
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN1 + GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);

    // Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768);
    // Set ACLK = XT1
    CS_initClockSignal(CS_ACLK, CS_XT1CLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Initializes the XT1 crystal oscillator
    CS_turnOnXT1LF(CS_XT1_DRIVE_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
}

/* UART Initialization */
void Init_UART(void)
{
    /* UART: It configures P1.0 and P1.1 to be connected internally to the
     * eSCSI module, which is a serial communications module, and places it
     * in UART mode. This let's you communicate with the PC via a software
     * COM port over the USB cable. You can use a console program, like PuTTY,
     * to type to your LaunchPad. The code in this sample just echos back
     * whatever character was received.
     */

    //Configure UART pins, which maps them to a COM port over the USB cable
    //Set P1.0 and P1.1 as Secondary Module Function Input.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);

    /*
     * UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 9600 baud rate. These
     * values were calculated using the online calculator that TI provides at:
     * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
     */

    //SMCLK = 1MHz, Baudrate = 9600
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 17, UCOS16 = 1
    EUSCI_A_UART_initParam param = {0};
        param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
        param.clockPrescalar    = 6;
        param.firstModReg       = 8;
        param.secondModReg      = 17;
        param.parity            = EUSCI_A_UART_NO_PARITY;
        param.msborLsbFirst     = EUSCI_A_UART_LSB_FIRST;
        param.numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT;
        param.uartMode          = EUSCI_A_UART_MODE;
        param.overSampling      = 1;

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

    // Enable EUSCI_A0 RX interrupt
    EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
}

/* PWM Initialization */
void Init_PWM(void)
{
    /*
     * The internal timers (TIMER_A) can auto-generate a PWM signal without needing to
     * flip an output bit every cycle in software. The catch is that it limits which
     * pins you can use to output the signal, whereas manually flipping an output bit
     * means it can be on any GPIO. This function populates a data structure that tells
     * the API to use the timer as a hardware-generated PWM source.
     *
     */
    //Generate PWM - Timer runs in Up-Down mode
    param.clockSource           = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider    = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerPeriod           = TIMER_A_PERIOD; //Defined in main.h
    param.compareRegister       = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    param.compareOutputMode     = TIMER_A_OUTPUTMODE_RESET_SET;
    param.dutyCycle             = HIGH_COUNT; //Defined in main.h

    //PWM_PORT PWM_PIN (defined in main.h) as PWM output
    GPIO_setAsPeripheralModuleFunctionOutputPin(PWM_PORT, PWM_PIN, GPIO_PRIMARY_MODULE_FUNCTION);
}

void Init_ADC(void)
{
    /*
     * To use the ADC, you need to tell a physical pin to be an analog input instead
     * of a GPIO, then you need to tell the ADC to use that analog input. Defined
     * these in main.h for A9 on P8.1.
     */

    //Set ADC_IN to input direction
    GPIO_setAsPeripheralModuleFunctionInputPin(ADC_IN_PORT, ADC_IN_PIN, GPIO_PRIMARY_MODULE_FUNCTION);

    //Initialize the ADC Module
    /*
     * Base Address for the ADC Module
     * Use internal ADC bit as sample/hold signal to start conversion
     * USE MODOSC 5MHZ Digital Oscillator as clock source
     * Use default clock divider of 1
     */
    ADC_init(ADC_BASE,
             ADC_SAMPLEHOLDSOURCE_SC,
             ADC_CLOCKSOURCE_ADCOSC,
             ADC_CLOCKDIVIDER_1);

    ADC_enable(ADC_BASE);

    /*
     * Base Address for the ADC Module
     * Sample/hold for 16 clock cycles
     * Do not enable Multiple Sampling
     */
    ADC_setupSamplingTimer(ADC_BASE,
                           ADC_CYCLEHOLD_16_CYCLES,
                           ADC_MULTIPLESAMPLESDISABLE);

    //Configure Memory Buffer
    /*
     * Base Address for the ADC Module
     * Use input ADC_IN_CHANNEL
     * Use positive reference of AVcc
     * Use negative reference of AVss
     */
    ADC_configureMemory(ADC_BASE,
                        ADC_IN_CHANNEL,
                        ADC_VREFPOS_AVCC,
                        ADC_VREFNEG_AVSS);

    ADC_clearInterrupt(ADC_BASE,
                       ADC_COMPLETED_INTERRUPT);

    //Enable Memory Buffer interrupt
    ADC_enableInterrupt(ADC_BASE,
                        ADC_COMPLETED_INTERRUPT);
}

//ADC interrupt service routine
#pragma vector=ADC_VECTOR
__interrupt
void ADC_ISR(void)
{
    uint8_t ADCStatus = ADC_getInterruptStatus(ADC_BASE, ADC_COMPLETED_INTERRUPT_FLAG);

    ADC_clearInterrupt(ADC_BASE, ADCStatus);

    if (ADCStatus)
    {
        ADCState = 0; //Not busy anymore
        ADCResult = ADC_getResults(ADC_BASE);
    }
}

void setEnableBitOutput(int enableBit3, int enableBit2, int enableBit1)
{
    if(enableBit3 == 0 && enableBit2 == 0 && enableBit1 == 0)
    {
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 0 && enableBit2 == 0 && enableBit1 == 1)
    {
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 0 && enableBit2 == 1 && enableBit1 == 0)
    {
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 0 && enableBit2 == 1 && enableBit1 == 1)
    {
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 1 && enableBit2 == 0 && enableBit1 == 0)
    {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 1 && enableBit2 == 0 && enableBit1 == 1)
    {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 1 && enableBit2 == 1 && enableBit1 == 0)
    {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
    else if(enableBit3 == 1 && enableBit2 == 1 && enableBit1 == 1)
    {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
    }
}

void getCurrentTime() {
    sprintf(currentTime, "%d", RTC_COUNTER);
}

void transmitString(char * string) {
    int i;
    for (i = 0; i < strlen(string); i++) {
        __delay_cycles(2000);
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, string[i]);
    } 
}

void printNewLine(){
    __delay_cycles(2000);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '\n');
    __delay_cycles(2000);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '\r');
}

void transmitNumber(int number) {
    if (number > 9999) { return; }
    int remainder = number;
    char printNum[4] = {'0', '0', '0', '0'};
    while (remainder >= 1000) {
        remainder -= 1000;
        printNum[0]++;
    } // while
    __delay_cycles(2000);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, printNum[0]);
    while (remainder >= 100) {
        remainder -= 100;
        printNum[1]++;
    } // while
    __delay_cycles(2000);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, printNum[1]);
    while (remainder >= 10) {
        remainder -= 10;
        printNum[2]++;
    } // while
    __delay_cycles(2000);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, printNum[2]);
    while (remainder >= 1) {
        remainder -= 1;
        printNum[3]++;
    } // while
    __delay_cycles(2000);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, printNum[3]);
}

/* EUSCI A0 UART ISR - Echoes data back to PC host */
#pragma vector=USCI_A0_VECTOR
__interrupt
void EUSCIA0_ISR(void)
{
    uint8_t RxStatus = EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, RxStatus);

    if (RxStatus)
    {
        //EUSCI_A_UART_transmitData(EUSCI_A0_BASE, EUSCI_A_UART_receiveData(EUSCI_A0_BASE));
        char recieve = EUSCI_A_UART_receiveData(EUSCI_A0_BASE);
        if (recieve == '\'') {
            printNewLine();
            if(strcmp(command, "ARM1") == 0){
                transmitString("Zone 1 Armed");
                ZONE_1_ARMED_STATUS = ARMED;
            }
            else if(strcmp(command, "ARM2") == 0){
                transmitString("Zone 2 Armed");
                ZONE_2_ARMED_STATUS = ARMED;
            }
            else if(strcmp(command, "ARM3") == 0){
                transmitString("Zone 3 Armed");
                ZONE_3_ARMED_STATUS = ARMED;
            }
            else if(strcmp(command, "ARM4") == 0){
                transmitString("Zone 4 Armed");
                ZONE_4_ARMED_STATUS = ARMED;
            }
            else if(strcmp(command, "DISARM1") == 0){
                transmitString("Zone 1 Disarmed");
                ZONE_1_ARMED_STATUS = NOT_ARMED;
            }
            else if(strcmp(command, "DISARM2") == 0){
                transmitString("Zone 2 Disarmed");
                ZONE_2_ARMED_STATUS = NOT_ARMED;
            }
            else if(strcmp(command, "DISARM3") == 0){
                transmitString("Zone 3 Disarmed");
                ZONE_3_ARMED_STATUS = NOT_ARMED;
            }
            else if(strcmp(command, "DISARM4") == 0){
                transmitString("Zone 4 Disarmed");
                ZONE_4_ARMED_STATUS = NOT_ARMED;
            }
            else if(strcmp(command, "ARMALL") == 0){
                transmitString("All Zones Armed");
                ZONE_1_ARMED_STATUS = ARMED;
                ZONE_2_ARMED_STATUS = ARMED;
                ZONE_3_ARMED_STATUS = ARMED;
                ZONE_4_ARMED_STATUS = ARMED;
            }
            else if(strcmp(command, "DISARMALL") == 0){
                transmitString("All Zones Disarmed");
                ZONE_1_ARMED_STATUS = NOT_ARMED;
                ZONE_2_ARMED_STATUS = NOT_ARMED;
                ZONE_3_ARMED_STATUS = NOT_ARMED;
                ZONE_4_ARMED_STATUS = NOT_ARMED;
            }
            //else if(strncmp(command, "ARMTIMEALL", strlen("ARMTIMEALL"))){
            else if(strncmp(command, "ATIMEALL", strlen("ATIMEALL")) == 0){
                armTime[0] = atoi(command + strlen("ATIMEALL"));
                armTime[1] = atoi(command + strlen("ATIMEALL"));
                armTime[2] = atoi(command + strlen("ATIMEALL"));
                armTime[3] = atoi(command + strlen("ATIMEALL"));
                transmitString("Zone1 Arm time: ");
                transmitNumber(armTime[0]);
                printNewLine();
                transmitString("Zone2 Arm time: ");
                transmitNumber(armTime[0]);
                printNewLine();
                transmitString("Zone3 Arm time: ");
                transmitNumber(armTime[1]);
                printNewLine();
                transmitString("Zone4 Arm time: ");
                transmitNumber(armTime[1]);
            }
            else if(strncmp(command, "ATIME1", strlen("ATIME1")) == 0) {
                armTime[0] = atoi(command + strlen("ATIME1"));
                transmitString("Zone1 Arm time: ");
                transmitNumber(armTime[0]);
            }
            else if(strncmp(command, "ATIME2", strlen("ATIME2")) == 0) {
                armTime[1] = atoi(command + strlen("ATIME2"));
                transmitString("Zone2 Arm time: ");
                transmitNumber(armTime[1]);
            }    
            else if(strncmp(command, "ATIME3", strlen("ATIME3")) == 0) {
                armTime[2] = atoi(command + strlen("ATIME3"));
                transmitString("Zone3 Arm time: ");
                transmitNumber(armTime[2]);
            }    
            else if(strncmp(command, "ATIME4", strlen("ATIME4")) == 0) {
                armTime[3] = atoi(command + strlen("ATIME4"));
                transmitString("Zone4 Arm time: ");
                transmitNumber(armTime[3]);
            }
            else if(strncmp(command, "DTIME1", strlen("DTIME1")) == 0) {
                disarmTime[0] = atoi(command + strlen("DTIME1"));
                transmitString("Zone1 Disarm time: ");
                transmitNumber(disarmTime[0]);
                MIC_FLAG = 0;
            }
            else if(strncmp(command, "DTIME2", strlen("DTIME2")) == 0) {
                disarmTime[1] = atoi(command + strlen("DTIME2"));
                transmitString("Zone2 Disarm time: ");
                transmitNumber(disarmTime[1]);
                MIC_FLAG = 0;
            }    
            else if(strncmp(command, "DTIME3", strlen("DTIME3")) == 0) {
                disarmTime[2] = atoi(command + strlen("DTIME3"));
                transmitString("Zone3 Disarm time: ");
                transmitNumber(disarmTime[2]);
                MIC_FLAG = 0;
            }    
            else if(strncmp(command, "DTIME4", strlen("DTIME4")) == 0) {
                disarmTime[3] = atoi(command + strlen("DTIME4"));
                transmitString("Zone4 Disarm time: ");
                transmitNumber(disarmTime[3]);
                MIC_FLAG = 0;
            }              
            else if(strncmp(command, "DTIMEALL", strlen("DTIMEALL")) == 0){
                disarmTime[0] = atoi(command + strlen("DTIMEALL"));
                disarmTime[1] = atoi(command + strlen("DTIMEALL"));
                disarmTime[2] = atoi(command + strlen("DTIMEALL"));
                disarmTime[3] = atoi(command + strlen("DTIMEALL"));
                transmitString("Zone1 Disarm time: ");
                transmitNumber(disarmTime[0]);
                printNewLine();
                transmitString("Zone2 Disarm time: ");
                transmitNumber(disarmTime[1]);
                printNewLine();
                transmitString("Zone3 Disarm time: ");
                transmitNumber(disarmTime[2]);
                printNewLine();
                transmitString("Zone4 Disarm time: ");
                transmitNumber(disarmTime[3]);
                MIC_FLAG = 0;
            }
            else if(strcmp(command, "STATUS") == 0){
            //     transmitString("Current Time: ");
            //    __delay_cycles(2000);
            //    getCurrentTime();
            //    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, currentTime[3]);
            //    __delay_cycles(2000);
            //    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, currentTime[2]);
            //    __delay_cycles(2000);
            //    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, currentTime[1]);
            //    __delay_cycles(2000);
            //    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, currentTime[0]);
            //    printNewLine();

               if(ZONE_1_STATUS == 0) { transmitString("Zone 1 OK"); }
               else if (ZONE_1_STATUS == 1) { transmitString("Zone 1 NOT_OK"); }
               printNewLine();

               if(ZONE_2_STATUS == 0) { transmitString("Zone 2 OK"); }
               else if (ZONE_2_STATUS == 1) { transmitString("Zone 2 NOT_OK"); }
               printNewLine();

               if(ZONE_3_STATUS == 0) { transmitString("Zone 3 OK"); }
               else if (ZONE_3_STATUS == 1) { transmitString("Zone 3 NOT_OK"); }
               printNewLine();

               if(ZONE_4_STATUS == 0) { transmitString("Zone 4 OK"); }
               else if (ZONE_4_STATUS == 1) { transmitString("Zone 4 NOT_OK"); }
               printNewLine();
            }
            else if(strcmp(command, "R") == 0)
            {
                transmitString("RESET THE SYSTEM . . .");
                ZONE_1_ARMED_STATUS = ARMED;
                ZONE_2_ARMED_STATUS = ARMED;
                ZONE_3_ARMED_STATUS = ARMED;
                ZONE_4_ARMED_STATUS = ARMED;

                ZONE_A_SOUND_STATUS = 0;
                ZONE_1_STATUS = 0;
                ZONE_2_STATUS = 0;
                ZONE_3_STATUS = 0;
                ZONE_4_STATUS = 0;

                ZONE_1_STATUS_DISPLAY = ZONE_OK;
                ZONE_2_STATUS_DISPLAY = ZONE_OK;
                ZONE_3_STATUS_DISPLAY = ZONE_OK;
                ZONE_4_STATUS_DISPLAY = ZONE_OK;

                ALARM_FLAG = 1;

                ZONE_1_LOCK_STATUS = 0;
                ZONE_2_LOCK_STATUS = 0;
                ZONE_3_LOCK_STATUS = 0;
                ZONE_4_LOCK_STATUS = 0;

                DISPLAY_LCD_FLAG = 0;
                MIC_FLAG = 0;
                noiseLevelCounter = 0;
                RTC_COUNTER = 0;
                MEASURE_FLAG = 0;
                Timer_A_stop(TIMER_A0_BASE); //Shut off PWM signal
                clearLCD();
            }
            else if(strcmp(command, "BUZZEROFF") == 0)
            {
                transmitString("BUZZER IS TURNED OFF");
                MIC_FLAG = 0;
                ALARM_FLAG = 1;
            }
            else if(strcmp(command, "ENABLEBUZZER") == 0) /*  1 = enable, 0 = disable */
            {
                transmitString("BUZZER IS ENABLED");
                ALARM_FLAG = 1;
            }
            else if(strcmp(command, "DISABLEBUZZER") == 0) /*  1 = enable, 0 = disable */
            {
                transmitString("BUZZER IS DISABLED");
                ALARM_FLAG = 0;

            }
            else if(strcmp(command, "F") == 0){
                transmitString("To be honest, you should probably just drop out of school . . .");
            }
            else if(strcmp(command, "L") == 0){
                transmitString("Your entire life is a BIG L");
            }
            else {
                transmitString("Invalid command. Please Try Again");
            } 
            printNewLine();
            __delay_cycles(2000);
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '>');
            __delay_cycles(2000);
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, ' ');
            commandPosition = 0;
            memset(command, 0, 50);
        }
        else{
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, recieve);
            command[commandPosition] = recieve;
            commandPosition++;
        }
    }
}
