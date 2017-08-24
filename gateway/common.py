import ConfigParser, os, serial
import memcache
import gsmio
import pandas as pd
from datetime import datetime as dt

def get_mc_server():
    return memcache.Client(['127.0.0.1:11211'],debug=0)	

mc = get_mc_server()
smsoutbox_column_names = ["ts","sms_msg","user_id","read_status"]

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

def reset_smsoutbox_memory():
	smsoutbox = pd.DataFrame(columns = smsoutbox_column_names)
	mc.set("smsoutbox",smsoutbox)

def print_smsoutbox_memory():
	print mc.get("smsoutbox")	

def save_sms_to_memory(msg_str):
	# read smsoutbox from memory
	smsoutbox = mc.get("smsoutbox")

	# set to an empty df if empty
	if smsoutbox is None:
		reset_smsoutbox_memory()

	# prep the data to append
	data = {"ts": [dt.today()],	"sms_msg": [msg_str], 
	"user_id": [1], "read_status" : [0]}

	# append the data
	smsoutbox = smsoutbox.append(pd.DataFrame(data), 
		ignore_index = True)

	# save to memory
	mc.set("smsoutbox",smsoutbox)

def main():
	mc = get_mc_server()
	
	# new server config
	c = dewsl_server_config()
	mc.set("server_config",c.config)

	smsoutbox = mc.get("smsoutbox")
	if smsoutbox is None:
		mc.set("smsoutbox",[])
		print "set smsoutbox as empty list"
	else:
		print smsoutbox

	cfg = mc.get("server_config")
	for key in cfg.keys():
		print key, cfg[key]
	# print c.config['gsmdb']['username']

if __name__ == "__main__":
    main()