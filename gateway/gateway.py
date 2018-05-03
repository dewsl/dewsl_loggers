import gsmio
import argparse
import dbio
import sys, os
import raindetect as rd
import pandas as pd
import common
import lockscript
import serial
import time
import re
from datetime import datetime as dt
import subprocess

sys.path.append(os.path.realpath('..'))

def get_arguments():
    parser = argparse.ArgumentParser(description = "Gateway debug [-options]")

    parser.add_argument("-ir", "--initialize", 
        help = "initialize gateway configurations", action = 'store_true')

    parser.add_argument("-gp", "--get_phonebook", 
        help = "loads entries from phonebook", action = 'store_true')
    parser.add_argument('-d', "--deploy", help = "run whole gateway routine", 
        action = 'store_true')
    parser.add_argument('-wcs', "--write_custom_sms", 
        help = "write custom sms to smsoutbox", action = 'store_true')
    parser.add_argument('-som', "--send_outbox_messages", 
        help = "send messages stored in smsoutbox", action = 'store_true')
    
    parser.add_argument('-igm', "--initialize_gsm_module", 
        help = "initialize gsm module", action = 'store_true')
    parser.add_argument('-dg', "--debug_gsm", 
        help = "enter gsm AT mode", action = 'store_true')
    parser.add_argument('-rg', "--reset_gsm", 
        help = "reset gsm module", action = 'store_true')

    parser.add_argument('-rd', "--rain_detect", 
        help = "check current rain value", action = 'store_true')
    parser.add_argument('-rs', "--reset_rain_value",
        help = "set current rain value to zero", action = 'store_true')

    parser.add_argument('-sm', "--send_smsoutbox_memory",
        help = "send outbox messages in memory (RAM)", action = 'store_true')

    parser.add_argument('-pm', "--print_memory", type = str,
        help = "print value in memory (RAM)")
    parser.add_argument('-um', "--purge_memory", type = str,
        help = "purge value in memory (RAM)")

    parser.add_argument('-st', "--set_system_time",
        help = "set system time using time in power mngt module",
        action = 'store_true')
    parser.add_argument('-cm', "--create_startup_message",
        help = "create startup message", action = 'store_true')


    try:
        args = parser.parse_args()
        return args        
    except IndexError:
        print '>> Error in parsing arguments'
        error = parser.format_help()
        print error
        sys.exit()

def send_smsoutbox_memory():
    # lockscript.get_lock('gsm')

    print "Sending from memory ..."
    mc = common.get_mc_server()
    sc = mc.get('server_config')
    smsoutbox = mc.get("smsoutbox")

    # print smsoutbox
    phonebook = mc.get("phonebook")

    resend_limit = sc['gsmio']['sendretry']

    smsoutbox_unsent = smsoutbox[smsoutbox["stat"] < resend_limit]
    print smsoutbox_unsent

    for index, row in smsoutbox_unsent.iterrows():
        sms_msg = row['msg']
        sim_num = phonebook[row["contact_id"]]
        stat = gsmio.send_msg(sms_msg, sim_num)
        
        if stat == 0: 
            smsoutbox.loc[index, 'stat'] = resend_limit
            '>> Message sent'
        else:
            print '>> Message sending failed'
            print '>> Writing to mysql for sending later'
            smsoutbox.loc[index, 'stat'] += 1

            if smsoutbox.loc[index, 'stat'] >= resend_limit:
                dbio.write_sms_to_inbox(sms_msg, sim_num)

    print smsoutbox

    mc.set("smsoutbox",smsoutbox)

def custom_sms_routine():
    sms = raw_input("Custom message: ")
    sim_num = raw_input("Number (default: server): ")

    print sms, 
    if len(sim_num) == 0:
        dbio.write_sms_to_outbox(sms)
    else:
        dbio.write_sms_to_outbox(sms,sim_num)

def send_unsent_msg_outbox():
    all_sms = dbio.get_db_outbox()

    try:
        for sms in all_sms:
            # print type(sms.sid)
            gsmio.send_msg(sms.msg,sms.sid)  
    except TypeError:
        print '>> Aborting'

def set_system_time():
    now = time.time()
    
    pmod = serial.Serial()    
    sc = common.get_config_handle()
    print 'Initializing power module', sc['pmm']['port']
    pmod.port = sc['pmm']['port']
    pmod.baudrate = sc['pmm']['baud']
    pmod.open()
    
    pmod.write("PM+R\r")

    a = pmod.readline()
    print a

    try:
        a = re.sub(r'\W+','',a)
        print "I got time"
        print a
        
        dtnow = dt.strptime(a,"%y%m%d%H%M%S")
        print "Time is"
        print dt.now().strftime("%c")
        
        subprocess.call(["date","-s",dtnow.strftime("%c")])
        
    except ValueError:
        print "Error in processing time"
        f = open('timelog.txt','a')
        f.write("no time\r\n")
        f.close()

def create_startup_message():
    sc = common.get_config_handle()
    ts = dt.today().strftime("%c")
    common.save_sms_to_memory("%s SUCCESSFUL SYSTEM STARTUP at %s" % 
        (sc['coordinfo']['name'],ts))

def main():
    args = get_arguments()

    if args is None:
        print 'No arguments'
        return
        
    if args.print_memory:
        common.print_memory(args.print_memory)

    if args.initialize:
        common.main()

    if args.initialize_gsm_module:
        lockscript.get_lock('gateway gsm')
        gsmio.init_gsm()
    if args.get_phonebook:
        dbio.get_phonebook_numbers()
    if args.write_custom_sms:
        custom_sms_routine()
    if args.send_outbox_messages:
        lockscript.get_lock('gateway gsm')
        send_unsent_msg_outbox()
    if args.debug_gsm:
        lockscript.get_lock('gateway gsm')
        gsmio.gsm_debug()
    if args.reset_gsm:
        lockscript.get_lock('gateway gsm')
        gsmio.reset_gsm()

    if args.reset_rain_value:
        rd.reset_rain_value()
    if args.rain_detect:
        rd.check_rain_value()

    if args.send_smsoutbox_memory:
        lockscript.get_lock('gateway gsm')
        send_smsoutbox_memory()
    if args.purge_memory:
        common.purge_memory(args.purge_memory)

    if args.set_system_time:
        set_system_time()
    if args.create_startup_message:
        create_startup_message()
     

if __name__ == '__main__':
##    main()
    retry_count = 0
    is_gsm_init= False
    while True:
        try:
            if is_gsm_init:
                gsmio.init_gsm()
                is_gsm_init = False
            main()
            break
        except KeyboardInterrupt:
            print '>> Exiting gracefully.'
            break
        except IndexError:
            print time.asctime()
            print "Unexpected error:", sys.exc_info()[0]
        except gsmio.CustomGSMResetException:
            retry_count += 1
            print 'Will try again...'
            is_gsm_init = True

        if retry_count >= 10:
            print '>> Critical failure in GSM comms'
            sys.exit()
