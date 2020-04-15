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
cmd9 = ['python3', '/pi/home/gateway3/gateway.py', '-rd']

def execute(cmd):
   op = subprocess.run(cmd, stdout = subprocess.PIPE, stderr = subprocess.PIPE, universal_newlines = True)
   print(op.stdout, op.stderr)
   return op.returncode

def getDatetime():
    execute(cmd2)

def setDatetime():
   ip1 = input("Set correct datetime (YYYY-MM-DD HH:MM:SS): ")
   if isCorrectDatetime(ip1):
      cmd3.append(ip1)
      if execute(cmd3) == 0:
         execute(cmd4)
   else:
      print("Invalid format")

def getServerconfig():
   #execute(cmd5)
   op1 = common.mc.get("server_config")['coordinfo']['name']
   op2 = common.mc.get("server_config")['coordinfo']['simnet']
   op3 = str(common.mc.get("server_config")['serverinfo']['simnum'])
   print("\nSitecode: {}\nGSM Network: {}\nServer Num: {}\n".format(op1, op2, op3))

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
    S: Print ServerConfig Settings\n\
    X: EXIT\n")
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
      elif option == 'R':
         modRain.check_rain_value(reset_rain = True)
      elif option == 'S':
         getServerconfig()
      elif option == 'X':
         print("Exiting...")
         global exitFlag
         exitFlag = 1
   else:
      print("Reselect.")

exitFlag = 0
debugOptions = ['C', 'D', 'G', 'R', 'S', 'X']
while True:
   main()
   if exitFlag == 1:
      break

