import RPi.GPIO as GPIO
import serial
import common
import re
import signal
from datetime import datetime as dt
import time

#timeout callables
def signal_handler(signum, frame):
    raise SampleTimeoutException("Timeout!")
class SampleTimeoutException(Exception):
    pass

#Turn on Custom Due thru trig pin
trig_pin = 37 #should be in server_config, what is this pin, 5V out?
GPIO.setmode(GPIO.BOARD)
GPIO.setup(trig_pin, GPIO.OUT)
GPIO.output(trig_pin,True)
print(">> Triggered!")
time.sleep(3)
#open serial port
ser = serial.Serial("/dev/dueusbport",9600) #should be in server_config

try:
	#initialize timeout countdown
	signal.signal(signal.SIGALRM, signal_handler)
	signal.alarm(300) #should be in server_config

	#send first command
	cmd = "ARQCMD6T\r\n".encode()
	print(">> Command: {}".format(cmd))
	ser.write(cmd)
	time.sleep(1)
	#filter and append timestamp to data
	#save to memcache for sending
	while True:
		if(ser.in_waiting > 0):
			rx = ser.read_until('\n')
			
			hashStart=rx.find('#')
			if hashStart != -1:
				print("Valid raw packet received:\n {}".format(rx))
				ser.write("OK")
			elif rx == "STOPLORA\r\n":
				time.sleep(.5)
				break
			else:
				print("Invalid packet:\n {}".format(rx))
			msg = rx[hashStart+1:-1]
			msg = re.sub('[^A-Za-z0-9\*\:\.\-\,\+\/]',"",msg)
			rx = re.sub('[^A-Za-z0-9\*<>\#\/\:\.\-\,\+]',"",rx)
			if re.search(">>\d{1,2}\/\d{1,2}#.*<<",rx):
				print ">> Sending whole message.\n"
				timestamp = dt.now().strftime("%y%m%d%H%M%S")
				tilt_soms_msg = msg + timestamp
				#print(tilt_soms_msg)
				common.save_sms_to_memory(tilt_soms_msg)
			elif re.search(">>\d{1,2}\/\d{1,2}#.*",rx):
				tilt_soms_msg = msg
			elif re.search(".*<<",rx):
				print ">> Sending completed message.\n"
				timestamp = dt.now().strftime("%y%m%d%H%M%S")
				tilt_soms_msg = tilt_soms_msg + msg + timestamp
				#print(tilt_soms_msg)
				common.save_sms_to_memory(tilt_soms_msg)
			elif re.search(".*",rx):
				tilt_soms_msg += msg
			else:
				print "::: unrecognized"

except SampleTimeoutException:
	print(">> Timeout!")
except KeyboardInterrupt:
	print(">> Stopped!")
except serial.serialutil.SerialException:
	print(">> No serial port. Check serial connection.")
finally:
	#exit routiine, also used for timeout and interrupt/exceptions
	ser.close()
	GPIO.output(trig_pin,False)
	GPIO.cleanup(trig_pin)
	print(">> Finished.")
