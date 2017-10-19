import memcache
import common
import dbio
import pandas as pd
import struct
import serial
import sys
import time
from xbee import XBee, ZigBee
import argparse
import re
import signal
import lockscript
from datetime import datetime as dt

DEST_ADDR_LONG = "\x00\x00\x00\x00\x00\x00\xff\xff" #

def get_network_info():
	sc = common.get_config_handle()
	# mc = common.get_mc_server()
	site_code = sc['coordinfo']['name']

	# get info from mysql tables
	query = ("select r.name, r.xbee_addr_short, r.xbee_addr_long from "
		"gateways g inner join routers r on g.gateway_id = r.gateway_id "
		"where g.code = '%s';") % (site_code)

	router_info = dbio.query_database(query,"gni")

	# print router_info

	# routers = pd.DataFrame(columns = ['name','addr_short','addr_long'])

	network_info = {}
	network_info['site_code'] = site_code
	network_info['router_name_by_addr_long'] = {}
	network_info['router_name_by_addr_short'] = {}
	network_info['addr_short_list'] = []
	network_info['addr_long_list'] = []
	network_info['router_addr_long_by_name'] = {}
	network_info['router_addr_short_by_name'] = {}

	for name, addr_short, addr_long in router_info:
		addr_long_packed = struct.pack('>q',int(addr_long,16))
		network_info['router_name_by_addr_long'][addr_long_packed] = name
		network_info['router_name_by_addr_short'][addr_short] = name
		network_info['addr_short_list'].append(addr_short)
		network_info['addr_long_list'].append(addr_long_packed)
		network_info['router_addr_long_by_name'][name] = addr_long_packed
		network_info['router_addr_short_by_name'][name] = addr_short

	print network_info
	common.mc.set('network_info',network_info)

def reset(xbee):
	print '>> Sending reset command to routers ..'

	#poweroff
	xbee.remote_at(
	dest_addr_long=DEST_ADDR_LONG, 			
		command="D1",
		parameter='\x04')
		#frame_id="A")
		
	time.sleep(2)
	
	# #poweron
	xbee.remote_at(
		dest_addr_long=DEST_ADDR_LONG, 			
		command="D1",
		parameter='\x00')
		#frame_id="A")
		
	# ser.close()

	print '>> Reset done ..'
	return

# def routine():
# 	reset()

def get_rssi(xbee):
	net_info = common.mc.get('network_info')

	rssi_msg = "GATEWAY*RSSI,"

	rssi_info = {}

	# cycle through addr_long
	for addr_long in net_info['addr_long_list']:
		router_name = net_info['router_name_by_addr_long'][addr_long]
		
		# print addr_long
		# send rssi command
		xbee.remote_at(	
			dest_addr_long = addr_long, 			
			command="DB", 
			frame_id="A")
		
		# parse response
		response=xbee.wait_read_frame()
		stat = response['status']
		stat = ord(stat)
		
		# evaluate response
		if stat == 0:
			# there is a connection                           
			par_db = response['parameter']
			par_db = ord(par_db)
			print "%s is alive. RSSI = -%d dBm" % (router_name, par_db)
			# rssi_msg += ",%s,%d," % (router_name, par_db)
			rssi_info[router_name] = par_db
		else:
			print "Can't connect to", router_name
			rssi_info[router_name] = ""
			# rssi_msg += ",%s,," % (router_name)

	# rssi_msg = re.sub('[^A-Zbcxy0-9\,]',"",rssi_msg)
	# print rssi_msg

	return rssi_info

def wakeup(xbee):
	xbee.send("tx", data = "Wake up and get data\n", 
		dest_addr_long = DEST_ADDR_LONG, dest_addr = "\xff\xfe")
	resp = xbee.wait_read_frame()
	print "Wake up"
	
	# ser.close()
	return

        sys.exit()

