import serial, datetime, ConfigParser, time, re, sys, os
from datetime import datetime as dt
import dbio
import common
import RPi.GPIO as GPIO

# cfg = ConfigParser.ConfigParser()
# cfg.read('server_config.txt')

# # Baudrate = cfg.getint('Serial', 'Baudrate')
# # Timeout = cfg.getint('Serial', 'Timeout')
# Namedb = cfg.get('LocalDB', 'DBName')
# SaveToFile = cfg.get('I/O', 'savetofile')

class SmsItem:
    def __init__(self,sid,num,msg,ts):
       self.sid = sid
       self.simnum = num
       self.msg = msg
       self.ts = ts

class CustomGSMResetException(Exception):
    pass

def gsm_debug():
    gsm = init_gsm_serial()
    print '>> AT mode for GSM '
    print '>> Keyboard interrupt to exit'
    try:
        while True:
            cmd = raw_input("").strip()
            reply = gsmcmd(cmd,gsm)
            print reply.strip()

    except KeyboardInterrupt:
        print '>> Leaving AT mode ...'

def power_gsm(mode='ON'):
    print ">> Power GSM", mode
    # reset_pin = common.get_config_handle()['gsmio']['reset_pin']
    reset_pin =38
    stat_pin = 37

    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(reset_pin, GPIO.OUT)
    GPIO.setup(stat_pin, GPIO.IN)

    if mode == 'ON':
        exit_stat = 1
    else:
        exit_stat = 0

    now = time.time()

    while GPIO.input(stat_pin) != exit_stat and time.time()<now+10:
        GPIO.output(reset_pin, True)
        time.sleep(0.2)

    GPIO.output(reset_pin, False)

    if time.time()>10:
        return False
    else:
        return True

def reset_gsm():
    print ">> Resetting GSM Module ...",
    # reset_pin = cfg.getint('gsmio', 'resetpin')
    reset_pin = common.get_config_handle()['gsmio']['resetpin']
    # resetPin = 38
    try:
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(reset_pin, GPIO.OUT)

        GPIO.output(reset_pin, True)
        time.sleep(1)
        GPIO.output(reset_pin, False)
        # time.sleep(5)
        # GPIO.output(resetPin, True)
        print 'done'
        raise CustomGSMResetException((">> Raising exception to reset code "
            "from GSM module reset"))
    except ImportError:
        return

def init_gsm_serial():
    gsm = serial.Serial()

    s_conf = common.get_config_handle()

    port = s_conf['gsmio']['port']
    baudrate = s_conf['gsmio']['baudrate']
    timeout = s_conf['gsmio']['timeout']

    gsm.port = port
    gsm.baudrate = baudrate
    gsm.timeout = timeout
    if not gsm.isOpen():
        # print "opening gsm port"
        gsm.open()

    return gsm
       
def init_gsm():
    print 'Connecting to GSM modem'
    
    gsm = init_gsm_serial()

    power_gsm()

    print "Initializing ..."
    gsm.write('AT\r\n')
    time.sleep(0.5)
    gsm.write('AT\r\n')
    time.sleep(0.5)    
    print 'Switching to no-echo mode', gsmcmd('ATE0').strip('\r\n')
    print 'Switching to text mode', gsmcmd('AT+CMGF=1').rstrip('\r\n')
    mc = common.get_mc_server()
    mc.set('init_gsm',True)
    
def gsm_flush():
    """Removes any pending inputs from the GSM modem and checks if it is alive."""
    gsm = init_gsm_serial()
    try:
        gsm.flushInput()
        gsm.flushOutput()
        ghost = gsm.read(gsm.inWaiting())
        stat = gsmcmd('\x1a\rAT\r')    
        while('E' in stat):
            gsm.flushInput()
            gsm.flushOutput()
            ghost = gsm.read(gsm.inWaiting())
            stat = gsmcmd('\x1a\rAT\r')
    except serial.SerialException:
        print "NO SERIAL COMMUNICATION (gsmflush)"
        RunSenslopeServer(gsm_network)

def gsmcmd(cmd,gsm=None):
    """
    Sends a command 'cmd' to GSM Module
    Returns the reply of the module
    Usage: str = gsmcmd()
    """
    if gsm is None:
        gsm = init_gsm_serial()

    try:
        gsm.flushInput()
        gsm.flushOutput()
        a = ''
        now = time.time()
        gsm.write(cmd+'\r\n')
        while True:
            #print cmd
            #gsm.write(cmd+'\r\n')
            a += gsm.read(gsm.inWaiting())

            if a.find('OK')>=0:
                break
            elif a.find('ERROR')>=0:
                break
            elif time.time()>now+10:
                break
            # else:
            #     print 'None'            
            #a += gsm.read()
            #print '+' + a
            # print a
            time.sleep(0.5)
            # print '.',
        if time.time()>now+10:  
            a = '>> Error: GSM Unresponsive'
            reset_gsm()
        return a
    except serial.SerialException:
        print "NO SERIAL COMMUNICATION (gsmcmd)"
        reset_gsm()

