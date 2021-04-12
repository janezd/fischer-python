#include "umFish40VC.H"
#include "Python.h"


#define DEFFUNC(n,d) {#n, (binaryfunc)n, METH_VARARGS, d}
#define DEFFUNC_NA(n) {#n, (binaryfunc)n, METH_NOARGS}

#define PYERROR(tpe,msg) { PyErr_Format(tpe, msg); return NULL; }
#define PYERROR_1(tpe,msg) { PyErr_Format(tpe, msg); return -1; }

#define PYASSERT(cond,msg) if (cond) { PyErr_Format(PyExc_IndexError, msg); return NULL; }
#define PYASSERT_1(cond,msg) if (cond) { PyErr_Format(PyExc_IndexError, msg); return -1; }

#define RETURN_NONE { Py_INCREF(Py_None); return Py_None; }

extern PyTypeObject *PyInterface_Type;

#define PyInterface_AsInterface(self) ((TInterface *)self)

#define IHANDLE (((TInterface *)self)->iHandle)
#define PROXYHANDLE (((TProxyInterface *)self)->iHandle)


bool PyToInteger(PyObject *value, int &res, char *msg = NULL)
{
  if (!PyInt_Check(value)) {
    PyErr_Format(PyExc_AttributeError, msg ? msg : "integer value expected");
    return false;
  }

  res = PyInt_AsLong(value);
  return true;
}

#define GETINTEGER(name) int name; if (!PyToInteger(value, name)) return -1;



class TInterface {
public:
  PyObject_HEAD
  int iHandle;
};


void interface_dealloc(PyObject *self)
{ 
  rbCloseInterface(PyInterface_AsInterface(self)->iHandle);
  PyObject_Del(self); 
}


class TProxyInterface {
public:
  PyObject_HEAD
  int iHandle;
};

void proxyinterface_dealloc(PyObject *self)
{
  PyObject_Del(self);
}


PyObject *proxyinterface_new(PyTypeObject *type, const int &iHandle)
{
  TInterface *intf = PyObject_New(TInterface, PyInterface_Type);
  intf->iHandle = iHandle;
  return (PyObject *)intf;
}


/*************************/
/* Motor proxy */


inline int motorDir(const int &m)
{
  return m ? (m & 1 ? 1 : -1) : 0;
}


int decodeMotorArg(PyObject *value, int &speed, int &counter)
{
  if (PyTuple_Check(value)) {
    if (PyTuple_Size(value) != 2)
      PYERROR_1(PyExc_AttributeError, "an integer or a two-element tuple expected");

    PyObject *pyspeed = PyTuple_GET_ITEM(value, 0);
    if (PyBool_Check(pyspeed))
      speed = PyObject_IsTrue(pyspeed) ? 7 : 0;
    else
      if (!PyToInteger(pyspeed, speed, "speed should be an integer"))
        return -1;

    if (!PyToInteger(PyTuple_GET_ITEM(value, 1), counter, "counter should be an integer"))
      return -1;

    PYASSERT_1((counter < 1) || (counter > 32), "counter out of range (0-32)");
  }


  else {
    if (PyBool_Check(value)) {
      speed = PyObject_IsTrue(value) ? 7 : 0;
      counter = 0;
    }

    else {
      PYASSERT_1(!PyInt_Check(value), "integer speed expected");
      speed = PyInt_AsLong(value);
      counter = 0;
    }
  }


  PYASSERT_1(abs(speed) > 7, "speed out of range (0-7)");
  return 0;
}



int setMotor(const int handle, const int i, PyObject *value)
{
  PYASSERT_1((i<1) || (i>4), "motor index out of range (0-3)");

  int speed, counter;
  if (decodeMotorArg(value, speed, counter) == -1)
    return -1;

  if (!speed)
    rbSetMotor(handle, i, 0);
  else {
    if (counter)
      rbRobMotor(handle, i, speed > 0 ? 1 : 2, abs(speed), counter);
    else
      rbSetMotorEx(handle, i, speed > 0 ? 1 : 2, abs(speed));
  }

  return 0;
}


int motorProxy_setitem(PyObject *self, int i, PyObject *value)
{
  return setMotor(PROXYHANDLE, i+1, value);
}


PyObject *motorProxy_getitem(PyObject *self, int i)
{
  PYASSERT((i < 0) || (i > 3), "motor index out of range (0-3)");
  return PyInt_FromLong(motorDir(rbGetMotors(PROXYHANDLE) >> (2*i)));
}


int motorProxy_len(PyObject *)
{
  return 4;
}


