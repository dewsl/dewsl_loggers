import ConfigParser, MySQLdb, time, sys, os
from datetime import datetime as dt
from ConfigParser import SafeConfigParser
import memcache
import gsmio
import common

# cfg = SafeConfigParser()
# cfg.read("server_config.txt")
#cfg.read("/home/pi/rpiserver/senslope-server-config.txt")


class DbInstance:
    def __init__(self,name,host,user,password):
       self.name = name
       self.host = host
       self.user = user
       self.password = password

# Definition: Connect to senslopedb in mysql
def db_connect():
    s_conf = common.get_config_handle()

    while True:
        try:
            db = MySQLdb.connect(host = s_conf['localdb']['host'],
                user = s_conf['localdb']['user'],
                passwd = s_conf['localdb']['password'],
                db = s_conf['localdb']['dbname']
            )
            cur = db.cursor()
            return db, cur
        except MySQLdb.OperationalError:
            print '6.',
            time.sleep(2)
            
def create_table(table_name, type):
    db, cur = db_connect()
    # cur.execute("CREATE DATABASE IF NOT EXISTS %s" %Namedb)
    # cur.execute("USE %s"%Namedb)
    
    if type == "runtime":
        cur.execute(("CREATE TABLE IF NOT EXISTS %s(ts datetime,"
            " script_name char(7), status char(10), PRIMARY KEY (ts,"
            " script_name))") %table_name)
    elif type == "gndmeas":
        cur.execute(("CREATE TABLE IF NOT EXISTS %s(ts datetime,"
            " meas_type char(10), site_id char (3), observer_name char(100),"
            " crack_id char(1), meas float(6,2), weather char(20), PRIMARY KEY "
            "(ts, meas_type, site_id, crack_id))") %table_name)
    elif type == "smsinbox":
        cur.execute(("CREATE TABLE IF NOT EXISTS %s(sms_id int unsigned"
            " not null auto_increment, ts datetime, sim_num varchar(20),"
            " sms_msg varchar(255), read_status varchar(20), PRIMARY KEY"
            " (sms_id))") %table_name)
    elif type == "smsoutbox":
        cur.execute(("CREATE TABLE IF NOT EXISTS %s(sms_id int signed not null"
        " auto_increment, ts_written datetime, ts_sent datetime,"
        " recepients varchar(255), sms_msg varchar(255), "
        "send_status varchar(20), PRIMARY KEY (sms_id))") %table_name)
    elif type == "raintips":
        cur.execute(("CREATE TABLE IF NOT EXISTS %s(ts datetime,"
            " tips smallint, PRIMARY KEY (ts))") %table_name)
    else:
        raise ValueError("ERROR: No option for creating table " + type)
        
    db.close()
    
def set_read_status(read_status,sms_id_list):
    db, cur = db_connect()
    
    if len(sms_id_list) <= 0:
        return

    query = ("update smsinbox set read_status = '%s'"
        " where sms_id in (%s) ") % (read_status,
         str(sms_id_list)[1:-1].replace("L",""))
    commit_to_db(query,"setReadStatus", instance='GSM')
    
def set_send_status(send_status,sms_id_list):
    db, cur = db_connect()
    
    if len(sms_id_list) <= 0:
        return
        
    now = dt.today().strftime("%Y-%m-%d %H:%M:%S")

    query = ("update smsoutbox set send_status = '%s',"
        " ts_written ='%s' where sms_id in (%s) ") % (send_status, 
        now, str(sms_id_list)[1:-1].replace("L",""))
    commit_to_db(query,"setSendStatus", instance='GSM')

def query_database(query, identifier='', instance='local'):
    db, cur = db_connect()
    a = ''

    # print query, identifier
    try:
        a = cur.execute(query)
        # db.commit()
        # if a:
        #     a = cur.fetchall()
        # else:
        #     # print '>> Warning: Query has no result set', identifier
        #     a = None
        try:
            a = cur.fetchall()
            return a
        except ValueError:
            return None
    except MySQLdb.OperationalError:
        a =  None
    except KeyError:
        a = None

