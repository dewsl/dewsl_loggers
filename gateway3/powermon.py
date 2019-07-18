#!/usr/bin/env python
from ina219 import INA219
from ina219 import DeviceRangeError
from datetime import datetime as dt

def read():
	SHUNT_OHMS = 0.1

	ina = INA219(SHUNT_OHMS)
	ina.configure()
	power_info = {}
	

	try:
		power_info["ts"] = dt.now()
		power_info["bus_voltage"] = ina.voltage()
		power_info["bus_current"] = ina.current()
		power_info["sys_power"] = ina.power()

		print("Info for %s" % power_info["ts"].strftime("%Y-%m-%d %H:%M:%S"))
		print("Bus Voltage: %.3f V" % power_info["bus_voltage"])
		print("Bus Current: %.3f mA" % power_info["bus_current"])
		print("Power: %.3f mW" % power_info["sys_power"])

		return power_info

	except DeviceRangeError as e:
	# Current out of device range with specified shunt resister
		print(e)
		return None


if __name__ == "__main__":
    read()