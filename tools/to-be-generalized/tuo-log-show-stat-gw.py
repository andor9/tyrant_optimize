#!/usr/bin/python2
# -*- coding: utf-8 -*-

import os
import re
import sys

file_names = sys.argv[1:]

# tuo-exp-gw.attack.ded4-vs-ddd.cs_lc.fa_fa.e[gbge:ybge:ebge].climb.log
FNAME_FORMAT = re.compile(
    r'^tuo-exp-gw'
    r'.(?P<ad>attack|defense)'
    r'.(?P<commander>[\w+]+)-vs-(?P<enemy>[\w:;+-]+)'
    r'.(?P<yfort>\w+).(?P<efort>\w+)'
     '.e\[(?P<bges>[a-zA-Z0-9:\' -]+)\]'
    r'.(?P<algo>\w+).(?:(?P<order>ordered|random)\.)?log$'
)

# Optimized Deck: 6 units: 22.2: Typhon Vex, Soul Annex, Syndicate Goliath, ...
# Optimized Deck: 8 units: (0.443299% stall) 83.8144: Daedalus-4, Dreamhaunter, Jilted Baughe, Soul Annex, ...
RECORD_FORMAT = re.compile(
    r'^(?P<fin>Optimized Deck: )?(?P<units>\d+) units:'
    r'(?: \$(?P<cost>\d+))?'
    r'(?: \((?P<stall>[0-9.]+)% stall\))?'
    r' (?P<win_rate>[0-9.]+):'
    r' (?P<deck>.*)$'
)

REFIND_FORMAT = re.compile(
    r'^Results refined: (?P<new>[0-9.]+)'
)

# Deck improved: KAhNWwVJ+VJ+eeAhZCDhHBEh: [123] A -> [456] B: 94.684 (191 123 41 / 200)'
ITERS_FORMAT = re.compile(
    r'^(Deck improved: .*: )?[0-9.]+ \((\d+ )+/ (?P<iters>\d+)\)$'
)

# PID
PID_FORMAT = re.compile(
    '^pid: (?P<pid>\d+)'
)


##
##  -s (sim) mode
##

# Dracorex Hivelord, Unirager, Tormentor, Tormentor, Overmind Bane, Sacrificial Chamber, Cimex Parasite, Vile Emergence, Vile Emergence, Vile Emergence, Riktox Liquefier
# win%: 82.4078 (1837 1880 ... / <iters>)
# stall%: 0.0936906 (0 2 1 ... / <iters>)
# loss%: 17.4986 (163 118 ... / <iters>)

SIM_DECK_FORMAT = re.compile(
    '^(?P<deck>(?P<fin>[^,:#()]+(?:, [^,:\\n]+)+))$'
)

SIM_WIN_RATE_FORMAT = re.compile(
    '^win%: (?P<win_rate>[0-9.]+) \((?:\d+ )+/ (?P<iters>\d+)\)$'
)

SIM_STALL_RATE_FORMAT = re.compile(
    '^stall%: (?P<stall>[0-9.]+) \((?:\d+ )+/ (?:\d+)\)$'
)


result_key = lambda fname_attrs, result_attrs: tuple(map(lambda x: (x, fname_attrs[x]), ['ad', 'yfort', 'efort', 'bges', 'enemy']))
result_key_sort_key = lambda key: key
result_key_header = lambda key: "\n\n  -       -    -  - - -----  ({ad})  [{yfort}]×[{efort}]  / vs /  {enemy}  ----- - -  -    -       -\n".format(**dict(key))

results = dict()

max_ybge_len = 2
max_ebge_len = 2
max_ad_len = 1
max_cost_len = 1