def get_phonebook_numbers():
    print '>> Getting phonebook entries'
    query = 'select * from phonebook'

    pb_items = query_database(query)

    pb_numbers = {}
    pb_names = {}

    for item in pb_items:
        print item
        pb_id,name,sim_num = item
        pb_numbers[pb_id] = sim_num
        pb_names[pb_id] = name

    print pb_numbers
    print pb_names

    mc = common.get_mc_server()
    mc.set('pb_numbers',pb_numbers)
    mc.set('pb_names',pb_names)


def write_sms_to_inbox(msglist):
    query = "INSERT INTO smsinbox (timestamp,sim_num,sms_msg,read_status) VALUES "
    
    for m in msglist:
        query += "('" + str(m.dt.replace("/","-")) + "','" + str(m.simnum) + "','"
        query += str(m.data.replace("'","\"")) + "','UNREAD'),"
    
    # just to remove the trailing ','
    query = query[:-1]
    print query
    
    commitToDb(query, "WriteRawSmsToDb", instance='GSM')

def write_sms_to_outbox(sms_msg,pb_id=None,send_status=0):
    query = ("INSERT INTO smsoutbox (ts_written,pb_id,sms_msg,"
        "send_status) VALUES ")
    
    tsw = dt.today().strftime("%Y-%m-%d %H:%M:%S")

    mc = common.get_mc_server()
    inv_pb_names = mc.get('phonebook_inv')

    # default send to server if sim_num is empty
    if not pb_id:
        #mc = common.get_mc_server()
        s_conf = common.get_config_handle()

        #pb_names = mc.get('pb_names')
        #sendto = s_conf['serverinfo']['sendto']

        #inv_pb_names = {v: k for k, v in pb_names.items()}
        #print inv_pb_names
        #pb_id = inv_pb_names[sendto]

        pb_id = s_conf['serverinfo']['simnum']

    pb_id = inv_pb_names[str(pb_id)]

    query += "('%s','%s','%s','%s')" % (tsw,pb_id,sms_msg,send_status)
    
    print query
    
    commit_to_db(query, "WriteOutboxMessageToDb")

def update_smsoutbox_send_status(sms_id, send_status=15):
    now = dt.today().strftime("%Y-%m-%d %H:%M:%S")

    query = ("update smsoutbox set send_status = '%s',"
        " ts_sent ='%s' where sms_id = '%s' ") % (send_status, 
        now, sms_id)
    print query

    commit_to_db(query,"UpdateSendStatus", instance='GSM')    

    
def get_db_inbox(read_status):
    db, cur = db_connect()
    
    while True:
        try:
            query = ("select sms_id, ts, sim_num, sms_msg from smsinbox"
                " where read_status = '%s' limit 200") % (read_status)
        
            a = cur.execute(query)
            out = []
            if a:
                out = cur.fetchall()
            return out

        except MySQLdb.OperationalError:
            print '9.',
            
def get_db_outbox(send_status=0):
    db, cur = db_connect()
    
    while True:
        # try:
        query = ("select sms_id, pb_id, sms_msg"
            " from smsoutbox where send_status = %d"
            " limit 200") % (send_status)
    
        print query
        a = cur.execute(query)
        out = []
        if a:
            out = cur.fetchall()
            db.close()
            break
        else:
            print '>> No message to send'
            return
        # return out

    all_msgs = []
    for msg in out:
        print msg
        num,pb_id,sms_msg = msg
        sms_item = gsmio.SmsItem(num,pb_id,sms_msg,'')
        all_msgs.append(sms_item)

    return all_msgs

def commit_to_db(query, identifier, instance='local'):
    db, cur = db_connect()
    
    try:
        retry = 0
        while True:
            try:
                a = cur.execute(query)
                # db.commit()
                if a:
                    db.commit()
                    break
                else:
                    print '>> Warning: Query has no result set', identifier
                    db.commit()
                    time.sleep(0.5)
                    break
            # except MySQLdb.OperationalError:
            except IndexError:
                print '5.',
                #time.sleep(2)
                if retry > 10:
                    break
                else:
                    retry += 1
                    time.sleep(2)
    except KeyError:
        print '>> Error: Writing to database', identifier
    except MySQLdb.IntegrityError:
        print '>> Warning: Duplicate entry detected', identifier
    finally:
        db.close()
