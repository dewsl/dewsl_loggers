# from gsmSerialio import *
import configparser, os, re
from datetime import datetime as dt, timedelta as td
import time, sys, memcache
import powermon as pmon
import gsmio
import common
import argparse
import raindetect as rd
import xbeegate as xb
import dbio

def main():
    parser = argparse.ArgumentParser(description = "Health routine [-options]")
    parser.add_argument("-r", "--read_only", 
        help = "do not save data to memory", action = 'store_true')

    try:
        args = parser.parse_args()
    except IndexError:
        print('>> Error in parsing arguments')
        error = parser.format_help()
        print(error)
        sys.exit()

    mc = common.get_mc_server()

    ts = dt.today()
    ts_str = ts.strftime("%x,%X,")

    try:
        if mc.get('rst_gsm_done') * mc.get('init_gsm_done') * mc.get('send_sms_done'):
            gsmio.check_csq()
            csq = int(mc.get("csq_val"))
            csq = str(csq)
        else:
            print(">> GSM is busy")
            csq = "98"
    except Exception as e:
        print(">> Error reading GSM CSQ. Setting CSQ to error value",e)
        csq = "98"

    cfg = mc.get("server_config")
    
    try:
        mmpertip = float(cfg['rain']['mmpertip'])
    except:
        os.system('python3 /home/pi/gateway3/gateway.py -ir')
        sys.exit()

    rainval = "%0.2f" % (rd.check_rain_value(reset_rain = True) * mmpertip) 
    sysvol = "%0.2f" % (pmon.read()["bus_voltage"])
    
    temp = os.popen("vcgencmd measure_temp").readline()
    tempval = temp.replace("temp=","").replace("'C\n","")
    print("Temperature: {0}".format(tempval))

    msgtosend = cfg["coordinfo"]["name"] + "W," + ts_str
    msgtosend += "0,000," + tempval + "," + rainval + ","
    msgtosend += sysvol + ","
    msgtosend += csq

    if not args.read_only:
        print(">> Saving to memory:")
        time.sleep(5)
        print(msgtosend)
        #common.save_sms_to_memory(msgtosend)
        dbio.write_sms_to_outbox(msgtosend)
        #dbio.write_sms_to_outbox(msgtosend,pb_id='639175972526')

    
if __name__=='__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("Aborting ...")
        # gsmClosePortAndHandle()