for fname in file_names:
    m = FNAME_FORMAT.match(os.path.basename(fname))
    if not m:
        raise Exception('bad file name (format violation): %s' % fname)
    fname_attrs = m.groupdict()
    fname_attrs['ybge'] = fname_attrs['bges'].split(':', 2)[1]
    fname_attrs['ebge'] = fname_attrs['bges'].split(':', 2)[2]
    max_ybge_len = max(max_ybge_len, len(fname_attrs['ybge']))
    max_ebge_len = max(max_ebge_len, len(fname_attrs['ebge']))
    max_ad_len = max(max_ad_len, len(fname_attrs['ad']))
    result_attrs = {}
    with open(fname, 'r') as f:
        algo = fname_attrs['algo']
        if algo == 'sim':
            for x in f:
                for fmt in (SIM_DECK_FORMAT, SIM_WIN_RATE_FORMAT, SIM_STALL_RATE_FORMAT):
                    m = fmt.match(x)
                    if m:
                        if (fmt == SIM_DECK_FORMAT):
                            if result_attrs.has_key('deck'):
                                break
                            result_attrs['units'] = x.count(',')
                        result_attrs.update(m.groupdict())
                        break
        elif algo in ('climbex', 'climb'):
            m_rec = None
            m_imp = None
            m_pid = None
            refind_new_value = None
            for x in f:
                m = REFIND_FORMAT.match(x)
                if m:
                    refind_new_value = m.group('new')
                    continue

                m = RECORD_FORMAT.match(x)
                if m:
                    m_rec = m
                    refind_new_value = None
                    continue

                m = ITERS_FORMAT.match(x)
                if m:
                    m_imp = m
                    continue

                m = PID_FORMAT.match(x)
                if m:
                    m_pid = m
                    continue
            if None in (m_rec, m_imp, m_pid):
                raise Exception('file not ready yet: %s' % fname)

            result_attrs['is_alive'] = False
            if m_pid is not None:
                result_attrs.update(m_pid.groupdict())
                pid = int(m_pid.group('pid'))
                cmdline = '/proc/{}/cmdline'.format(pid)
                if os.path.isfile(cmdline):
                    with open('/proc/{}/cmdline'.format(pid), 'rb') as f_cmdline:
                        data = f_cmdline.read().split('\x00')
                        if os.path.split(data[0])[1] == 'tuo':
                            result_attrs['is_alive'] = True
            result_attrs.update(m_rec.groupdict())
            result_attrs.update(m_imp.groupdict())
            if refind_new_value:
                result_attrs['win_rate'] = refind_new_value
        else:
            raise Exception("unsupported algo: {}".format(algo))
        win_rate = float(result_attrs['win_rate'])
        res_key = result_key(fname_attrs, result_attrs)
        if res_key not in results:
            results[res_key] = list()
        results[res_key].append((fname, fname_attrs, result_attrs, win_rate))
        if not result_attrs.has_key('cost') or result_attrs['cost'] is None:
            result_attrs['cost'] = '--'
        max_cost_len = max(max_cost_len, len(result_attrs['cost']))

out_fmt = (
    '{{proc_state}} I{{result_attrs[iters]:<3}} {{win_rate:6.1f}}%'
    '  ({{fname_attrs[ad]:^{max_ad_len}}})'
    '  $[ {{result_attrs[cost]:<{max_cost_len}}} ]'
    '  [{{fname_attrs[yfort]:^5}}]×[{{fname_attrs[efort]:^5}}]'
    '  [{{fname_attrs[ybge]:^{max_ybge_len}}}] × [{{fname_attrs[ebge]:^{max_ebge_len}}}]'
    ' :: <{{result_attrs[units]:>2}} unt> {{result_attrs[deck]}}'
).format(**globals())

sorted_keys = sorted(results.keys(), key=result_key_sort_key)
for k in sorted_keys:
    print result_key_header(k)
    sorted_by_winrate_desc = sorted(results[k], key=lambda t: -t[3]) # sort by win_rate (index=4)
    for x in sorted_by_winrate_desc:
        out_attrs = {
            "proc_state": x[2]['fin'] and 'FIN' or x[2]['is_alive'] and 'RUN' or 'RIP',
            "fname_attrs": x[1],
            "result_attrs": x[2],
            "win_rate": x[3],
        }
        print out_fmt.format(**out_attrs)
