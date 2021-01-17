# AndamBus

Automation project based on Arduino Mega 2560 board as a base unit for middle-sized automation tasks - e.g. home automation of various kind:

    HVAC
    Solar systems
    garden watering
    measuring various physical quantities using analog read

Base units are used in a semi-autonomous mode to automate various tasks. This behavior is achieved by simply configuring unit pins to be used for a specific task:

    as a input/output, analog/digital pin
    as 1-wire thermometer
    as a virtual device dedicated for a specific task (like thermostat or any other control)

Main unit communicates with the base units using AndamBus protocol, which is a protocol developed for this purpose. Physical layer is built on RS485, while using a main unit as a bus arbiter sending command to a specific unit identified by address. The specific unit answers only to commands, where it is addressed. Communication speed is set to fixed 115200, which is tested and used in my environment.

Arduino units are using their EEPROM to store the configuration including any service information like unit address.
