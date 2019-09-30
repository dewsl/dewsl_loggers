import RPi.GPIO as GPIO
import serial
import re
import signal
import time
from datetime import datetime as dt
import common
import dbio

def signal_handler(signum, frame):
    raise SampleTimeoutException("Timeout!")
class SampleTimeoutException(Exception):
    pass

try:
    trig_pin = 37 #should be in server_config
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(trig_pin, GPIO.OUT)
    GPIO.output(trig_pin, False)
    time.sleep(1)
    GPIO.output(trig_pin, True)
    time.sleep(1)
    print("TRIGGERED")

    s = serial.Serial('/dev/dueusbport', 9600) #should be in server_config
    print("SERIAL CONNECTION ESTABLISHED")
    
    signal.signal(signal.SIGALRM, signal_handler)
    signal.alarm(300) #should be in server_config

    ts = dt.now().strftime('%y%m%d%H%M%S')
    strcmd = 'ARQCMD6T' + '/' + ts + '\r\n'
    cmd = strcmd.encode()
    print(("COMMAND: {}".format(cmd)))
    s.write(cmd)
    time.sleep(1)

    while True:
        if(s.in_waiting > 0):
            rx = s.read_until().decode()
            if rx.startswith('>>'):
                #print("RAW PACKET:\n{}".format(rx))
                s.write('OK'.encode())
                rx = re.sub('[^A-Za-z0-9\+\/\*\:\.\-\,]', '', rx)
                #common.save_sms_to_memory(rx)
                dbio.write_sms_to_outbox(rx)
                print(("VALID PACKET STORED FOR SENDING:\n{}\n".format(rx)))
            elif rx == 'STOPLORA\r\n':
                print(("FINISHED".format(rx)))	
                time.sleep(1)
                break
            elif rx == 'Time out...\r\n':
            	pass
            else:
                print(("INVALID PACKET:\n{}".format(rx)))

except SampleTimeoutException:
    print(">>TIMEOUT")
except KeyboardInterrupt:
    print(">>INTERRUPTED")
except serial.serialutil.SerialException:
    print(">>CHECK SERIAL CONNECTION")
finally:
    GPIO.output(trig_pin, False)
    GPIO.cleanup(trig_pin)
    s.close()
    print("0") #return 0
