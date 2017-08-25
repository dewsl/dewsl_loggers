import gsmio
import argparse
import dbio
import sys, os
import raindetect as rd
import pandas as pd
import common

sys.path.append(os.path.realpath('..'))

def get_arguments():
    parser = argparse.ArgumentParser(description = "Gateway debug [-options]")
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
    parser.add_argument('-rd', "--rain_detect", 
        help = "check current rain value", action = 'store_true')
    parser.add_argument('-rs', "--reset_rain_value",
        help = "set current rain value to zero", action = 'store_true')
    parser.add_argument('-sm', "--send_smsoutbox_memory",
        help = "send outbox messages in memory (RAM)", action = 'store_true')
    parser.add_argument('-ps', "--print_smsoutbox_memory",
        help = "print outbox messages in memory (RAM)", action = 'store_true')
    parser.add_argument('-ug', "--purge_smsoutbox_memory",
        help = "purge sent smsoutbox messages in memory (RAM)", action = 'store_true')

    try:
        args = parser.parse_args()
        return args        
    except IndexError:
        print '>> Error in parsing arguments'
        error = parser.format_help()
        print error
        sys.exit()

def send_smsoutbox_memory():
    print "Sending from memory ..."
    mc = common.get_mc_server()
    smsoutbox = mc.get("smsoutbox")

    # print smsoutbox
    phonebook = mc.get("phonebook")

    smsoutbox_unsent = smsoutbox[smsoutbox["stat"] == 0]

    for index, row in smsoutbox_unsent.iterrows():
        stat = gsmio.send_msg(row['msg'], phonebook[row["contact_id"]])
        
        if stat == 0:
            smsoutbox.loc[index, 'stat'] = 1
        else:
            print '>> Message sending fail'

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
    for sms in all_sms:
        # print type(sms.sid)
        gsmio.send_msg(sms.msg,sms.sid)  

def main():
    args = get_arguments()

    if args is None:
        print 'No arguments'
        return

    if args.initialize_gsm_module:
        gsmio.init_gsm()
    if args.get_phonebook:
        dbio.get_phonebook_numbers()
    if args.write_custom_sms:
        custom_sms_routine()
    if args.send_outbox_messages:
        send_unsent_msg_outbox()
    if args.debug_gsm:
        gsmio.gsm_debug()
    if args.reset_rain_value:
        rd.reset_rain_value()
    if args.rain_detect:
        rd.check_rain_value()
    if args.send_smsoutbox_memory:
        send_smsoutbox_memory()
    if args.print_smsoutbox_memory:
        common.print_smsoutbox_memory()
    if args.purge_smsoutbox_memory:
        common.purge_smsoutbox_memory()
     

if __name__ == '__main__':
##    main()
    while True:
        try:
            main()
            break
        except KeyboardInterrupt:
            print '>> Exiting gracefully.'
            break
        except IndexError:
            print time.asctime()
            print "Unexpected error:", sys.exc_info()[0]
        except gsmio.CustomGSMResetException:
            print 'Will try again...'