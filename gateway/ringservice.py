import common
import RPi.GPIO as GPIO
from datetime import datetime as dt
import time
import gsmio
import gateway as gw

def ring_isr(channel):
	print dt.now()
	time.sleep(1)

	# check if RI from call
	if GPIO.input(channel) == 0:
		print ">> Dropping call...",
		gsmio.gsmcmd("ATH")
		print 'done'
	else:
		common.save_smsinbox_to_memory()

    
def main():
    sconf = common.get_config_handle()
    ri_pin = sconf['gsmio']['ripin']
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(ri_pin, GPIO.IN, pull_up_down = GPIO.PUD_UP)
    GPIO.add_event_detect(ri_pin, GPIO.FALLING, callback=ring_isr, bouncetime=100)

    while True:
        time.sleep(100000)

if __name__=='__main__':
    try:
        main()
    except KeyboardInterrupt:
        print "Aborting ..."
