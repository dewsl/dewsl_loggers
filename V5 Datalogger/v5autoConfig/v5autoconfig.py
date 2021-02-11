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

def set_datetime(ts):
	#S...C...timestamp
	print("\nStart process: Updating timestamp")
	waitread("S", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	if ts == 'SYSTEMTIME':
		timeStamp = dt.today().strftime("%Y,%m,%d,%H,%M,%S,%w")
	else:
		timeStamp = dt.strptime(ts,'%Y-%m-%d %H:%M:%S').strftime("%Y,%m,%d,%H,%M,%S,%w")
	serial_clear()
	swrite(timeStamp, 1)
	readout = ser.read(12)
	pcTs = timeStamp
	mcuTs = readout.decode('ascii')
	print ("PC timestamp: ", pcTs)
	print ("MCU timestamp: ", mcuTs)
	print ("End process: Timestamp succesfully updated!")
	waitread("C", True, "----------", 5, 1)

def set_servernum(network):
	#F...C...servernum
	if network == 'GLOBE':
		servernum = "639175972526"
	elif network == 'SMART':
		servernum = "639088125642"
	else:
		return
	print("\nStart process: Updating server number")
	waitread("F", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	print("Server Number:",servernum)
	serial_clear()
	swrite(servernum, 1)
	print ("End process: Server number succesfully updated!")
	waitread("C", True, "----------", 5, 1)

def set_loggerversion(logver):
	#J...C...loggerversion
	print("\nStart process: Updating logger version")
	waitread("J", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	print("Logger version:",logver)
	serial_clear()
	swrite(logver, 1)
	print ("End process: Logger version succesfully updated!")
	waitread("C", True, "----------", 5, 1)

def set_sendingtime(sendingtime):
	#D...C...sendingtime
	print("\nStart process: Updating sending time")
	waitread("D", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	print("Sending time:", sendingtime)
	swrite(sendingtime, 1)
	print ("End process: Sending time succesfully updated!")
	waitread("C", True, "----------", 5, 1)

def set_mcupassword(pw):
	#K...C...password
	print("\nStart process: Updating mcu password")
	waitread("K", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	print("MCU password:", pw)
	swrite(pw, 1)
	print ("End process: MCU password succesfully updated!")
	waitread("C", True, "----------", 5, 1)

def set_loggernames():
	#N...C...logname
	lognames = conf['loggernames'][1].split(' ')
	print("\nStart process: Updating datalogger names")
	waitread("N", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	sread(True)
	for logname in lognames:
		swrite(logname.upper(),1)
		sread(True)
	print ("End process: Names succesfully updated!")
	waitread("C", True, "----------", 5, 1)

def set_command(cmd):
	#Z...C...cmd
	print("\nStart process: Updating sensor command")
	waitread("Z", False, "Enter C to change:", 10, 1)
	swrite("C", 1)
	print("Command:", cmd)
	swrite(cmd, 1)
	print ("End process: Command succesfully updated!")
	waitread("C", True, "----------", 5, 1)

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
		readout += ser.read(ser.in_waiting).decode('ascii', 'replace')
	if printer:
		print(readout)
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

class CsvException(Exception):
	pass

def loadconfig(printer=True):
	csvfile = "datav5config.csv"
	cnf = dict()
	with open(csvfile) as f:
		reader = csv.reader(f)
		header_row = next(reader)
		#print (header_row)
		for row in reader:
			#print(row)
			if len(row) == 0 or row[0].lower().strip() == "" or row[0].lower().strip() == "configparameters":
				continue
			if check_csv_integrity(row) == False:
				raise CsvException("CSV data error found.")
			if row[0].lower().strip() in cnf:
				print("Check duplicate configparameters.")
				raise CsvException("CSV data error found.")
			cnf[row[0].lower().strip()] = [int(row[1].strip()), row[2].strip()]
		if printer:
			print("-"*35,"\n","Config Settings")
			for key in cnf.keys():
				if cnf[key][0]:
					print(f'{key}: {cnf[key][1]}')
			print("-"*35, "\n")
	return cnf

def csvfinalcheck():
	if check_logver_logname() == 0:
		raise CsvException("CSV data error found.")

def check_logver_logname():
	logver = conf['loggerversion'][1]
	lognames = conf['loggernames'][1].split(' ')
	if logver in ['1', '3']:
		logname_count = 2
	elif logver == '4':
		logname_count = 3
	elif logver == '5':
		logname_count = 4
	else:
		logname_count = 1
	if len(lognames) == logname_count:
		#print(lognames)
		return True
	else:
		print("Check logger name count.")
		return False

def check_csv_integrity(row):
	if len(row) == 3:
		if row[1].strip() in ['0', '1']:
			if row[0].lower().strip() in ['realtimeclock', 'servernetwork', 'loggerversion', 'sendingtime', 'loggernames', 'sensorcommand', 'mcupassword']:
				if row[0].lower().strip() == 'realtimeclock':
					if ( row[2].lower().strip() == 'systemtime' ):
						pass
					else:
						try:
							dt.strptime(row[2].lower().strip(),'%Y-%m-%d %H:%M:%S')
							pass
						except Exception as e:
							print(e, "Check realtimeclock value. Allowed values [systemtime, YYYY-MM-DD HH:MM:SS]")
							return False
				if row[0].lower().strip() == 'servernetwork':
					if ( row[2].upper().strip() in ['GLOBE', 'SMART'] ):
						pass
					else:
						print("Check servernetwork value. Allowed values [GLOBE, SMART]")
						return False
				if row[0].lower().strip() == 'loggerversion':
					try:
						int(row[2].strip())
						pass
					except Exception as e:
						print(e, "Check loggerversion value.")
						return False
				if row[0].lower().strip() == 'sendingtime' :
					try:
						int(row[2].strip())
						pass
					except Exception as e:
						print(e, "Check sendingtime value.")
						return False
				if row[0].lower().strip() == 'sensorcommand':
					if ( row[2].upper().strip() in ['ARQCMD6T', 'ARQCMD6S'] ):
						pass
					else:
						print("Check sensorcommand value.")
						return False
				if row[0].lower().strip() == 'mcupassword':
					if ( len(row[2].upper().strip()) > 0 ):
						pass
					else:
						print("Check mcupassword value.")
						return False
			else:
				print("Check configparameters column.")
				return False
		else:
			print("Check update column.")
			return False
	else:
		print("Check data entries. Check for missing or excess commas. Remove blank rows.")
		return False
	#print("Done")
	return True

def main():
	serial_clear()
	try:
		while not isfound("----------"):
			swrite("C",1)
		serial_clear()
		print('-'*10)
		if conf['realtimeclock'][0]:
			set_datetime(conf['realtimeclock'][1].upper())
		if conf['servernetwork'][0]:
			set_servernum(conf['servernetwork'][1].upper())
		if conf['loggerversion'][0]:
			set_loggerversion(conf['loggerversion'][1])
		if conf['sendingtime'][0]:
			set_sendingtime(conf['sendingtime'][1])
		if conf['sensorcommand'][0]:
			set_command(conf['sensorcommand'][1].upper())
		if conf['mcupassword'][0]:
			set_mcupassword(conf['mcupassword'][1])
		if conf['loggernames'][0]:
			set_loggernames()
		swrite("E")
		print('\n',"*"*10,"V5 AutoConfig Finished.","*"*10)
	except Exception as e:
		print("Error:", e)
	except KeyboardInterrupt:
		print("Terminating...")
	finally:
		ser.close()

print('\n',"*"*35,'\n',"*"*10,"V5 AUTOCONFIG","*"*10,'\n',"*"*35,'\n')
try:
	conf = loadconfig()
	csvfinalcheck()
	serport = comport_setup()
	serbaud = 115200
	ser = serial.Serial(serport, serbaud)
	print("Started")
	time.sleep(3)
	main()
except Exception as e:
	print(e)
