# from gsmSerialio import *
import ConfigParser, os, sys, subprocess
from datetime import datetime as dt
from crontab import CronTab
import gsmio
import common
import processcontrol as pctrl
import dbio
import subprocess

mc = common.get_mc_server()
sconf = common.get_config_handle()

def gateway_initialize():
    common.main()

def change_server_number(row):
    cfg = common.read_cfg_file()

    args = row['msg'].split()

    try:
        number = args[2]
    except IndexError:
        reply = "Insufficient arguments."
        common.save_sms_to_memory(reply, row['contact_id'])
        return

    print "Changing server num"
    try:
        cfg.set('serverinfo','simnum',number)
        common.save_cfg_changes(cfg)
        reply = "NEW SERVERNUMBER " + number
    except TypeError:
        print ">> No number given"
        reply = "ERROR IN SMS" + row['msg']
    
    # sendMsgWRetry(reply,msg.simnum)
    common.save_sms_to_memory(reply, row['contact_id'])
    gateway_initialize()

def change_xbee_timeout(row):
    cfg = common.read_cfg_file()

    args = row['msg'].split()

    try:
        timeout_s = args[2]
    except IndexError:
        reply = "Insufficient arguments."
        common.save_sms_to_memory(reply, row['contact_id'])
        return

    print "Changing xbee sampling timeout"
    try:
        cfg.set('xbee','sampletimeout',timeout_s)
        common.save_cfg_changes(cfg)
        reply = "NEW XBEE TIMEOUT " + timeout_s
    except TypeError:
        print ">> No timeoout value given"
        reply = "ERROR IN SMS: " + row['msg']
    
    # sendMsgWRetry(reply,msg.simnum)
    common.save_sms_to_memory(reply, row['contact_id'])
    gateway_initialize()

def register_number(row):
    print '>> Registering number'

    args = row['msg'].split()
    try:
        name = args[2]
        sim_num = args[3]
    except IndexError:
        reply = "Insufficient arguments."
        common.save_sms_to_memory(reply, row['contact_id'])
        return

    query = "insert into phonebook (name,sim_num) values ('%s','%s')" % (name,sim_num)

    dbio.commit_to_db(query,'rn')

    reply = "Contact %s-%s registered" % (name, sim_num)
    common.save_sms_to_memory(reply, row['contact_id'])
    

def change_coord_name(row):
    print "Changing site name"

    cfg = common.read_cfg_file()

    msg = row['msg']

    try:
        cfg.set('coordinfo','name',msg.split(" ")[2].upper())
        common.save_cfg_changes(cfg)
        reply = "New coordname " + msg.split(" ")[2].upper()
    except TypeError:
        print ">> No name given"
        reply = "ERROR in SMS" + msg.data

    print reply
    common.save_sms_to_memory(reply, row["contact_id"])

def change_running_version(args, row):
    version = args[2]
    try:
        common.spawn_process("python ~/gateway/processcontrol.py -v %s" % version)
    except IndexError:
        reply = "No argument given (%s)" % (row['msg'])
        common.save_sms_to_memory(reply, row["contact_id"])
        return

    reply = "Running version changed to %s" % (version)
    common.save_sms_to_memory(reply, row["contact_id"])

def cycle_gsm(row):
    gsmio.power_gsm('OFF')
    gsmio.power_gsm()

    reply = 'Reset GSM success'
    common.save_sms_to_memory(reply, row["contact_id"])

def change_report_interval(row):
    msg_arguments = row['msg'].split(" ")

    try:
        job_name = msg_arguments[2]
    except IndexError:
        reply_message = "ERROR: job name"
        print "No report interval in message"
        common.save_sms_to_memory(reply_message,row['contact_id'])

    try:
        interval = msg_arguments[3]
    except IndexError:
        reply_message = "ERROR: missing argument:\n %s" % row["msg"]
        common.save_sms_to_memory(reply_message,row['contact_id'])
        return
    
    reply_message = pctrl.change_report_interval(job_name,interval)
    common.save_sms_to_memory(reply_message,row['contact_id'])

