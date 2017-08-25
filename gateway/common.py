import ConfigParser, os, serial
import memcache
import gsmio
import pandas as pd
from datetime import datetime as dt
import dbio

def get_mc_server():
    return memcache.Client(['127.0.0.1:11211'],debug=0)	

mc = get_mc_server()
storage_column_names = ["ts","msg","contact_id","stat"]

def read_cfg_file():
	cfg = ConfigParser.ConfigParser()
	cfgfiletxt = 'server_config.txt'
	cfile = os.path.dirname(os.path.realpath(__file__)) + '/' + cfgfiletxt
	cfg.read(cfile)
	return cfg

def save_cfg_changes(cfg):
    with open(cfile, 'wb') as c:
        cfg.write(c)

class Container(object):
	pass

class dewsl_server_config:
	def __init__(self):
		self.version = 1

		cfg = read_cfg_file()

		self.config = dict()  

		for section in cfg.sections():
			options = dict()
			for opt in cfg.options(section):

				try:
					options[opt] = cfg.getboolean(section, opt)
					continue
				except ValueError:
					# may not be booelan
					pass

				try:
					options[opt] = cfg.getint(section, opt)
					continue
				except ValueError:
					# may not be integer
					pass

				# should be a string
				options[opt] = cfg.get(section, opt)

			# setattr(self, section.lower(), options)
			self.config[section.lower()] = options

def get_config_handle():
	return mc.get('server_config')

def reset_memory(valuestr):
	value_pointer = mc.get(valuestr)

	if value_pointer is None:
		value_pointer = pd.DataFrame(columns = storage_column_names)
		mc.set(valuestr,value_pointer)		
		print "set %s as empty object" % (valuestr)
	else:
		print value_pointer

def print_memory(valuestr):
	print mc.get(valuestr)

def save_smsinbox_to_memory():
	allmsgs = gsmio.get_sms_from_sim()

	phonebook_inv = mc.get("phonebook_inv")
	smsinbox = mc.get("smsinbox")

	print phonebook_inv

	for m in allmsgs:
		data = {"ts": [m.ts], "msg": [m.msg], 
		"contact_id": [phonebook_inv[m.simnum]], "stat" : [0]}

		smsinbox = smsinbox.append(pd.DataFrame(data), 
		ignore_index = True)

	mc.set("smsinbox",smsinbox)
	gsmio.delete_read_messages()

def save_phonebook_memory():
	query = "select pb_id, sim_num from phonebook"
	pb_result_set = dbio.query_database(query,"spm")
	
	phonebook = {}
	for pb_id, sim_num in pb_result_set:
		phonebook[pb_id] = sim_num

	phonebook_inv = {}
	for pb_id, sim_num in pb_result_set:
		phonebook_inv[sim_num] = pb_id

	mc.set("phonebook",phonebook)
	mc.set("phonebook_inv",phonebook_inv)

def purge_memory(valuestr):
	value_pointer = mc.get(valuestr)

	value_pointer = value_pointer[value_pointer["stat"] == 0]
	mc.set(valuestr,value_pointer)

def save_sms_to_memory(msg_str):
	# read smsoutbox from memory
	smsoutbox = mc.get("smsoutbox")

	# set to an empty df if empty
	if smsoutbox is None:
		reset_smsoutbox_memory()
	# else:
	# 	print smsoutbox

	# prep the data to append
	data = {"ts": [dt.today()],	"msg": [msg_str], 
	"contact_id": [3], "stat" : [0]}

	# append the data
	smsoutbox = smsoutbox.append(pd.DataFrame(data), 
		ignore_index = True)

	# save to memory
	mc.set("smsoutbox",smsoutbox)

def main():
	# new server config
	c = dewsl_server_config()
	mc.set("server_config",c.config)

	reset_memory("smsoutbox")
	reset_memory("smsinbox")

	cfg = mc.get("server_config")
	for key in cfg.keys():
		print key, cfg[key]
	# print c.config['gsmdb']['username']

	save_phonebook_memory()

if __name__ == "__main__":
    main()