def receive(xbee):
	
	paddr=""
	i=0
	sfin=[]
	fin=[]
	net_info = common.mc.get("network_info")
	router_volt_msg = "Router voltages:"

	router_tilt_soms_msg = {}
	voltage_info = {}
	
	while True:
		print "\nwaiting for packets ... "

		try:
			response = xbee.wait_read_frame()
		except SampleTimeoutException, KeyboardInterrupt:
			print 'Timeout!'
			break

		#print response
		rf = response['rf_data']
		print rf 
		rf=str(rf)
		datalen=len(rf)
		
		paddr = ""
		paddr = paddr + hex(int(ord(response['source_addr_long'][4])))
		paddr = paddr + hex(int(ord(response['source_addr_long'][5])))
		paddr = paddr + hex(int(ord(response['source_addr_long'][6])))
		paddr = paddr + hex(int(ord(response['source_addr_long'][7])))

		if paddr in net_info['addr_long_list']:
			router_name = net_info['router_name_by_addr_long'][paddr]
		elif paddr in net_info['addr_short_list']:
			router_name = net_info['router_name_by_addr_short'][paddr]
		else:
			print ">> Error: unknown address", paddr
			continue

		print ">> Packet from: %s" % (router_name)
		
		hashStart=rf.find('#')
		
		slashStart=rf.find("/")
		
		if rf.find('VOLTAGE') is not -1:
			# voltage info packet
			try:
				volt = re.search("(?<=\#)[0-9\.]+(?=\<)",rf).group(0)
			except NameError, TypeError:
				volt = ""
				print ">> Error in volt conversion", rf
			# volt= re.sub('[^.0-9\*]',"",volt)
			print "%s: %s" % (router_name, volt)
			voltage_info[router_name] = volt
		else:
			# tilt or soms packet

			msg = rf[hashStart+1:-1]
			msg = re.sub('[^A-Zxyabc0-9\*]',"",msg)

			try:
				# append message to existing packet
				router_tilt_soms_msg[router_name] += msg
				
				if (rf.find("<") != -1):
					# end of sms msg
					router_tilt_soms_msg[router_name] += ","
			except KeyError:
				# new packet
				router_tilt_soms_msg[router_name] = msg
			
		# exit routine here for coding
		
			
	return router_tilt_soms_msg, voltage_info

def get_arguments():
    parser = argparse.ArgumentParser(description = "Gateway debug [-options]")
    parser.add_argument("-n", "--get_network_info", 
        help = "loads network info", action = 'store_true')
    parser.add_argument("-r", "--get_rssi", 
        help = "retrieves rssi data from routers")
    parser.add_argument("-s", "--sample_routers", 
        help = "execute sampling routine for routers", action = 'store_true')

    try:
        args = parser.parse_args()
        return args        
    except IndexError:
        print '>> Error in parsing arguments'
        error = parser.format_help()
        print error

class SampleTimeoutException(Exception):
	pass

def signal_handler(signum, frame):
	raise SampleTimeoutException("Timed out!")

def routine(xbee):
	reset(xbee)
	time.sleep(5)
	rssi_info = get_rssi(xbee)
	time.sleep(2)
	wakeup(xbee)

	sc = common.get_config_handle()

	signal.signal(signal.SIGALRM,signal_handler)
	signal.alarm(sc['xbee']['sampletimeout'])

	# try:
	router_tsm_msgs, voltage_info = receive(xbee)
	# except KeyboardInterrupt:
		# print '>> Uesr timeout!'

	for key in router_tsm_msgs.keys():
		for msg in router_tsm_msgs[key].split(",")[:-1]:
			# msg_cleaned = re.sub('[^A-Zxyabc0-9\*]',"",msg)
			# print msg_cleaned
			print msg
			common.save_sms_to_memory(msg)

	# rssi and voltage message
	sc = common.get_config_handle()
	site_code = sc['coordinfo']['name']
	rssi_msg = "GATEWAY*RSSI,%s" % (site_code)
	for key in rssi_info.keys():
		try:
			rssi_msg += ",%s,%s,%s" % (key,rssi_info[key],voltage_info[key])
		except KeyError:
			rssi_msg += ",%s,%s,," % (key,rssi_info[key])
	rssi_msg += dt.now().strftime("*%y%m%d%H%M%S")
	print rssi_msg
	common.save_sms_to_memory(rssi_msg)

def main():
	# get_network_info()
	# print common.mc
	lockscript.get_lock('xbeegate')
	args = get_arguments()

	sc = common.get_config_handle()

	try:
		ser = serial.Serial(sc['xbee']['port'], sc['xbee']['baud'])
		xbee = ZigBee(ser,escaped=True)
	except serial.serialutil.SerialException:
		print '>> Error: No serial port'
		# send error message to server here
		sys.exit()

	
	# get_rssi(xbee)
	if args.get_network_info:
		get_network_info()
	if args.get_rssi:
		for i in range(0,int(args.get_rssi)):
			print get_rssi(xbee)
	if args.sample_routers:
		routine(xbee)


	# ser.close()

if __name__=='__main__':
    try:
	    main()
    except KeyboardInterrupt:
        print "Aborting ..."