# def run_cmd_terminal(row):
    

def main():
    
    # allmsgs = senslopedbio.getAllSmsFromDb("UNREAD")
    smsinbox = mc.get("smsinbox")
    smsinbox = smsinbox[smsinbox["stat"]==0]

    print dt.now()

    exit_flag = False
    reboot_system = False

    if len(smsinbox) <= 0:
        print '>> No message to process'
        return
    # else:
    #     print smsinbox

    for index, row in smsinbox.iterrows():
        # print "MSG:", row["contact_id"], row["msg"], row["ts"]

        msg_args = row['msg'].split(" ")
        
        
        # check password and message structure
        if is_sms_valid(row['msg']):
            print "sms is valid"
        else:
            print "sms is not valid"
            print row['msg']
            if sconf["sms"]["delsmsafterread"]:
                smsinbox.loc[index, 'stat'] = 1
            continue
        
        cmd = msg_args[0].lower()
        if cmd == "coordname":
            change_coord_name(row)
        elif cmd == "version":
            change_running_version(msg_args, row)

    #             registerNumber(msg)
    #         elif msg.simnum not in getcfg.authnum.split(","):
    #             print ">> Error: number not registered"
    #             continue
        elif cmd == "servernum":
            change_server_number(row)
            common.spawn_process('python /home/pi/gateway/common.py')            
    #         elif cmd == "servernum?":
    #             print 'response'
    #             # sendMsgWRetry("SERVERNUM " + getcfg.servernum, msg.simnum)
    #             senslopeServer.WriteOutboxMessageToDb("SERVERNUM " + getcfg.servernum, msg.simnum)            
    #         elif cmd == "coordname":
    #             changeCoordName(msg)
    #         elif cmd == "sendingtime":
    #             changeReportInterval(msg)
        elif cmd == "resetgsm":
            cycle_gsm(row)
        elif cmd == "register":
            register_number(row)
            # exit_flag = True
    #             # sendMsgWRetry("RESET successful", msg.simnum)
    #             senslopeServer.WriteOutboxMessageToDb("RESET successful", msg.simnum)
        elif cmd == 'reboot':
            print ">> Setting reboot command for 2 mins"
            ts = dt.today().strftime("%c")
            reboot_message = "USER REBOOT initiated at %s" % (ts)
            common.save_sms_to_memory(reboot_message, row['contact_id'])
            reboot_system = True
        elif cmd == 'sensorpoll':
            ts = dt.today().strftime("%c")
            cmd_reply = "USER initiated sensorpoll at %s" % (ts)
            common.save_sms_to_memory(cmd_reply, row['contact_id'])
            subprocess.Popen(["python","/home/pi/gateway/xbeegate.py -s"])                
        elif cmd == 'interval':
            change_report_interval(row)
        elif cmd == 'xbeetimeout':
            change_xbee_timeout(row)
        else:
            # read_fail = True
            print ">> Command not recognized", cmd

        if sconf["sms"]["delsmsafterread"]:
            smsinbox.loc[index, 'stat'] = 1
            if exit_flag:
                sys.exit()

    mc.set("smsinbox",smsinbox)


        
    # # gsmClosePortAndHandle()
    # setReadStatus("READ-SUCCESS",read_success_list)
    # setReadStatus("READ-FAIL",read_fail_list)
    
    if reboot_system:
        # reboot after 2 mins
        os.system('sudo shutdown -r +2')
        sys.exit()

def is_sms_valid(sms):
    args = sms.split(" ")
    try:
        passwd = args[1].lower()
    except IndexError:
        print "No password relayed"
        return 0
    
    if passwd != sconf["user"]["pass"]:
        print ">> Wrong password"
        return 0

    if len(args) < 2:
        print ">> Sms structure not valid"
        return 0

    return 1
    
if __name__=='__main__':
     
    try:
        main()
    except KeyboardInterrupt:
        print "Aborting ..."
        # gsmClosePortAndHandle()