PyObject *motorProxy_str(PyObject *self)
{
  int motors = rbGetMotors(PROXYHANDLE);

  char *temps = new char[17], *ti = temps;
  *ti++ = '[';
  for(int ci = 4; ci; ci--) {
    const int m = motors & 3;
    switch (m) {
      case 0: *ti++ = '0'; break;
      case 1: *ti++ = '1'; break;
      case 2: *ti++ = '-'; *ti++ = '1'; break;
    };
    *ti++ = ',';
    *ti++ = ' ';
  }
  ti[-1] = ']';
  *ti = 0;

  PyObject *res = PyString_FromString(temps);
  delete temps;

  return res;
}



PySequenceMethods motorProxy_as_sequence = {
  motorProxy_len,                      /* sq_length */
  0, 0,
  motorProxy_getitem,               /* sq_item */
  0,              /* sq_slice */
  motorProxy_setitem,            /* sq_ass_item */
  0,           /* sq_ass_slice */
  0,
};


PyTypeObject PyMotorProxy_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "fischer.MotorProxy",
  sizeof(TProxyInterface), 0,
  proxyinterface_dealloc,
  0, 
  0, 0, 0, 0,
  0, &motorProxy_as_sequence, 0, /* protocols */
  0, 0,
  motorProxy_str,
  0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  0,
  0, 0, 0, 0, 0, 0, 0, 0,
  PyType_GenericAlloc, 
  0, 0, 0, 0, 0, 0, 0, 0,
};

PyTypeObject *PyMotorProxy_Type = &PyMotorProxy_Type_s;




/*************************/
/* Counter proxy */


int counterProxy_setitem(PyObject *self, int i, PyObject *value)
{
  PYASSERT_1((i < 0) || (i > 31), "counter index out of range (0-31)");
  PYASSERT_1(!PyInt_Check(value), "integer value expected");
  
  rbSetCounter(PROXYHANDLE, i+1, PyInt_AsLong(value));
  return 0;
}


PyObject *counterProxy_getitem(PyObject *self, int i)
{
  PYASSERT((i < 0) || (i > 31), "counter index out of range (0-31)");
  return PyInt_FromLong(rbGetCounter(PROXYHANDLE, i+1));
}

int counterProxy_len(PyObject *)
{
  return 32;
}


PyObject *counterProxy_str(PyObject *self)
{
  int iHandle = PROXYHANDLE;

  char *temps = new char[600], *ti = temps;
  *ti++ = '[';
  for(int ci = 1; ci <= 32; ci++) {
    sprintf(ti, "%i, ", rbGetCounter(iHandle, ci));
    ti += strlen(ti);
  }
  ti[-1] = ']';
  *ti = 0;

  PyObject *res = PyString_FromString(temps);
  delete temps;

  return res;
}



PySequenceMethods counterProxy_as_sequence = {
  counterProxy_len,                      /* sq_length */
  0, 0,
  counterProxy_getitem,               /* sq_item */
  0,              /* sq_slice */
  counterProxy_setitem,            /* sq_ass_item */
  0,           /* sq_ass_slice */
  0,
};


PyTypeObject PyCounterProxy_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "fischer.CounterProxy",
  sizeof(TProxyInterface), 0,
  proxyinterface_dealloc,
  0, 
  0, 0, 0, 0,
  0, &counterProxy_as_sequence, 0, /* protocols */
  0, 0,
  counterProxy_str,
  0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  0,
  0, 0, 0, 0, 0, 0, 0, 0,
  PyType_GenericAlloc, 
  0, 0, 0, 0, 0, 0, 0, 0,
};

PyTypeObject *PyCounterProxy_Type = &PyCounterProxy_Type_s;




/*************************/
/* Lamp proxy */



int setLamp(const int handle, const int i, PyObject *value)
{
  PYASSERT_1((i < 1) || (i > 8), "lamp index out of range (0-7)");

  if (PyBool_Check(value) || !PyInt_Check(value)) {
    rbSetLamp(handle, i, PyObject_IsTrue(value) ? 1 : 0);
    return 0;
  }

  else {
    int intensity = PyInt_AsLong(value);
    PYASSERT_1((intensity < 0) || (intensity > 7), "intensity out of range (0-7)");

    if (!intensity)
      rbSetLamp(handle, i, 0);
    else
      rbSetLampEx(handle, i, 1, intensity);
  }

  return 0;
}


int lampProxy_setitem(PyObject *self, int i, PyObject *value)
{
  return setLamp(PROXYHANDLE, i+1, value);
}



int lampProxy_len(PyObject *)
{
  return 8;
}


PySequenceMethods lampProxy_as_sequence = {
  lampProxy_len,                      /* sq_length */
  0, 0,
  0,               /* sq_item */
  0,              /* sq_slice */
  lampProxy_setitem,            /* sq_ass_item */
  0,           /* sq_ass_slice */
  0,
};


