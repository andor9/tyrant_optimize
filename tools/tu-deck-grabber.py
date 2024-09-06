#!/usr/bin/python3

import os
import sys
import re
import time
import pickle
import urllib
import urllib3
import json
import readline
import atexit
import certifi
import configparser
import optparse

from random import randint

from xml.dom import minidom
from xml.dom.minidom import Element

from urllib3.util.timeout import Timeout
from urllib3 import PoolManager, Retry

XML_DIR = "~/3pp/tyrant_optimize/data/"

DEFAULT_USER_DB_PATH = "~/.tu-deck-grabber.udb"
DEFAULT_CONFIG_PATH = "~/.tu-deck-grabber.ini"

#PROTOCOL = "http"
#API_HOST = "localhost:8000"
PROTOCOL = "https"
API_HOST = "mobile.tyrantonline.com"
API_PATH = "api.php"


STATIC_HEADERS = {
    "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:38.0) Gecko/20100101 Firefox/38.0",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.5",
    "Accept-Encoding": 'gzip, compress',
    "Connection": "keep-alive",
    "Content-Type": "application/x-www-form-urlencoded",
}


BASIC_BODY_PARAMS = {
    "unity": "Unity4_6_6",
    "client_version": "68",
    "device_type": "Intel(R)+Pentium(R)+4+CPU+2.40GHz+(7830+MB)",
    "os_version": "Windows+XP+Service+Pack+3+(5.1.2600)",
    "platform": "Web",
}

# parse options
parser = optparse.OptionParser(
    usage="Usage: %prog [options]"
)

parser.add_option("-u", "--user", metavar='LOGIN', action="store", help="select user")
parser.add_option("-c", "--config", metavar='FILE',
    action="store", default = DEFAULT_CONFIG_PATH,
    help="specify config file [default: %default]"
)
parser.add_option("-b", "--user-db", metavar='FILE',
    action="store", default = DEFAULT_USER_DB_PATH,
    help="specify user-db file [default: %default]"
)
(options, args) = parser.parse_args()

# parse config
config = configparser.ConfigParser()
config_fname = os.path.expanduser(options.config)
if not os.path.exists(config_fname):
    print("ERROR: no such config file: {}".format(config_fname))
    sys.exit(1)
config.read(config_fname)

# setup personal params
login = options.user or config['CORE']['default_login']
if login not in config:
    print("ERROR: no section named '{}' in config file: {}".format(login, config_fname))
    sys.exit(1)
PERSONAL_BODY_PARAMS = dict(config[login].items())
PERSONAL_URL_PARAMS = {}
PERSONAL_URL_PARAMS["user_id"] = PERSONAL_BODY_PARAMS["user_id"]

def getFirstChildNode(n, nodeType, nodeName=None):
    if not n:
        return None
    try:
        return next(filter(
            lambda n: n.nodeType == nodeType and (
                nodeName is None or n.nodeName == nodeName
            ), n.childNodes
        ))
    except StopIteration:
        return None

def getAllChildNodes(n, nodeType, nodeName=None):
    if not n:
        return None
    return list(filter(
        lambda n: n.nodeType == nodeType and (
            nodeName is None or n.nodeName == nodeName
        ), n.childNodes
    ))

def getFirstChildElementNode(n, nodeName):
    return getFirstChildNode(n, Element.ELEMENT_NODE, nodeName)

def getFirstChildTextNodeValue(n):
    try:
        return getFirstChildNode(n, Element.TEXT_NODE).nodeValue
    except AttributeError:
        return None

def getAllChildElementNodes(n, nodeName):
    return getAllChildNodes(n, Element.ELEMENT_NODE, nodeName)


class UserDbEntry:
    def __init__(self, kv):
        self.user_id = int(kv['user_id'])
        self.name = kv['name']
        self.guild_name = (kv['guild_name'] or '') if ('guild_name' in kv) else ''
        self.level = int(kv['level'])
        self.elo = int(kv['hunting_elo'])
        self._refreshed_at = int(time.time())
    def __cmp__(self, other):
        if self.user_id > other.user_id: return 1
        if self.user_id < other.user_id: return -1
        if self.name > other.name: return 1
        if self.name < other.name: return -1
        if self.guild_name > other.guild_name: return 1
        if self.guild_name < other.guild_name: return -1
        if self.level > other.level: return 1
        if self.level < other.level: return -1
        return 0
    def __eq__(self, other):
        return self.__cmp__(other) == 0
    def __str__(self):
        return "[{}] {} ({} lvl / elo: {})".format(
                self.guild_name, self.name, self.level, self.elo
            )

