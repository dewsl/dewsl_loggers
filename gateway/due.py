import RPi.GPIO as GPIO
import serial
import common
import re
import signal
from datetime import datetime as dt

#timeout callables
def signal_handler(signum, frame):
    raise SampleTimeoutException("Timed out!")
class SampleTimeoutException(Exception):
    pass

#Turn on Custom Due thru trig pin
trig_pin = 12 #should be in server_config, what is this pin, 5V out?
GPIO.setmode(GPIO.BOARD)
GPIO.setup(trig_pin, GPIO.OUT)
GPIO.output(trig_pin,True)

#open serial port
ser = serial.Serial("dev/dueusbport",9600) #should be in server_config

#send first command
cmd = "ARCCMD6T".encode()

#initialize timeout countdown
signal.signal(signal.SIGALRM, signal_handler)
signal.alarm(360)

#filter and append timestamp to each data
#save to memcache for sending
while True:
    if(ser.in_waiting > 0):
        data = ser.readline()
        print(data)
	data = re.sub(r'[^\w*<>#/:.\-,+]',"",data)
	timestamp = dt.now().strftime("%y%m%d%H%M%S")
	dts = data + ',' + timestamp
	if dts.startswith('>>'):
		print ">> Valid sensor data!"
		dts = re.sub(r'[<>]',"",dts)
		common.save_sms_to_memory(dts)

#exit routiine, also used for timeout and interrupt/exceptions
ser.close()
GPIO.outpt(trig_pin,False)
GPIO.cleanup(trig_pin)