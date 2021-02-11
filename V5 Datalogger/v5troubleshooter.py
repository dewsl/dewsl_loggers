import serial.tools.list_ports as sertool
import serial
import time
from datetime import datetime as dt
import csv

def list_comports():
	comports = set()
	ports = sertool.comports()
	for port in ports:
		#print(port.device)
		comports.add(port.device)
	#print(comports)
	return comports

def comport_setup():
	get_comport1 = list_comports()
	print("Plug in or replug the v5 device.")
	counter = 0
	while True:
		time.sleep(0.5)
		get_comport2 = list_comports()
		comdiff = get_comport2 - get_comport1
		if len(comdiff) == 1:
			comport = list(comdiff)[0]
			break
		elif len(get_comport1 - get_comport2) == 1:
			get_comport1 = get_comport2
		if counter > 60:
			print("Error: Timeout reached; no device detected.")
			raise
		counter += 1
	print(comport)
	return comport

def serial_clear(input = True, output = True):
	if input:
		ser.reset_input_buffer()
		#print("input clear!")
	if output:
		ser.reset_output_buffer()
		#print("output clear!")

def sread(printer=False):
	readout = ""
	while ser.in_waiting:
		readout += ser.read(ser.in_waiting).decode('ascii')
	if printer:
		print(readout, end = '')
	return readout

def swrite(text, delay=0):
	text += '\r\n'
	ser.write(text.encode())
	if delay:
		time.sleep(delay)

def isfound(text):
	readout = sread()
	#if readout: print(readout)
	return True if text in readout else False

def waitread(command, rptcmd, expectedout, timeout, increment):
	timeout = max(0.2, timeout)
	timeout = min(timeout, 20)
	increment = max(0.1, increment)
	increment = min(increment, timeout)
	millicounter = 0
	serial_clear()
	swrite(command)
	while millicounter < timeout:
		time.sleep(increment)
		millicounter += increment
		if isfound(expectedout):
			serial_clear()
			return True
		if rptcmd:
			swrite(command)
	serial_clear()
	print("Error: Device is not responding.")
	return False

def proc_csq():
	print("\nStart process: Getting CSQ data")
	serial_clear()
	for i in range(10):
		swrite("O", 3)
		sread(True)
	print ("End process: Done!")
	waitread("C", True, "----------", 5, 1)

def main():
	proc_csq()

print('\n',"#"*35,'\n',"#"*8,"V5 TROUBLESHOOTER","#"*8,'\n',"#"*35,'\n')
try:
	serport = comport_setup()
	serbaud = 115200
	ser = serial.Serial(serport, serbaud)
	print("Started")
	time.sleep(1)
	main()
except Exception as e:
	print(e)
finally:
	ser.close()
