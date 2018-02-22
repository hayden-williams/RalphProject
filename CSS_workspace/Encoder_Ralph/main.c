//*****************************************************************************
//
// MSP432 main.c template - Empty main
//
//****************************************************************************


#include "driverlib.h"
#include "stdio.h"

volatile int j = -1;  //counter for adc array
volatile int a;
//volatile int AVGER=1;  // ralph was experimenting with this
volatile int backup=1118;
volatile bool b;
volatile float i;
volatile int count=0;
volatile float it=0; //used to keep track of time delays in between motor cycles

volatile int irRead[3]; //ADC read for IR sensor
volatile int stepNumber = 0; //number of times stepper motor motion has occured

#define delay1 800
#define delay 900
void main(void){

    WDTCTL = WDTPW | WDTHOLD;           // Stop watchdog timer
    MAP_Interrupt_disableMaster();
     /*******Setting Clock Rate*************************************************************************/
     unsigned int dcoFrequency = 3840000;
      MAP_CS_setDCOFrequency(dcoFrequency); // DCO- digitally controlled oscillator
      // Clock configurations
      const Timer_A_UpModeConfig upConfig_0 =    // Configure counter in Up mode
      {   TIMER_A_CLOCKSOURCE_SMCLK,      // Tie Timer A to SMCLK
          TIMER_A_CLOCKSOURCE_DIVIDER_64,     // Increment counter every 64 clock cycles
          2000,  // Period of Timer A (this value placed in TAxCCR0)
          TIMER_A_TAIE_INTERRUPT_DISABLE,     // Disable Timer A rollover interrupt
          TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Enable Capture Compare interrupt
          TIMER_A_DO_CLEAR            // Clear counter upon initialization
      };

      MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);  // Tie SMCLK to DCO


   // Start Timer A

      /*Feed UART Configuration Correct Parameters*************************************************************************/

      /*Configure LEDS*************************************************************************/
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN0);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN1);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN4);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN5);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P6,GPIO_PIN6);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P2,GPIO_PIN7);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P3,GPIO_PIN0);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P6,GPIO_PIN4);
     MAP_GPIO_setAsOutputPin(GPIO_PORT_P6,GPIO_PIN5);
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN5);
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN4);
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN0);
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);//A1
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN5);//B1
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN6);//A2
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN7);//B2
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN0);
     MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN1);

     MAP_GPIO_setAsInputPinWithPullUpResistor (GPIO_PORT_P1, GPIO_PIN1);
     MAP_GPIO_setAsInputPinWithPullUpResistor (GPIO_PORT_P1, GPIO_PIN4);

     // Configuring GPIOs (5.5 A0)
     MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5, GPIO_TERTIARY_MODULE_FUNCTION);

     // Set Resolution to 10 bit
     MAP_ADC14_setResolution(ADC_10BIT);

     // Initializing ADC (MCLK/1/4)
      MAP_ADC14_enableModule();
      MAP_ADC14_initModule(ADC_CLOCKSOURCE_SMCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_4, 0);

     // Configuring ADC Memory
     MAP_ADC14_configureSingleSampleMode(ADC_MEM0, true);
     MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, false);

     /* Configuring Sample Timer */
     MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

     /* Enabling/Toggling Conversion */
     MAP_ADC14_enableConversion();
     MAP_ADC14_toggleConversionTrigger();

     // TODO: Enable watchdog timer - no need

