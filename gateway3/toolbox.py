import subprocess
import datetime
import gsmio as modGsm
import raindetect as modRain
import common

cmd1 = ['date']
cmd2 = ['sudo', 'hwclock', '-r']
cmd3 = ['sudo', 'date', '-s']
cmd4 = ['sudo', 'hwclock', '-w']
cmd5 = ['sudo', 'cat', '/boot/server_config.txt']
cmd6 = ['python3', '-u', '/home/pi/gateway3/lorarx.py', '-f', '433']
cmd7 = ['python3', '/home/pi/gateway3/gateway.py', '-ir']
cmd9 = ['python3', '/home/pi/gateway3/gateway.py', '-rd']

def execute(cmd, printer = True):
    op = subprocess.run(cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE, universal_newlines = True)
    out, err = op.stdout, op.stderr
    if printer:
        print(out, err)
    return op.returncode

def getDatetime():
    execute(cmd2)

def setDatetime():
    ip1 = input("Set correct datetime (YYYY-MM-DD HH:MM:SS): ")
    if isCorrectDatetime(ip1):
        if execute(cmd3+[ip1]) == 0:
            execute(cmd4)
    else:
        print("Invalid format")

def setConfigMem():
    execute(cmd7, printer = False)

def getLoraData():
    with subprocess.Popen(cmd6, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True) as p:
        for line in p.stdout:
            print(line, end='') # process line here

def getServerconfig():
    #execute(cmd5)
    op1 = common.mc.get("server_config")['coordinfo']['name']
    op2 = common.mc.get("server_config")['coordinfo']['simnet']
    op3 = str(common.mc.get("server_config")['serverinfo']['simnum'])
    print("\nSitecode: {}\nGSM Network: {}\nServer Num: {}\n".format(op1, op2, op3))

def loadConfigFile():
    with open(configfile) as f:
        configlist = f.readlines()
    return configlist

def buildConfigFile():
    with open(configfile, 'w+') as f:
        for item in configlist:
            f.write(item)

def findItemInConfig(configlist):
    i=0
    iName, iSimnet, iSimnum = None, None, None
    for line in configlist:
        if line.startswith('name'):
            iName = i
        elif line.startswith('simnet'):
            iSimnet = i
        elif line.startswith('simnum'):
            iSimnum = i
        i += 1
    return iName, iSimnet, iSimnum

def setSiteCode():
    print("V4 SITES: ", end = '')
    for site in v4sites:
        print(site, end = ' ')
    newSiteCode = input("\nInput correct SITE CODE (press Enter to skip): ").upper()
    if isCorrectSiteCode(newSiteCode):
        configlist[iName] = "name = {}\n".format(newSiteCode)
        return True
    else:
        return False

def setSimNet():
    print(gsmnetwork)
    newSimNet = input("Input correct SIM NETWORK - GLOBE/SMART (press Enter to skip): ").upper()
    if isCorrectSimNet(newSimNet):
        configlist[iSimnet] = "simnet = {}\n".format(newSimNet)
        configlist[iSimnum] = "simnum = {}\n".format(gsmnetwork[newSimNet])
        return True
    else:
        return False

def isCorrectSiteCode(newSiteCode):
    if (len(newSiteCode) == 3) and (newSiteCode in v4sites + testsite):
        return True
    else:
        if newSiteCode != '':
            print("Invalid input: {}".format(newSiteCode))
        return False

def isCorrectSimNet(newSimNet):
    if (len(newSimNet) == 5) and (newSimNet in gsmnetwork.keys()):
        return True
    else:
        if newSimNet != '':
            print("Invalid input: {}".format(newSimNet))
        return False

def isCorrectDatetime(dt):
    try:
        datetime.datetime.strptime(dt, '%Y-%m-%d %H:%M:%S')
        return True
    except ValueError:
        return False

def isSet():
    flag = input("Change (y/n)? ")
    if (flag.lower() == 'y'):
        return True
    else:
        print("OK")
        return False

def printMenu():
    print("="*50)
    print("Select option:\n\
     C: Print Debug Menu\n\
     D: Datetime\n\
     G: GSM CSQ+Network\n\
     R: Rain Gauge\n\
     S: ServerConfig Settings\n\n\
     E: EXIT")
    print("="*50)

def main():
    printMenu()
    option = input().upper()
    if option in debugOptions:
        if option == 'C':
            pass
        elif option == 'D':
            getDatetime()
            if isSet():
                setDatetime()
        elif option == 'G':
            modGsm.check_csq()
            modGsm.check_network()
        elif option == 'L':
            getLoraData()
        elif option == 'R':
            modRain.check_rain_value(reset_rain = True)
        elif option == 'S':
            getServerconfig()
            if isSet():
                if setSiteCode() + setSimNet() > 0:
                    buildConfigFile()
                    setConfigMem()
                    print("\nNew configuration settings:", end='')
                    getServerconfig()
        elif option == 'E':
            print("Exiting...")
            global exitFlag
            exitFlag = 1
    else:
        print("Reselect.")

exitFlag = 0
debugOptions = ['C', 'D', 'G', 'R', 'S', 'E']
configfile = "/home/pi/gateway3/server_config.txt"
v4sites = ['MAR', 'MAG', 'LUN', 'PEP', 'UMI', 'BTO', 'IMU', 'BAK', 'LTE', 'MSL', 'BAY', 'INA']
testsite = ['PHI', 'MAD']
gsmnetwork = {'GLOBE':639175972526, 'SMART':639088125642}
configlist = loadConfigFile()
iName, iSimnet, iSimnum = findItemInConfig(configlist)

while True:
    try:
        main()
        if exitFlag == 1:
            break
    except Exception as e:
        print("Report the ff error: ", e)
