#!/usr/bin/env python3

from time import sleep
import sys
sys.path.insert(0, '/home/pi/pySX127x')
from SX127x.LoRa import *
from SX127x.LoRaArgumentParser import LoRaArgumentParser
from SX127x.board_config import BOARD
import common
import re
from datetime import datetime as dt
import signal
import lockscript
import dbio

BOARD.setup()

parser = LoRaArgumentParser("Continous LoRa receiver.")


class LoRaRcvCont(LoRa):
    def __init__(self, verbose=False):
        super(LoRaRcvCont, self).__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_dio_mapping([0] * 6)

    def on_rx_done(self):
        #BOARD.led_on()
        print("\nRxDone")
        self.clear_irq_flags(RxDone=1)
        payload = self.read_payload(nocheck=True)
        data = ''.join([chr(c) for c in payload])
        print(">> Saving...")
        #print data
        #common.save_sms_to_memory(data)
        data = re.sub(r'[^a-zA-z0-9*<>#/:.\-,+]',"",data)
        timestamp = dt.now().strftime("%y%m%d%H%M%S")
        rssi = self.get_pkt_rssi_value()*-1
        print("RSSI: " + str(rssi))
        if data.startswith('>>'):
            print(">> Valid lora data.")
            hashStart = data.find('#')
            if hashStart != -1:
                data = data[hashStart+1:-1]
            data = data[:data.find('<<')]
            data = re.sub(r'[<>]',"",data)
            router_name = data[:5]
            global router_list
            global rssi_info
            global volt_info
            if router_name in router_list:
                if rssi_info[router_name] == '':
                    rssi_info[router_name] = str(rssi)
                if 'VOLT' in data:
                    volt_info[router_name] = data[11:data.find('*',11)]
                    print(data)
            dts = data #+ '*' + timestamp
            #common.save_sms_to_memory(dts)
            if 'VOLT' not in dts:
                dbio.write_sms_to_outbox(dts)
        else:
            print(">> Invalidated.")
            dts = data + '*INVALID*' + timestamp
            filename = "/home/pi/logs/lora_data.txt"
            with open(filename,'a+') as fh:
                fh.write(dts+'\n')
            print(dts)
            print(">> Saved to text file.")
        #print(bytes(payload).decode())
        self.set_mode(MODE.SLEEP)
        self.reset_ptr_rx()
        #BOARD.led_off()
        self.set_mode(MODE.RXCONT)

    def on_tx_done(self):
        print("\nTxDone")
        print((self.get_irq_flags()))

    def on_cad_done(self):
        print("\non_CadDone")
        print((self.get_irq_flags()))

    def on_rx_timeout(self):
        print("\non_RxTimeout")
        print((self.get_irq_flags()))

    def on_valid_header(self):
        print("\non_ValidHeader")
        print((self.get_irq_flags()))

    def on_payload_crc_error(self):
        print("\non_PayloadCrcError")
        print((self.get_irq_flags()))

    def on_fhss_change_channel(self):
        print("\non_FhssChangeChannel")
        print((self.get_irq_flags()))

    def start(self):
        self.reset_ptr_rx()
        self.set_mode(MODE.RXCONT)
        ctr=0
        while True:
            ctr += 1
            sleep(1)
            #self.rssi_value = self.get_rssi_value()
            #status = self.get_modem_status()
            sys.stdout.flush()
            #sys.stdout.write("\r%d %d %d" % (self.rssi_value, status['rx_ongoing'], status['modem_clear']))
            sys.stdout.write("\r%s %d" %('waiting for packets', ctr))

def signal_handler(signum, frame):
    raise SampleTimeoutException("Timed out!")

class SampleTimeoutException(Exception):
    pass

lockscript.get_lock('lora')
lora = LoRaRcvCont(verbose=False)
args = parser.parse_args(lora)

lora.set_mode(MODE.STDBY)
lora.set_pa_config(pa_select=1)
#lora.set_rx_crc(True)
#lora.set_coding_rate(CODING_RATE.CR4_6)
#lora.set_pa_config(max_power=0, output_power=0)
#lora.set_lna_gain(GAIN.G1)
#lora.set_implicit_header_mode(False)
#lora.set_low_data_rate_optim(True)
#lora.set_pa_ramp(PA_RAMP.RAMP_50_us)
#lora.set_agc_auto_on(True)

#print(lora)
print("Starting LoRa")
assert(lora.get_agc_auto_on() == 1)

rxtimeout = common.mc.get("server_config")['lora']['rxtimeout']
site_code = common.mc.get("network_info")['site_code']
router_list = common.mc.get("network_info")['router_addr_short_by_name'].keys()
rssi_info = {}
volt_info = {}
for router in router_list:
    rssi_info[router] = ''
    volt_info[router] = ''

def build_rssi_msg():
    global site_code
    global rssi_info
    global volt_info
    timestamp = dt.now().strftime("%y%m%d%H%M%S")
    rssi_msg = 'GATEWAY*RSSI,{},'.format(site_code)
    for router in sorted(rssi_info.keys()):
        rssi_msg += '{},{},{},'.format(router, rssi_info[router], volt_info[router])
    rssi_msg += '*{}'.format(timestamp)
    return rssi_msg
 
#try: input("Press enter to start...")
#except: pass

try:
    signal.signal(signal.SIGALRM, signal_handler)
    signal.alarm(rxtimeout)
    lora.start()
except KeyboardInterrupt:
    sys.stdout.flush()
    print("")
    sys.stderr.write("KeyboardInterrupt\n")
except SampleTimeoutException:
    sys.stdout.flush()
    print("")
    #save rssi info: dbio.    
    sys.stderr.write("TimeoutInterrupt\n")
finally:
    sys.stdout.flush()
    print("")
    lora.set_mode(MODE.SLEEP)
#    print(lora)
    print("Terminating")
    BOARD.teardown()
    dbio.write_sms_to_outbox(build_rssi_msg())
