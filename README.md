Fischer Interface for Python
============================

> **Disclaimer**: This is a toy project I found on my disk. I apparently wrote it together with Matija Mestnik, when we both worked at the Laboratory of AI at Faculty of Computer Science, University of Ljubljana. I am storing this here for posterity.
> 
> I clearly remember this actually worked (though I may no longer be proud of the code itself). Judging by this documentation, it was compatible with Python 2.3 on (probably some old) Windows. 

Installation
------------

No fancy installer, just copy fischer.pyd to c:\python23\lib\site-packages (or wherever your Python's site-packages directory is). You will also need to copy umFish40.dll from Fischer's CD to somewhere where your Windows will find it, preferably to the Window's system32 directory. To check you've done everything right, open python and type `import fischer`. If it imports OK, you're done.

Class `Interface`
-----------------

To control your Fischer interface, create an an instance of class `Interface`. The class contains various methods and (virtual) variables through which you can switch your motors and lamps on and off.

```python
import fischer
intf = fischer.Interface()
```

If it complains, it didn't find your interface and you'll to give your interface's type as an argument to the constructor. You'll find the constants in `fischer.ITypes`; see what's there and find out what works for you. The default is `fischer.ITypes.ROBO_first_USB`.

One instance of the class corresponds to one Fischer interface. When the instance is created, the connection is established and the interface unit is busy. If you try to create another instance of `Interface`, the construction will fail (unless you have more than one unit attached to the computed).

The connection is closed when the object is deleted. This happens according to Python's garbage collection rules - objects are deleted when nobody knows about them any more, that is, when they go out of scope. Hence, if you create an instance inside of function and don't store it into a global variable or return it as a result, the object is destroyed and the connection is closed.

In the remaining examples will shall assume that you named the `Interface` instance `intf`, as we did above.

### Motors (M) and Counters

The board can control up to four motors labeled M1 through M4. In Python you will see them as `intf.M1`, `intf.M2`, `intf.M3` and `intf.M4`: writing to these variables sets the motor's speed and reading them retrieves the speed.

Speeds go from -7 to 7; negative and positive values make the motor turn in different directions and at a speed 0 it stands still. Hence, by writing 

```python
intf.M1 = 7
intf.M2 = -7
intf.M3 = 1
intf.M4 = 0
```

the first two motors will turn like mad in the opposite directions, the third motor will turn slowly and the fourth will stopped (especially if it *did* move before).

To set the motor at the maximal speed in the positive (whichever that is) direction, you can also set it to `True`. Setting the motor to `False` stops it.

You can set a motor to \"rob-mode\" by assigning it a tuple with the speed and a counter, 

```python
intf.M1 = 7, 12
```

If you read a value of one of these variables, you'll get -1, 1 or 0 - the direction in which the motor is turning. You cannot read the speed (Fischer's DLL doesn't have such a function).

The status of the motor can be read from `rob1`, `rob2`, `rob3` and `rob4` - you'll get `False` if the motor is in the normal operation mode and `True` if it's in rob mode. These variables are read-only.

Counters can be set or read from `counter1` \... `counter4`.

### Lamps (L)

Lamps share the same outputs on the board as motors. You can have two lamps in place of one motor, which gives eight lamps altogether. To switch them on and off use attributes `L1` to `L8`.

Lamps can have intensities from 0 to 7. Intensity 7 is the brightest and 0 means the lamp is switched off. You can also use `True` and `False`.

This will give you a boring light show (see the last section for something slightly less boring.

```python
import time

for t in range(20):
    intf.L1 = True
    time.sleep(0.5)
    intf.L1 = False
    intf.L2 = True
    time.sleep(0.5)
    intf.L2 = False
```

The status of a lamp cannot be read.

### Sensors (I), Voltages (A1, A2, V) and Resistances (AX, AY, AXS1, AXS2, AXS3)

Sensors (switches) are usually attached to inputs `I1` to `I8`. You can read their status from the attributes with the same names. Result will be a zero or one. Sensor variables are read-only, of course.

The board has three inputs you can read the voltage from: A1, A2 and AV. The `Interface` has three read-only attributes with the same names.

The board can also measure resistance on inputs AX, AY, AXS1, AXS2 and AXS3. The attributes in Python again have the same names and are read-only, of course.

### Miscellaneous attributes

Class interface also has read-only attributes `deviceType`, `deviceSerialNr`, `deviceFirmwareNr`, `deviceFirmware` and `deviceName`, which return what they say they do.

### Advanced: Reading/setting everything at once

You can set or get the state of all motors at once through the attribute `M`. To achieve the same effect as above, say

```python
intf.M = [7, -7, 1, 0]
```

You can also use `intf.M` like this

```python
motors = intf.M
motors[0] = [7]
print motors[2]
```

However, adding 

```python
motors = [7, -7, 1, 0]
```

will not set the motors, it will only assign a new list to a variable named `motors`.

Same trick can be done with the lamps (`L`), counters (`counter`), motors' modes (`rob`), sensors (`I`), voltages (`A`). The same limitations (read-only, write-only) apply as for the individual values.

If you decide to use this, be careful not to mix up the indices. In Python, the first element of the list has index 0, so `intf.M[0]`, which controls the first motor, corresponds to `M1` and `intf.M[3]` corresponds to `M4`.

This will switch the lights on and off sequentially. 

```python
import time

for t in range(20):
    for l in range(8):
        intf.L[l] = True
        time.sleep(0.5)
        intf.L[l] = False
        time.sleep(0.5)
```
