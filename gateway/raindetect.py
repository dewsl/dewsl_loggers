import RPi.GPIO as GPIO,sys
from datetime import datetime as dt, timedelta as td
import common
import pandas as pd
from dateutil.rrule import rrule, MINUTELY
from bisect import bisect

mc = common.get_mc_server()

def get_round_up_time(dt_start, interval = 15):
    # print dt_start
    times = list(rrule(MINUTELY,interval=interval, dtstart=dt_start.date(),count=96))
    # print times
    return times[bisect(times,dt_start)]

def get_rain_tips_per_interval(reset_value=False):
    rain_events = mc.get("rain_events")
    report_events = []
    
    while len(rain_events) > 0:
        event_min = rain_events.min()
        dt_round_up = get_round_up_time(event_min)
        report_events.append((dt_round_up,len(rain_events[rain_events<dt_round_up])))
        rain_events = rain_events[rain_events>dt_round_up]

    if reset_value:
        reset_rain_value()

    return report_events

def check_rain_value():
    rain_events = mc.get("rain_events")
    print rain_events
    print "Tips:", len(rain_events)

def reset_rain_value():
    mc.set("rain_events",pd.Series())

def record_rain(channel):
    rain_events = mc.get("rain_events")
    rain_events = rain_events.append(pd.Series([dt.now()]))
    mc.set("rain_events",rain_events)
    print "tips:", len(rain_events)
    
def main():
    rain_pin = 40
    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(rain_pin, GPIO.IN, pull_up_down = GPIO.PUD_UP)
    GPIO.add_event_detect(rain_pin, GPIO.FALLING, callback=record_rain, bouncetime=500)

    reset_rain_value()

    while True:
        pass

    GPIO.cleanup()

if __name__=='__main__':
    try:
        main()
    except KeyboardInterrupt:
        print "Aborting ..."
        # gsmClosePortAndHandle()