class UserDb:
    def __init__(self, entries = None):
        self.id2entry = dict((x.user_id, x) for x in (entries or []))
        self._dirty = False
    def append(self, entry):
        self.id2entry[entry.user_id] = entry
        self._refreshed_at = int(time.time())
        self._dirty = True
    def getByUserId(self, uid):
        if uid in self.id2entry:
            return self.id2entry[uid]
        return None
    def size(self):
        return len(self.id2entry)

user_db = UserDb()

udb_fname = os.path.expanduser(options.user_db)
if os.path.exists(udb_fname):
    with open(udb_fname, 'rb') as f:
        user_db = pickle.load(f)
        print("INFO: user-db(total {} entries) loaded from dump: {}".format(user_db.size(), udb_fname))

id_to_card_name = {}

for i in range(1, 100):
    xml_fname = os.path.join(os.path.expanduser(XML_DIR), 'cards_section_{}.xml'.format(i))
    if not os.path.exists(xml_fname):
        break
    st = os.stat(xml_fname)
    xml_file_hash = int(((st.st_mtime * 31) + i) * 31 + st.st_size) & 0xFFFFFFFF
    print("INFO: next cards xml file: {} (hash: 0x{:08x})".format(xml_fname, xml_file_hash))

    dump_fname = re.sub('\.xml$', '.pck', xml_fname)
    if os.path.exists(dump_fname):
        with open(dump_fname, 'rb') as f:
            xhash = pickle.load(f)
            xmap = pickle.load(f)
            if xhash == xml_file_hash:
                id_to_card_name.update(xmap)
                print("INFO: image of {} loaded from dump: {}".format(xml_fname, dump_fname))
                continue
            print("INFO: reloading {}: dumped hash(0x{:08x}) <> actual hash(0x{:08x})".format(xml_fname, xhash, xml_file_hash))

    file_cards_by_id = {}
    xml = minidom.parse(xml_fname)
    root = getFirstChildElementNode(xml, 'root')
    for unit in getAllChildElementNodes(root, 'unit'):
        card_id = int(getFirstChildTextNodeValue(getFirstChildElementNode(unit, 'id')))
        card_name = getFirstChildTextNodeValue(getFirstChildElementNode(unit, 'name'))
        if card_name is None:
            print("Warning: file {}: found an unit(id={}) without name".format(xml_fname, card_id))
            continue
        level_raw = getFirstChildTextNodeValue(getFirstChildElementNode(unit, 'level'))
        level = level_raw and int(level_raw) or 1
        top_level_id = card_id
        file_cards_by_id[card_id] = "{}-{}".format(card_name, level)
        for upgrade in getAllChildElementNodes(unit, 'upgrade'):
            card_id = int(getFirstChildTextNodeValue(getFirstChildElementNode(upgrade, 'card_id')))
            level = int(getFirstChildTextNodeValue(getFirstChildElementNode(upgrade, 'level')))
            top_level_id = card_id
            file_cards_by_id[card_id] = "{}-{}".format(card_name, level)
        file_cards_by_id[top_level_id] = card_name

    # dump file image
    with open(dump_fname, 'wb') as f:
        pickle.dump(xml_file_hash, f)
        pickle.dump(file_cards_by_id, f)
        print("INFO: {} parsed & image saved to dump: {}".format(xml_fname, dump_fname))
    id_to_card_name.update(file_cards_by_id)

#for id, name in id_to_card_name.items():
#    print("{}: {}".format(id, name))



def mkUrlParams(kvmap):
    return "&".join("{}={}".format(k, v) for (k, v) in kvmap.items())

def getHuntingTargets(http, retries_on_empty=3):
    try_no = 0
    data = None
    while (try_no < retries_on_empty):
        try_no += 1
        x = {"message": "getHuntingTargets"}
        x.update(PERSONAL_URL_PARAMS)
        url_params = mkUrlParams(x)
        x = {
            "api_stat_name": "getHuntingTargets",
            "api_stat_time": str(randint(22,777)),
            "data_usage": str(randint(11111, 888888)),
            "timestamp": str(int(time.time())),
            "dummy": "data",
        }
        x.update(BASIC_BODY_PARAMS)
        x.update(PERSONAL_BODY_PARAMS)
        body_params = mkUrlParams(x)
        r = http.request(
            'POST', '{}://{}/{}?{}'.format(PROTOCOL, API_HOST, API_PATH, url_params),
            headers = STATIC_HEADERS,
            decode_content = True,
            preload_content = False,
            body = body_params
        )
        data = r.read().decode("UTF-8")
        if not data:
            print("WARN: getHuntingTargets: no data (try #{})".format(try_no))
            continue
        with open('/tmp/.tu-deck-grabber.getHuntingTargets.last', 'wb') as f:
            f.write(bytes(data, 'UTF-8'))
            f.flush()
        return json.loads(data)
    print("ERROR: getHuntingTargets: no data (all tries are spent)")
    return None

