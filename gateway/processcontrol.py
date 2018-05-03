from crontab import CronTab
import argparse

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


def main():

	gateway_cron = CronTab(user='pi')

	parser = argparse.ArgumentParser(description="Control crontab items\n PC [-options]")
	parser.add_argument("-v", "--version", help="switch between new and old versions")

	args = parser.parse_args()

	if args.version:
		change_version(gateway_cron, args.version)

if __name__ == "__main__":
    main()