PyTypeObject PyLampProxy_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "fischer.LampProxy",
  sizeof(TProxyInterface), 0,
  proxyinterface_dealloc,
  0, 
  0, 0, 0, 0,
  0, &lampProxy_as_sequence, 0, /* protocols */
  0, 0,
  0,
  0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  0,
  0, 0, 0, 0, 0, 0, 0, 0,
  PyType_GenericAlloc, 
  0, 0, 0, 0, 0, 0, 0, 0,
};

PyTypeObject *PyLampProxy_Type = &PyLampProxy_Type_s;





#define STATATTR(stc,upr) attr[0]==stc &&  attr[1]>='1' && attr[1] <= upr && !attr[2]
#define ATTR2(stc) attr[0]==stc && !attr[1]

int findInList(const char *attr, char *lst[])
{
  for(char **li = lst; *li && strcmp(attr, *li); li++);
  return !*li ? -1 : li-lst;
}


char *anlg[] = {"AX", "AY", "AXS1", "AXS2", "AXS3", NULL};


PyObject *interface_getattr(PyObject *self, char *attr)
{
  const int iHandle = IHANDLE;

  // Motors
  if (STATATTR('M', '4')) {
    int motors = rbGetMotors(iHandle);
    for(int st = attr[1]-'1'; st; motors >>= 2, st--);
    motors &= 3;
    return PyInt_FromLong(motorDir(motors));
  }

  if (!strncmp(attr, "rob", 4) && attr[4] >= '1' && attr[4] <= '4' && !attr[5])
    return PyInt_FromLong(rbGetModeStatus(iHandle, attr[4]-'0'));

  if (!strncmp(attr, "counter", 6) && attr[6] >= '1' && attr[6] <= '4' && !attr[7])
    return PyInt_FromLong(rbGetCounter(iHandle, attr[6]-'0'));
  

  // Inputs
  if (STATATTR('I', '7'))
    return PyInt_FromLong(rbGetInput(iHandle, attr[1]-'0'));


  // Voltage
  if (STATATTR('A', '2'))
    return PyInt_FromLong(rbGetVoltage(iHandle, attr[1]-'0'));

  if (!strcmp(attr, "AV"))
    return PyInt_FromLong(rbGetVoltage(iHandle, 3));


  // Resistance
  int ai = findInList(attr, anlg)+1;
  if (ai)
    return PyInt_FromLong(rbGetAnalog(iHandle, ai));


  // Auxiliary functions
  if (!strcmp(attr, "deviceType"))
    return PyInt_FromLong(rbGetActDeviceType(iHandle));

  if (!strcmp(attr, "deviceSerialNr"))
    return PyInt_FromLong(rbGetActDeviceSerialNr(iHandle));

  if (!strcmp(attr, "deviceFirmwareNr"))
    return PyInt_FromLong(rbGetActDeviceFirmwareNr(iHandle));

  if (!strcmp(attr, "deviceFirmware"))
    return PyString_FromString(rbGetActDeviceFirmware(iHandle));

  if (!strcmp(attr, "deviceName"))
    return PyString_FromString(rbGetActDeviceName(iHandle));


  if (ATTR2('I')) {
    int stat = rbGetInputs(iHandle);
    PyObject *res = PyTuple_New(8);
    for(int i = 0; i < 8; i++, stat >>= 1)
      PyTuple_SetItem(res, i, PyInt_FromLong(stat & 1));

    return res;
  }

  if (ATTR2('M'))
    return proxyinterface_new(PyMotorProxy_Type, iHandle);

  if (!strcmp(attr, "counter"))
    return proxyinterface_new(PyCounterProxy_Type, iHandle);

  if (!strcmp(attr, "L"))
    return proxyinterface_new(PyLampProxy_Type, iHandle);

  if (ATTR2('A'))
    return Py_BuildValue("ii", rbGetVoltage(iHandle, 1), rbGetVoltage(iHandle, 2));


  PyObject *pyname = PyString_FromString(attr);
  PyObject *res = PyObject_GenericGetAttr(self, pyname);
  Py_DECREF(pyname);
  return res;

}