//A1-B1-A2-B2-A1
while(1){

    if(j < 2) {
        j++;
    } else {
        irRead[0] = irRead[1]; // buffer for IR sensor...
        irRead[1] = irRead[2]; // ... for debouncing/variation
    }

    MAP_ADC14_toggleConversionTrigger();
    // Holds the clock while ADC14 collects data
    while(MAP_ADC14_isBusy() == 1){}
    // Converts read in value to voltage on the 3.3 scale, converts to IR reading scale
    irRead[j] = MAP_ADC14_getResult(ADC_MEM0) * (3.3/1023) * (100 * 3 / 5);
    MAP_ADC14_toggleConversionTrigger();
    //printf("%i %i %i\n", irRead[0],irRead[1],irRead[2]);

    if(stepNumber == 0) {
        if(irRead[0] > 30 && irRead[1] > 30 && irRead[2] > 30) { // if IR line is broken. i.e all buffer entries >30
            stepNumber = 1;
            MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);  // Tie SMCLK to DCO
            MAP_Timer_A_configureUpMode(TIMER_A0_BASE,&upConfig_0);  // Configure Timer A using above struct
            MAP_Interrupt_enableInterrupt(INT_TA0_0);   // Enable Timer A interrupt
            MAP_Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);  // Start Timer A
            MAP_Interrupt_enableMaster(); // enable interrupt, goes to interrupt code at bottom
        }
    }
    //Pushes board out of printer (b/c plate is at bottom)
    if (stepNumber == 1 && (irRead[0] > 30 || irRead[1] > 30 || irRead[2] > 30)) // Indefinitely push board out while plate at bootom
        {

            if(count==0) // first step in motor motion
            {
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN6);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN0);
            }

            if(count==1) // second step
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4|GPIO_PIN7);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN0);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN6);
            }
            if(count==2)  //third step
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN7);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN6);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN0);
            }
            if(count==3)  // fourth step
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN0);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN6);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN7);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);
            }

        }
    else if(stepNumber == 1){ // move to step two if plate has started to rise, ir sensor beam not broken
        stepNumber = 2;
        it = 0;
    }

    if(stepNumber == 2) {
        //Turns off stepper, removes resistance, let's board slide in
        if(it == 0) {
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN0);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN6);
             MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN7);
             MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);
        }

        //Push board from rack to 3D printer, turn on DC motor
        if(it == 600) {
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN5);
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN4);
        }

        //Stop DC motor
        if(it == 850) {
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN5);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN4);
        }
        //Reverse DC motor
        if(it == 870) {
            MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN5);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN4);
        }
        //Stop DC motor
        if(it == backup) {

            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN5);
            MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN4);
            printf("%i\n",backup);

            // ralph experimenting with this, was trying to get a half iteration( b/w 1117 and 1118)
//            if (AVGER==1)
//            {backup=1117;
//            AVGER=2;}
//            else if (AVGER==2)
//            {backup=1117;
//            AVGER=3;}
//            else if (AVGER==3)
//            {backup=1117;
//            AVGER=4;}
//            else if (AVGER==4)
//            {backup=1117;
//            AVGER=1;}


        }

        if(it < 1090 && it >= 845) { //  stepper motor pulling as dc motor is finishing pushing board in plate
            if(count==0) // similar stepper motor direction control
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN0);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN6);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN7);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);
            }
            if(count==1)
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN7);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN6);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3,GPIO_PIN0);
            }
            if(count==2)
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2,GPIO_PIN4|GPIO_PIN7);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN0);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P6,GPIO_PIN6);
            }
            if(count==3)
            {
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN7);
                MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6,GPIO_PIN6);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,GPIO_PIN4);
                MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3,GPIO_PIN0);
            }
        }

        if(it == 1120) { // at the end of sequence, disable interrupts
            printf("%i", irRead);
            //irRead[0] = 0;
            //Disable interrupts
            MAP_Timer_A_stopTimer(TIMER_A0_BASE);
            MAP_Timer_A_disableInterrupt(INT_TA0_0);
            MAP_Interrupt_disableMaster();

            stepNumber = 0;
            it = 0;
        }
    }

}
}
void TA0_0_IRQHandler(void){
MAP_Timer_A_clearCaptureCompareInterrupt (TIMER_A0_BASE,TIMER_A_CAPTURECOMPARE_REGISTER_0);

if (count<=2)
{
    count=count+1;
    }
else
{count=0;}

it=it+1; // increment iterator for out loop
}

// TODO: Add Reference command shaping function









