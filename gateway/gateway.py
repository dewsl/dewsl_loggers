import gsmio
import argparse
import dbio

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
    
    try:
        args = parser.parse_args()
        return args        
    except IndexError:
        print '>> Error in parsing arguments'
        error = parser.format_help()
        print error
        sys.exit()

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