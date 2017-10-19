# from gsmSerialio import *
import ConfigParser, os, re
from datetime import datetime as dt, timedelta as td
import time, sys, memcache
import powermon as pmon
import gsmio
import common
import argparse
import raindetect as rd

def main():

    parser = argparse.ArgumentParser(description = "Health routine [-options]")
    parser.add_argument("-r", "--read_only", 
        help = "do not save data to memory", action = 'store_true')

    try:
        args = parser.parse_args()
    except IndexError:
        print '>> Error in parsing arguments'
        error = parser.format_help()
        print error
        sys.exit()


    ts = dt.today()
    ts_str = ts.strftime("%x,%X,")

    try:
        csq = str(gsmio.check_csq())
    except AttributeError:
        print ">> Error reading GSM CSQ. Setting CSQ to error value"
        csq = "98"

    mc = common.get_mc_server()
    cfg = mc.get("server_config")

    mmpertip = float(cfg['rain']['mmpertip'])

    rainval = "%0.2f" % (rd.check_rain_value(reset_rain = True) * 0.5) 
    sysvol = "%0.2f" % (pmon.read()["bus_voltage"])
    
    msgtosend = cfg["coordinfo"]["name"] + "W," + ts_str
    msgtosend += "0,000,000," + rainval + ","
    msgtosend += sysvol + ","
    msgtosend += csq

    if not args.read_only:
        print msgtosend
        common.save_sms_to_memory(msgtosend)
    
if __name__=='__main__':
    try:
        main()
    except KeyboardInterrupt:
        print "Aborting ..."
        # gsmClosePortAndHandle()