def check_csq():
    csq_reply = gsmcmd('AT+CSQ')
    print csq_reply

    try:
        csq_val = int(re.search("(?<=: )\d{1,2}(?=,)",csq_reply).group(0))
        return csq_val
    except ValueError, AttributeError:
        return 0

def send_msg(msg, number):
    """
    Sends a command 'cmd' to GSM Module
    Returns the reply of the module
    Usage: str = gsmcmd()
    """
    # under development
    # return

    check_count = 0
    csq = 0
    while check_count < 10 and csq < 10:
        csq = check_csq()
        time.sleep(0.5)
        check_count += 1

    if check_count >= 10:
        print ">> No connection to network. Aborting ..."
        return -1

    # resolve sim_num from number
    mc = common.get_mc_server()
    pb_numbers = mc.get('pb_numbers')

    try:
        if type(number) == int or type(number) == long:
            number = pb_numbers[number]
        elif type(str):
            pass
    except KeyError:
        print ">> No record for phonebook id:", number 
        return -1

    gsm = init_gsm_serial()

    try: 
        a = ''
        now = time.time()
        preamble = "AT+CMGS=\""+number+"\""
        print "\nMSG:", msg
        print "NUM:", number
        gsm.write(preamble+"\r")
        now = time.time()
        while a.find('>')<0 and a.find("ERROR")<0 and time.time()<now+20:
            a += gsm.read(gsm.inWaiting())
            time.sleep(0.5)
            print '.',

        if time.time()>now+3 or a.find("ERROR") > -1:  
            print '>> Error: GSM Unresponsive at finding >'
            print a
            print '^^ a ^^'
            return -1
        else:
            print '>'
        
        a = ''
        now = time.time()
        gsm.write(msg+chr(26))
        while a.find('OK')<0 and a.find("ERROR")<0 and time.time()<now+60:
                a += gsm.read(gsm.inWaiting())
                time.sleep(0.5)
                print ':',
        if time.time()-60>now:
            print '>> Error: timeout reached'
            return -1
        elif a.find('ERROR')>-1:
            print '>> Error: GSM reported ERROR in SMS reading'
            return -1
        else:
            print ">> Message sent!"
            return 0
            
    except serial.SerialException:
        print "NO SERIAL COMMUNICATION (sendmsg)"
        RunSenslopeServer(gsm_network)	
        
def log_error(log):
    nowdate = dt.today().strftime("%A, %B %d, %Y, %X")
    f = open("errorLog.txt","a")
    f.write(nowdate+','+log.replace('\r','%').replace('\n','%') + '\n')
    f.close()
    
def count_msg():
    """
    Gets the # of SMS messages stored in GSM modem.
    Usage: c = countmsg()
    """
    while True:
        b = ''
        c = ''
        b = gsmcmd('AT+CPMS?')
        
        try:
            c = int( b.split(',')[1] )
            print '>> Received', c, 'message/s'
            return c
        except IndexError:
            print 'count_msg b = ',b
            # logError(b)
            if b:
                return 0                
            else:
                return -1
                
            ##if GSM sent blank data maybe GSM is inactive
        except ValueError:
            print '>> ValueError:'
            print b
            print '>> Retryring message reading'
            logError(b)
            # return -2   

def get_sms_from_sim(network):
    allmsgs = 'd' + gsmcmd('AT+CMGL="ALL"')
    allmsgs = allmsgs.replace("\r\nOK\r\n",'').split("+CMGL")[1:]
    # if allmsgs:
        # temp = allmsgs.pop(0) #removes "=ALL"
        
    msglist = []
    
    for msg in allmsgs:
        # if SaveToFile:
            # mon = dt.now().strftime("-%Y-%B-")
            # f = open("D:\\Server Files\\Consolidated\\"+network+mon+'backup.txt','a')
            # f.write(msg)
            # f.close()
                
        msg = msg.replace('\n','').split("\r")
        try:
            txtnum = re.search(r': [0-9]{1,2},',msg[0]).group(0).strip(': ,')
        except AttributeError:
            # particular msg may be some extra strip of string 
            print ">> Error: message may not have correct construction", msg[0]
            logError("wrong construction\n"+msg[0])
            continue
        
        try:
            sender = re.search(r'[0-9]{11,12}',msg[0]).group(0)
        except AttributeError:
            print 'Sender unknown.', msg[0]
            sender = "UNK"
            
        try:
            txtdatetimeStr = re.search(r'\d\d/\d\d/\d\d,\d\d:\d\d:\d\d',
                msg[0]).group(0)
            txtdatetime = dt.strptime(txtdatetimeStr,'%y/%m/%d,%H:%M:%S')
            txtdatetime = txtdatetime.strftime('%Y-%m-%d %H:%M:00')
        except:
            print "Error in date time conversion"
                
        smsItem = sms(txtnum, sender, msg[1], txtdatetimeStr)
        
        msglist.append(smsItem)
        
    
    return msglist
        