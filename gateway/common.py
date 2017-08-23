import ConfigParser, os, serial
import memcache

# USAGE
# 
# 
# import cfgfileio as cfg
# 
# s = cfg.config()
# print s.dbio.hostdb
# print s.io.rt_to_fill
# print s.io.printtimer
# print s.misc.debug


cfgfiletxt = 'server_config.txt'
cfile = os.path.dirname(os.path.realpath(__file__)) + '/' + cfgfiletxt
    
def read_cfg_file():
    cfg = ConfigParser.ConfigParser()
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

def get_mc_server():
    return memcache.Client(['127.0.0.1:11211'],debug=0)		

def get_config_handle():
	mc = get_mc_server()
	return mc.get('server_config')

def main():
	mc = memcache.Client(['127.0.0.1:11211'],debug=0)
	
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