int interface_setattr(PyObject *self, char *attr, PyObject *value)
{
  const int iHandle = IHANDLE;

  if (STATATTR('M', '4'))
    return setMotor(iHandle, attr[1]-'0', value);


  if (!strncmp(attr, "counter", 6) && attr[6] >= '1' && attr[6] <= '4' && !attr[7]) {
    GETINTEGER(count);
    rbSetCounter(iHandle, attr[6]-'0', count);
    return 0;
  }

  if (STATATTR('L', '8'))
    return setLamp(iHandle, attr[1]-'0', value);


  if (ATTR2('M')) {
    if (!PySequence_Check(value) || PyObject_Size(value) != 4)
      PYERROR_1(PyExc_TypeError, "a sequence of length 4 expected");

    for(int m = 0; m <=3; m++) {
      PyObject *el = PySequence_GetItem(value, m);
      const int &r = setMotor(iHandle, m+1, el);
      Py_DECREF(el);
      if (r < 0)
        return -1;
    }
    return 0;
  }
      

  if (ATTR2('L')) {
    if (!PySequence_Check(value) || PyObject_Size(value) != 8)
      PYERROR_1(PyExc_TypeError, "a sequence of length 8 expected");

    for(int m = 0; m <=8; m++) {
      PyObject *el = PySequence_GetItem(value, m);
      const int &r = setLamp(iHandle, m+1, el);
      Py_DECREF(el);
      if (r < 0)
        return -1;
    }
    return 0;
  }
      

  if (!strcmp(attr, "counter")) {
    if (!PySequence_Check(value) || PyObject_Size(value) != 32)
      PYERROR_1(PyExc_TypeError, "a sequence of length 32 expected");

    int count;
    for(int m = 0; m <=32; m++) {
      PyObject *el = PySequence_GetItem(value, m);
      bool intOK = PyToInteger(el, count);
      Py_DECREF(el);
      if (intOK)
        return -1;
      rbSetCounter(iHandle, m+1, count);
    }
    return 0;
  }
   
  PyObject *pyname = PyString_FromString(attr);
  const int &res = PyObject_GenericSetAttr(self, pyname, value);
  Py_DECREF(pyname);
  return res;

}




PyObject *interface_new(PyTypeObject *type, PyObject *args, PyObject *keywords)
{
  int itype = ftROBO_first_USB;
  if (!PyArg_ParseTuple(args, "|i:openInterface", &itype))
    return NULL;

  int iHandle = rbOpenInterfaceUSB(itype, 0);
  if (iHandle == rbFehler) {
    PyErr_Format(PyExc_ImportError, "Cannot contact Fischer interface");
    return NULL;
  }

  TInterface *intf = PyObject_New(TInterface, PyInterface_Type);
  intf->iHandle = iHandle;
  return (PyObject *)intf;
}


PyMethodDef interface_methods[] = {
     {NULL, NULL}
};

PyTypeObject PyInterface_Type_s = {
  PyObject_HEAD_INIT((_typeobject *)&PyType_Type)
  0,
  "fischer.Interface",
  sizeof(TInterface), 0,
  interface_dealloc,
  0, 
  interface_getattr, interface_setattr, 0, 0,
  0, 0, 0, /* protocols */
  0, 0,
  0, 0, 0, 0,
  Py_TPFLAGS_DEFAULT, /* tp_flags */
  "",                                              /* tp_doc */
  0, 0, 0, 0, 0, 0, 
  interface_methods,
  0, 0, 0, 0, 0, 0, 0, 0,
  PyType_GenericAlloc, 
  interface_new, 0, 0, 0, 0, 0, 0, 0,
};

PyTypeObject *PyInterface_Type = &PyInterface_Type_s;




#ifdef _MSC_VER
  #define NOMINMAX
  #define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
  #include <windows.h>
#endif


extern "C" BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{ switch (ul_reason_for_call)
	{ case DLL_PROCESS_ATTACH:case DLL_THREAD_ATTACH:case DLL_THREAD_DETACH:case DLL_PROCESS_DETACH:break; }
  return TRUE;
}

PyMethodDef fischerFunctions[]={
     {NULL, NULL}
};

typedef struct { char *name; int code;} TiTypesC;
TiTypesC iTypesC[] = {
  {"ROBO_first_USB", 0},
  {"Intelligent_IF", 10}, 
  {"Intelligent_IF_Slave", 20}, 
  {"ROBO_IF_IIM", 50}, 
  {"ROBO_IF_USB", 60}, 
  {"ROBO_IF_COM", 70}, 
  {"ROBO_IF_Over_RF", 80}, 
  {"ROBO_IO_Extension", 90}, 
  {0, 0}};

extern "C" __declspec(dllexport) void initfischer()
{
  if (   (PyType_Ready(PyInterface_Type) < 0)
      || (PyType_Ready(PyMotorProxy_Type) < 0)
      || (PyType_Ready(PyLampProxy_Type) < 0)
      || (PyType_Ready(PyCounterProxy_Type) < 0))
    return;

  PyObject *fischerModule = Py_InitModule("fischer", fischerFunctions);

  PyModule_AddObject(fischerModule, "Interface", (PyObject *)PyInterface_Type);

  PyObject *itypes = PyModule_New("ITypes");
	for(TiTypesC *itc = iTypesC; itc->name; itc++)
    PyModule_AddObject(itypes, itc->name, PyInt_FromLong(itc->code));

  PyModule_AddObject(fischerModule, "ITypes", itypes);
}

