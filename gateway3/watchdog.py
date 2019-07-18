import subprocess
import os
from datetime import datetime
import pprint

def main():
    procs = ['pgrep -f ringservice', 'pgrep -f raindetect']
    logfile = '/home/pi/logs/inactive_log.txt'
    for proc in procs:
        process = subprocess.Popen(proc, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        my_pid, err = process.communicate()
        
        if len(my_pid.splitlines()) > 1:
            print("Proc: "+proc+" is running")
        else:
            print("Proc: "+proc+" is not running")
            message = "******"+proc.upper()+" Script Inactive*******\n" \
                      "Script Status: Active\n" \
                      "Reboot ts: "+ts+"\n" \
                      "*******************************" 

            if('rain' in proc): 
                status = os.system("screen -dmS rain python3 ~/gateway3/raindetect.py")
            elif('ring' in proc):
                status = os.system("screen -dmS ring python3 ~/gateway3/ringservice.py")
            
            with open(logfile,'a+') as fh:
                fh.write(message+"\n\n")

if __name__ == "__main__":
    main()
 