def getBattleResults(http, retries_on_empty=3):
    try_no = 0
    data = None
    while (try_no < retries_on_empty):
        try_no += 1
        x = {"message": "getBattleResults"}
        x.update(PERSONAL_URL_PARAMS)
        url_params = mkUrlParams(x)
        x = {
            "api_stat_name": "getBattleResults",
            "api_stat_time": str(randint(22,777)),
            "data_usage": str(randint(11111, 888888)),
            "timestamp": str(int(time.time())),
            "battle_id": "0",
            "host_id": "0",
        }
        x.update(BASIC_BODY_PARAMS)
        x.update(PERSONAL_BODY_PARAMS)
        body_params = mkUrlParams(x)
        r = http.request(
            'POST', '{}://{}/{}?{}'.format(PROTOCOL, API_HOST, API_PATH, url_params),
            headers = STATIC_HEADERS,
            decode_content = True,
            preload_content = False,
            body = body_params
        )
        data = r.read().decode("UTF-8")
        if not data:
            print("WARN: getBattleResults: no data (try #{})".format(try_no))
            continue
        with open('/tmp/.tu-deck-grabber.getBattleResults.last', 'wb') as f:
            f.write(bytes(data, 'UTF-8'))
            f.flush()
        return json.loads(data)
    print("ERROR: getBattleResults: no data (all tries are spent)")
    return None

def getCardNameById(card_id):
    if card_id in id_to_card_name:
        return id_to_card_name[card_id]
    return '[{}]'.format(card_id)

def doHuntAndEnrichUserDb(http):
    global orig_res
    res = orig_res = getHuntingTargets(http)
    if (not res) or ('hunting_targets' not in res):
        return
    res = res['hunting_targets']
    for (uid, entry) in res.items():
        new_entry = UserDbEntry(entry)
        old_entry = user_db.getByUserId(new_entry.user_id)
        if old_entry is None:
            user_db.append(new_entry)
            print("INFO: user-db: added entry: {}".format(new_entry))
        else:
            if new_entry == old_entry:
                continue
            print("INFO: user-db: upgraded entry: {} -> {}".format(old_entry, new_entry))
            user_db.append(new_entry)
    if user_db._dirty:
        user_db._dirty = False
        tmp_udb_fname = udb_fname + '~'
        with open(tmp_udb_fname, 'wb') as f:
            pickle.dump(user_db, f)
        if (os.path.exists(udb_fname)):
            os.remove(udb_fname)
        os.renames(tmp_udb_fname, udb_fname)
        print("INFO: user-db: synced to disk: {} (total {} entries)".format(udb_fname, user_db.size()))

