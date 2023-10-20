//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "pc_serial_com.h"

#include "siren.h"
#include "fire_alarm.h"
#include "code.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "event_log.h"

//=====[Declaration of private defines]========================================

//=====[Declaration of private data types]=====================================

typedef enum{
    PC_SERIAL_COMMANDS,
    PC_SERIAL_GET_CODE,
    PC_SERIAL_SAVE_NEW_CODE,
} pcSerialComMode_t;

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromPcSerialCom[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static pcSerialComMode_t pcSerialComMode = PC_SERIAL_COMMANDS;
static bool codeComplete = false;
static bool dateComplete = true;
static int numberOfCodeChars = 0;

static char year[5] = "";
static char month[3] = "";
static char day[3] = "";
static char hour[3] = "";
static char minute[3] = "";
static char second[3] = "";
int indice = 0;

//=====[Declarations (prototypes) of private functions]========================

static int pcSerialComStringRead( char* str, int strLength );

static void pcSerialComGetCodeUpdate( char receivedChar );
static void pcSerialComSaveNewCodeUpdate( char receivedChar );

static void pcSerialComCommandUpdate( char receivedChar );

static void availableCommands();
static void commandShowCurrentAlarmState();
static void commandShowCurrentGasDetectorState();
static void commandShowCurrentOverTemperatureDetectorState();
static void commandEnterCodeSequence();
static void commandEnterNewCode();
static void commandShowCurrentTemperatureInCelsius();
static void commandShowCurrentTemperatureInFahrenheit();
static void commandSetDateAndTime();
static void commandShowDateAndTime();
static void commandShowStoredEvents();

//=====[Implementations of public functions]===================================

void pcSerialComInit()
{
    availableCommands();
}

char pcSerialComCharRead()
{
    char receivedChar = '\0';
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
    }
    return receivedChar;
}
//
void pcSerialComStringWrite( const char* str )
{
    uartUsb.write( str, strlen(str) );
}

void pcSerialComUpdate()
{
    if (!dateComplete) {
        commandSetDateAndTime();
    } 
    else {    
        char receivedChar = pcSerialComCharRead();
        if( receivedChar != '\0' ) {
            switch ( pcSerialComMode ) {
                case PC_SERIAL_COMMANDS:
                    pcSerialComCommandUpdate( receivedChar );
                break;

                case PC_SERIAL_GET_CODE:
                    pcSerialComGetCodeUpdate( receivedChar );
                break;

                case PC_SERIAL_SAVE_NEW_CODE:
                    pcSerialComSaveNewCodeUpdate( receivedChar );
                break;
                default:
                    pcSerialComMode = PC_SERIAL_COMMANDS;
                break;
            }
        }    
    }
}

bool pcSerialComCodeCompleteRead()
{
    return codeComplete;
}

void pcSerialComCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

/* 
Función: static void pcSerialComStringRead( char* str, int strLength )
Esta función es bloqueante ya que espera a que el usuario escriba antes de continuar la ejecución. Se utiliza en 
la función static void commandSetDateAndTime() para establecer el año, mes y día.

Para resolver este problema, se puede implementar una máquina de estados que gestione la entrada de la fecha, 
donde cada estado corresponde a un valor específico que se está solicitando. Se realizarán modificaciones en los 
archivos pc_serial_com.cpp y su archivo de encabezado (.h) para incluir un enum que defina los estados de la fecha 
y una variable booleana "datecomplete". Además, se modificará pcSerialComUpdate para determinar si se necesitan más 
datos de fecha. La función commandSetDateAndTime() se actualizará para incluir un switch que maneje la máquina de 
estados. pcSerialComStringRead ahora deberá devolver la cantidad de dígitos recibidos como un entero, y la opción 
's' en pcSerialComCommandUpdate se actualizará para reflejar estos cambios.

Adicionalmente, se identificó una función que utiliza un retraso en el siguiente archivo: smart_home_system.cpp
Función: void smartHomeSystemUpdate()
Aunque esta función incluye un retraso de 10 ms, no se considera bloqueante ya que el retraso es muy corto.

 */

