//=====[#include guards - begin]===============================================
#ifndef _PC_SERIAL_COM_H_
#define _PC_SERIAL_COM_H_
//=====[Declaration of public defines]=========================================

//=====[Declaration of public data types]======================================

// maquina de estados para la fecha
enum datesate{
    initState,
    yearState,
    monthState,
    dayState,
    hourState,
    minuteState,
    secondState
};

//=====[Declarations (prototypes) of public functions]=========================

void pcSerialComInit();
char pcSerialComCharRead();
void pcSerialComStringWrite( const char* str );
void pcSerialComUpdate();
bool pcSerialComCodeCompleteRead();
void pcSerialComCodeCompleteWrite( bool state );
//=====[#include guards - end]=================================================
#endif // _PC_SERIAL_COM_H_