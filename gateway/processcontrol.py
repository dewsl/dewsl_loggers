from crontab import CronTab
import argparse
import common
import re

def enable_cron(job_list, mode):
	for j in job_list:
		print 'job', repr(j)
		j.enable(mode)

def change_version(cron,ver):

	if ver == 'old':
		enable_cron(cron.find_command('/home/pi/'), True)
		enable_cron(cron.find_command('~/'), False)
	else:
		enable_cron(cron.find_command('/home/pi/'), False)
		enable_cron(cron.find_command('~/'), True)

	cron.write()

def change_report_interval(job_name, rep_int, cron=None):
	if not cron:
		cron = CronTab(user='pi')

	if job_name not in ['health','xbee']:
		print '>> Error: unknown job', job_name
		return 'ERROR: unknown job %s' % (job_name)

	job_list = cron.find_command(job_name)

	for j in job_list:
		j.minute.every(int(rep_int))

	cron.write()

	return 'CRON job %s changed to %s min report interval' % (job_name, rep_int)

def execute_script(script_name):
	out, err = common.spawn_process("ps ax | grep %s" % script_name)
	out_lines = re.split("[^(a-zA-Z0-9)]+\d{3,4}", out)

	if script_name == "raindetect":
		cmd_line_script = "screen -S rain -d -m sudo python ~/gateway/raindetect.py"
	elif script_name == "ringservice":
		cmd_line_script = "screen -S ring -d -m sudo python ~/gateway/ringservice.py"
	else:
		print ">> unknown script_name", script_name
		return
	
	if len(out_lines)<4:
		print ">> Executing script in background"
		common.spawn_process(cmd_line_script)
	else:
		print ">> Process already running"
	# print len(out_lines)
	# print out_lines

def main():

	gateway_cron = CronTab(user='pi')

	parser = argparse.ArgumentParser(description="Control crontab items\n PC [-options]")
	parser.add_argument("-v", "--version", help="switch between new and old versions")
	parser.add_argument("-xr", "--exe_rain", help="execute rain detect script if not running", 
		action = 'store_true')
	parser.add_argument("-xg", "--exe_ring", help="execute rain detect script if not running", 
		action = 'store_true')


	args = parser.parse_args()

	if args.version:
		change_version(gateway_cron, args.version)
	if args.exe_rain:
		execute_script("raindetect")
	if args.exe_ring:
		execute_script("ringservice")
		

if __name__ == "__main__":
    main()