/* static int pcSerialComStringRead( char* str, int strLength )
{
    static int strIndex = 0;


    while ( uartUsb.readable() && (strIndex < strLength) ) {
            uartUsb.read( &str[strIndex] , 1 );
            uartUsb.write( &str[strIndex] ,1 );
            strIndex++;
    }
    int auxindex  = strIndex;
    if (strIndex == strLength){
        str[strLength]='\0';
        strIndex = 0;
    }

    return auxindex;
} */

static int pcSerialComStringRead( char* str, int strLength )
{
    //static int strIndex = 0;


    while ( uartUsb.readable() && (indice < strLength) ) {
            uartUsb.read( &str[indice] , 1 );
            uartUsb.write( &str[indice] ,1 );
            indice++;
    }
/*     int auxindex  = strIndex;
    if (strIndex == strLength){
        str[strLength]='\0';
        strIndex = 0;
    } */

    return indice;
}

static void pcSerialComGetCodeUpdate( char receivedChar )
{
    codeSequenceFromPcSerialCom[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
   if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        codeComplete = true;
        numberOfCodeChars = 0;
    } 
}

static void pcSerialComSaveNewCodeUpdate( char receivedChar )
{
    static char newCodeSequence[CODE_NUMBER_OF_KEYS];

    newCodeSequence[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
    if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        numberOfCodeChars = 0;
        codeWrite( newCodeSequence );
        pcSerialComStringWrite( "\r\nNew code configured\r\n\r\n" );
    } 
}

static void pcSerialComCommandUpdate( char receivedChar )
{
    switch (receivedChar) {
        case '1': commandShowCurrentAlarmState(); break;
        case '2': commandShowCurrentGasDetectorState(); break;
        case '3': commandShowCurrentOverTemperatureDetectorState(); break;
        case '4': commandEnterCodeSequence(); break;
        case '5': commandEnterNewCode(); break;
        case 'c': case 'C': commandShowCurrentTemperatureInCelsius(); break;
        case 'f': case 'F': commandShowCurrentTemperatureInFahrenheit(); break;
        case 's': case 'S': dateComplete = false; commandSetDateAndTime(); break;
        case 't': case 'T': commandShowDateAndTime(); break;
        case 'e': case 'E': commandShowStoredEvents(); break;
        default: availableCommands(); break;
    } 
}

static void availableCommands()
{
    pcSerialComStringWrite( "Available commands:\r\n" );
    pcSerialComStringWrite( "Press '1' to get the alarm state\r\n" );
    pcSerialComStringWrite( "Press '2' to get the gas detector state\r\n" );
    pcSerialComStringWrite( "Press '3' to get the over temperature detector state\r\n" );
    pcSerialComStringWrite( "Press '4' to enter the code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press '5' to enter a new code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n" );
    pcSerialComStringWrite( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n" );
    pcSerialComStringWrite( "Press 's' or 'S' to set the date and time\r\n" );
    pcSerialComStringWrite( "Press 't' or 'T' to get the date and time\r\n" );
    pcSerialComStringWrite( "Press 'e' or 'E' to get the stored events\r\n" );
    pcSerialComStringWrite( "\r\n" );
}

static void commandShowCurrentAlarmState()
{
    if ( sirenStateRead() ) {
        pcSerialComStringWrite( "The alarm is activated\r\n");
    } else {
        pcSerialComStringWrite( "The alarm is not activated\r\n");
    }
}

static void commandShowCurrentGasDetectorState()
{
    if ( gasDetectorStateRead() ) {
        pcSerialComStringWrite( "Gas is being detected\r\n");
    } else {
        pcSerialComStringWrite( "Gas is not being detected\r\n");
    }    
}

static void commandShowCurrentOverTemperatureDetectorState()
{
    if ( overTemperatureDetectorStateRead() ) {
        pcSerialComStringWrite( "Temperature is above the maximum level\r\n");
    } else {
        pcSerialComStringWrite( "Temperature is below the maximum level\r\n");
    }
}

static void commandEnterCodeSequence()
{
    if( sirenStateRead() ) {
        pcSerialComStringWrite( "Please enter the four digits numeric code " );
        pcSerialComStringWrite( "to deactivate the alarm: " );
        pcSerialComMode = PC_SERIAL_GET_CODE;
        codeComplete = false;
        numberOfCodeChars = 0;
    } else {
        pcSerialComStringWrite( "Alarm is not activated.\r\n" );
    }
}

static void commandEnterNewCode()
{
    pcSerialComStringWrite( "Please enter the new four digits numeric code " );
    pcSerialComStringWrite( "to deactivate the alarm: " );
    numberOfCodeChars = 0;
    pcSerialComMode = PC_SERIAL_SAVE_NEW_CODE;

}

static void commandShowCurrentTemperatureInCelsius()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadCelsius() );
    pcSerialComStringWrite( str );  
}

