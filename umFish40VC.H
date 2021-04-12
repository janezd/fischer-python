// -----------------------------------------------------------------------------
//
// umFish40VC.H                                           Ulrich Müller 11.05.05
//                                                        PCS1700       VC++ 6.0
// Deklarationen und Definitionen für umFish40.DLL
//
// -----------------------------------------------------------------------------

#include <Windows.h>

#define rbFehler 0xE0000001L

enum Interfaces {
	ftROBO_first_USB       = 0,
	ftIntelligent_IF       = 10,
	ftIntelligent_IF_Slave = 20,
	ftROBO_IF_IIM          = 50,
	ftROBO_IF_USB          = 60,
	ftROBO_IF_COM          = 70,
	ftROBO_IF_Over_RF      = 80,
	ftROBO_IO_Extension    = 90
};

int __stdcall rbOpenInterfaceUSB(int ifTyp, int SerialNr);
int __stdcall rbOpenInterfaceCOM(int ifTyp, int ComNr, int AnalogZyklen);
int __stdcall rbCloseInterface(int iHandle);
int __stdcall rbGetInput(int iHandle, int InputNr);
int __stdcall rbGetInputs(int iHandle);
int __stdcall rbGetAnalog(int iHandle, int AnalogNr);
int __stdcall rbGetIRKey(int Handle, int Code, int KeyNr);
int __stdcall rbGetVoltage(int iHandle, int VoltNr);
int __stdcall rbSetMotor(int iHandle, int MotNr, int Dir);
int __stdcall rbSetMotorEx(int iHandle, int MotNr, int Dir, int Speed);
int __stdcall rbGetMotors(int iHandle);
int __stdcall rbSetMotors(int iHandle, int MotorStatus);
int __stdcall rbSetMotorsEx(int iHandle, int MotorStatus, 
														int SpeedStatus, int SpeedStatus16);
int __stdcall rbGetModeStatus(int iHandle, int MotNr);
int __stdcall rbSetModeStatus(int iHandle, int MotNr, int Mode);
int __stdcall rbSetLamp(int iHandle, int LampNr, int OnOff);
int __stdcall rbSetLampEx(int iHandle, int LampNr, int OnOff, int Power);
int __stdcall rbRobMotor(int iHandle, int MotNr, int Dir, int Speed, int ICount);
int __stdcall rbRobMotors(int iHandle, int MotorStatus, 
													int SpeedStatus, int SpeedStatus16, int ModeStatus);
int __stdcall rbGetCounter(int iHandle, int CounterNr);
int __stdcall rbSetCounter(int iHandle, int CounterNr, int ICount);
int __stdcall rbClearCounters(int iHandle);
int    __stdcall rbGetActDeviceType(int iHandle);
int    __stdcall rbGetActDeviceSerialNr(int iHandle);
int    __stdcall rbGetActDeviceFirmwareNr(int iHandle);
LPCSTR __stdcall rbGetActDeviceFirmware(int iHandle);
LPCSTR __stdcall rbGetActDeviceName(int iHandle);