def doGrabLastDeck(http):
    global orig_res
    res = orig_res = getBattleResults(http)
    if (not res) or ('battle_data' not in res):
        return
    res = res['battle_data']
    enemy_id = int(res['enemy_id'] or -1)
    enemy_name = res['enemy_name'] if ('enemy_name' in res) else '__UNNAMED__'
    enemy_size = int(res['eds'] or 1000)
    host_size = int(res['hds'] or 1000)
    enemy_udb_entry = user_db.id2entry[enemy_id] if (enemy_id in user_db.id2entry) else None
    end_time = int(res['end_time']) if ('end_time' in res) else int(time.time())
    winner = int(res['winner']) if ('winner' in res) else None
    rewards_list = res['rewards'] if ('rewards' in res) else None
    host_is_attacker = bool(res['host_is_attacker'])
    enemy_commander_id = int(res['defend_commander' if host_is_attacker else 'attack_commander'])
    card_map = dict((int(k), int(v)) for (k, v) in res['card_map'].items())
    is_attacker_card = lambda x: 1 <= x <= 50
    is_attacker_fort = lambda x: 51 <= x < 100
    is_defender_card = lambda x: 101 <= x <= 150
    is_defender_fort = lambda x: 151 <= x < 200
    is_attacker_both = lambda x: is_attacker_card(x) or is_attacker_fort(x)
    is_defender_both = lambda x: is_defender_card(x) or is_defender_fort(x)
    enemy_predicate = is_defender_both if host_is_attacker else is_attacker_both
    enemy_card_predicate = is_defender_card if host_is_attacker else is_attacker_card
    enemy_fort_predicate = is_defender_fort if host_is_attacker else is_attacker_fort
    enemy_card_id_to_count = {}
    enemy_forts = []
    enemy_played_cards_count = 0
    for (card_uid, card_id) in card_map.items():
        if not enemy_predicate(card_uid):
            continue
        if enemy_card_predicate(card_uid):
            if card_id in enemy_card_id_to_count:
                enemy_card_id_to_count[card_id] += 1
            else:
                enemy_card_id_to_count[card_id] = 1
        elif enemy_fort_predicate(card_uid):
            enemy_forts.append(card_id)
        else:
            pass # TODO notice?

        if not enemy_fort_predicate(card_uid):
            enemy_played_cards_count += 1

    # determine game type
    game_type = 'Arena'
    pvp_points = None
    if not host_is_attacker:
        game_type = 'Unknown'
        if rewards_list is not None:
            for rewards in rewards_list:
                if 'pvp_points' in rewards:
                    pvp_points = int(rewards['pvp_points'])
                    game_type = 'Brawl'
                    break
                if 'war_points' in rewards:
                    game_type = 'GW'
                if 'conquest_influence' in rewards:
                    game_type = 'CQ'

    # deck header
    enemy_guild_name = enemy_udb_entry and enemy_udb_entry.guild_name or '__UNKNOWN__'
    out = ""
    if config.getboolean('CORE', 'output_game_type'):
        out += game_type + '.'
    if config.getboolean('CORE', 'output_timestamp'):
        out += time.strftime('%Y%m%d', time.localtime(end_time)) + '.'
    if config.getboolean('CORE', 'output_winlose'):
        if winner is not None:
            out += 'Win.' if winner else 'Lose.'
        else:
            out += 'BIP.' # Battle In Progress
    if config.getboolean('CORE', 'output_pvp_points'):
        if pvp_points is not None:
            out += 'pvp{:02d}.'.format(pvp_points)
    if config.getboolean('CORE', 'output_hds'):
        out += "hds{:02d}.".format(host_size)
    if config.getboolean('CORE', 'output_eds'):
        out += "eds{:02d}.".format(enemy_size)
    if config.getboolean('CORE', 'output_guild'):
        out += re.sub('(?a)[^\w]', '_', enemy_guild_name) + '.'
    out += re.sub('(?a)[^\w]', '_', enemy_name)
    if config.getboolean('CORE', 'output_missing'):
        missing_cards = enemy_size - enemy_played_cards_count
        if (missing_cards > 0):
            out += ".m{}".format(missing_cards)
    out += ": "

    # append commander
    out += getCardNameById(enemy_commander_id)

    # append forts
    for card_id in enemy_forts:
        out += ", " + getCardNameById(card_id)

    # append cards
    card_ids_ordered = list(enemy_card_id_to_count.keys())
    card_ids_ordered.sort(reverse=False)
    for card_id in card_ids_ordered:
        count = enemy_card_id_to_count[card_id]
        out += ", " + getCardNameById(card_id)
        if (count > 1):
            out += " #" + str(count)

    print("Grabbed deck: " + out)

## configure readline
histfile = os.path.join(os.path.expanduser("~"), ".tu_deck_grabber_history")
if os.path.exists(histfile):
    readline.read_history_file(histfile)
readline.set_history_length(1000)
readline.read_init_file()
atexit.register(readline.write_history_file, histfile)

with PoolManager(1,
            timeout = Timeout(connect=15.0, read=20.0, total=30.0),
            retries = Retry(total=3),
            cert_reqs = 'CERT_REQUIRED',
            ca_certs = certifi.where(),
        ) as http:
    while True:
        try:
            line = input("{} ~> ".format(login))
        except EOFError:
            print()
            line = None
        if not line:
            break
        line = line.strip().lower()
        if line == 'exit':
            break
        if line == 'grab':
            doGrabLastDeck(http)
            continue
        if line == 'hunt':
            doHuntAndEnrichUserDb(http)
            continue
        print("ERROR: unknown command: {} (supported: [ grab | hunt | exit ])".format(line))