static void commandShowCurrentTemperatureInFahrenheit()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadFahrenheit() );
    pcSerialComStringWrite( str );  
}

static void commandSetDateAndTime()
{
// maquina de estados para
    static int date_state = initState;
    switch(date_state){
        case initState:
            pcSerialComStringWrite("\r\nType four digits for the current year (YYYY): ");
            date_state = yearState;
        break;
        case yearState:
            if (pcSerialComStringRead( year, 4) == 4){
                indice = 0;
                date_state = monthState;
                pcSerialComStringWrite("\r\n");
            pcSerialComStringWrite("Type two digits for the current month (01-12): ");
            }
        break;
        
        case monthState:
            if (pcSerialComStringRead( month, 2) == 2){
                indice = 0;
                date_state = dayState;
                pcSerialComStringWrite("\r\n");
                pcSerialComStringWrite("Type two digits for the current day (01-31): ");
            }
        break;
        case dayState:
            if ( pcSerialComStringRead( day, 2) == 2){
                indice = 0;
                date_state = hourState;
                pcSerialComStringWrite("\r\n");
                pcSerialComStringWrite("Type two digits for the current hour (00-23): ");
            }
        break;
        case hourState:
            if ( pcSerialComStringRead( hour, 2) == 2){
                indice = 0;
                date_state = minuteState;
                pcSerialComStringWrite("\r\n");
                pcSerialComStringWrite("Type two digits for the current minutes (00-59): ");
            }
        break;
        case minuteState:
            if ( pcSerialComStringRead( minute, 2) == 2){
                indice = 0;
                date_state = secondState;
                pcSerialComStringWrite("\r\n");
                pcSerialComStringWrite("Type two digits for the current seconds (00-59): ");
            }
        break;
        case secondState:
            if ( pcSerialComStringRead( second, 2) == 2){
                indice = 0;
                date_state = initState;
                pcSerialComStringWrite("\r\n");
                pcSerialComStringWrite("Date and time has been set\r\n"); 
                pcSerialComStringWrite(second); 
                 

                dateAndTimeWrite( atoi(year), atoi(month), atoi(day), 
                    atoi(hour), atoi(minute), atoi(second) );
                    indice = 0;
                    dateComplete = true;
                }
        break;}

}

static void commandShowDateAndTime()
{
    char str[100] = "";
    sprintf ( str, "Date and Time = %s", dateAndTimeRead() );
    pcSerialComStringWrite( str );
    pcSerialComStringWrite("\r\n");
}

static void commandShowStoredEvents()
{
    char str[EVENT_STR_LENGTH] = "";
    int i;
    for (i = 0; i < eventLogNumberOfStoredEvents(); i++) {
        eventLogRead( i, str );
        pcSerialComStringWrite( str );   
        pcSerialComStringWrite( "\r\n" );                    
    }
}