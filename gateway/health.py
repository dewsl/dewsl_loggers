# from gsmSerialio import *
import ConfigParser, os, re
from datetime import datetime as dt, timedelta as td
import time, sys, memcache
import powermon as pmon
import gsmio
import common

def main():

    ts = dt.today()
    ts_str = ts.strftime("%x,%X,")

    csq = str(gsmio.check_csq())

    mc = common.get_mc_server()
    cfg = mc.get("server_config")

    rainval = "0"
    sysvol = "%0.2f" % (pmon.read()["bus_voltage"])
    
    msgtosend = cfg["coordinfo"]["name"] + "W," + ts_str
    msgtosend += "0,000,000," + rainval + ","
    msgtosend += sysvol + ","
    msgtosend += csq

    common.save_sms_to_memory(msgtosend)
    
if __name__=='__main__':
    try:
        main()
    except KeyboardInterrupt:
        print "Aborting ..."
        gsmClosePortAndHandle()
