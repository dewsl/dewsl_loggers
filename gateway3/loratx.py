#!/usr/bin/env python

""" A simple beacon transmitter class to send a 1-byte message (0x0f) in regular time intervals. """

# Copyright 2015 Mayer Analytics Ltd.
#
# This file is part of pySX127x.
#
# pySX127x is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# pySX127x is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
# details.
#
# You can be released from the requirements of the license by obtaining a commercial license. Such a license is
# mandatory as soon as you develop commercial activities involving pySX127x without disclosing the source code of your
# own applications, or shipping pySX127x with a closed source product.
#
# You should have received a copy of the GNU General Public License along with pySX127.  If not, see
# <http://www.gnu.org/licenses/>.

from time import sleep
import sys
sys.path.insert(0, '/home/pi/pySX127x')
from SX127x.LoRa import *
from SX127x.LoRaArgumentParser import LoRaArgumentParser
from SX127x.board_config import BOARD

BOARD.setup()

parser = LoRaArgumentParser("A simple LoRa beacon")
parser.add_argument('--single', '-S', dest='single', default=False, action="store_true", help="Single transmission")
parser.add_argument('--wait', '-w', dest='wait', default=1, action="store", type=float, help="Waiting time between transmissions (default is 0s)")
parser.add_argument('--message', '-m', dest='message', default='', action="store", type=str, help="Message to be sent")

tmpmsg = [
    '>>PHILO*x*030B024FBFE8F830F0BF7348000082100BFF3D0FEDF80070B044E2FE1F7D050B0641B0FBF800D0B044360E6F7F020B014F4F1D083130B004DAF38083<<',
    '>>PHILO*x*0C0B004E6FBCF7E010BFD31601E087110BFF31001007D0B0B004050F3F7F090B0141D0D6F7D0E0BFF3AEFFAF85040BFA33B0C4F7F0A0BFE3F0F2607F140B004C4FEBF84<<',
    '>>PHILO*y*0C0CFE3E3FD2F7E010CF930001D087110C0140B00807D0B0CFC3E6FEDF7F090C0041F0F3F7D0E0C024BFFEEF850A0C034E5F2407F040C034350D6F7F140C014CDFE0F84<<', 
    '>>PHILO*y*030C024EDFF5F830F0CF432F00D082100C014D3FE3F80070C014E1FFBF7D050C0941A01B0800D0C054330E2F7F020C03415026083130CFC3D9F53083<<',
    '>>PHILO*d*0C16042A011603FC111604170B160416091604290E1604110A16042F0416042A141604160316041E0F160418101604110716042A0516040F0D16041E0216041D13160418<<']

class LoRaBeacon(LoRa):

    tx_counter = 0

    def __init__(self, verbose=False):
        super(LoRaBeacon, self).__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_dio_mapping([1,0,0,0,0,0])

    def on_rx_done(self):
        print("\nRxDone")
        print((self.get_irq_flags()))
        print((list(map(hex, self.read_payload(nocheck=True)))))
        self.set_mode(MODE.SLEEP)
        self.reset_ptr_rx()
        self.set_mode(MODE.RXCONT)

    def on_tx_done(self):
        global args
        self.set_mode(MODE.STDBY)
        self.clear_irq_flags(TxDone=1)
        sys.stdout.flush()
        self.tx_counter += 1
        sys.stdout.write("\rtx #%d" % self.tx_counter)
        if args.single:
            print()
            sys.exit(0)
        #BOARD.led_off()
        sleep(args.wait)
        preamble = [0xff, 0xff, 0x00, 0x00]
        try:
            rawinput = tmpmsg[self.tx_counter]
        except IndexError:
            print(">>Done sending all data")
            sys.exit(0)
        #rawinput = "hello LORA" #raw_input(">>> ")
        converted = [int(hex(ord(c)), 0) for c in rawinput]
        data = preamble + converted + [0x00]
        #self.write_payload([0x0f])
        self.write_payload(data)
        #BOARD.led_on()
        self.set_mode(MODE.TX)

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
        global args
        sys.stdout.write("\rstart")
        self.tx_counter = 0
        #BOARD.led_on()
        preamble = [0xff, 0xff, 0x00, 0x00]
        #rawinput = args.message
        rawinput = tmpmsg[self.tx_counter]
        #raw_input(">>> ")
        converted = [int(hex(ord(c)), 0) for c in rawinput]
        data = preamble + converted + [0x00]
        #self.write_payload([0x0f])
        self.write_payload(data)
        self.set_mode(MODE.TX)
        while True:
            sleep(1)

lora = LoRaBeacon(verbose=False)
args = parser.parse_args(lora)

lora.set_pa_config(pa_select=1)
#lora.set_rx_crc(True)
#lora.set_agc_auto_on(True)
#lora.set_lna_gain(GAIN.NOT_USED)
#lora.set_coding_rate(CODING_RATE.CR4_6)
#lora.set_implicit_header_mode(False)
#lora.set_pa_config(max_power=0x04, output_power=0x0F)
#lora.set_pa_config(max_power=0x04, output_power=0b01000000)
#lora.set_low_data_rate_optim(True)
#lora.set_pa_ramp(PA_RAMP.RAMP_50_us)


print(lora)
#assert(lora.get_lna()['lna_gain'] == GAIN.NOT_USED)
assert(lora.get_agc_auto_on() == 1)

print("Beacon config:")
print(("  Wait %f s" % args.wait))
print(("  Single tx = %s" % args.single))
print("")
try: print("") #input("Press enter to start...")
except: pass

try:
    lora.start()
except KeyboardInterrupt:
    sys.stdout.flush()
    print("")
    sys.stderr.write("KeyboardInterrupt\n")
finally:
    sys.stdout.flush()
    print("")
    lora.set_mode(MODE.SLEEP)
    print(lora)
    BOARD.teardown()
