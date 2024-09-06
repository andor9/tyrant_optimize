#!/usr/bin/python2

import os
import re
import sys

file_names = sys.argv[1:]

FNAME_FORMAT = re.compile(
    r'^tuo-exp-raid\.(?P<commander>[\w\+]+)'
    r'-vs-raid_(?P<level>\d+)_(?P<fort>\w+)'
    r'(?:\.mk_(?P<mk>\w+))?'
    r'\.effect\[(?P<effect>[\w-]+)\]'
    r'\.(?P<algo>\w+)\.(?:(?P<order>random|ordered)\.)?log$'
)

RECORD_FORMAT = re.compile(
    r'^(?P<fin>Optimized Deck: )?(?P<units>\d+) units:'
    r'(?: (?P<cost>\$\d+))?'
    r' \((?P<win_rate>[0-9.]+)% win\) (?P<score>[0-9.]+):'
    r' (?P<deck>.*)$'
)

REFIND_FORMAT = re.compile(
    r'^Results refined: '
)

ITERS_FORMAT = re.compile(
    r'^(Deck improved: .*: )?[0-9.]+ \(([0-9.]+ )+/ (?P<iters>\d+)\)$'
)

lvl2results = dict()

for fname in file_names:
    m = FNAME_FORMAT.match(os.path.basename(fname))
    if not m:
        raise Exception('bad file name (format violation): %s' % fname)
    fname_attrs = m.groupdict()
    result_attrs = {}
    with open(fname, 'r') as f:
        m_rec = None
        m_imp = None
        for x in f:
            if REFIND_FORMAT.match(x):
                continue

            m = RECORD_FORMAT.match(x)
            if m:
                m_rec = m
                continue

            m = ITERS_FORMAT.match(x)
            if m:
                m_imp = m
                continue
        if None in (m_rec, m_imp):
            raise Exception('file is not ready yet: %s' % fname)
        result_attrs.update(m_rec.groupdict())
        result_attrs.update(m_imp.groupdict())
        level = int(fname_attrs['level'])
        win_rate = float(result_attrs['win_rate'])
        score = float(result_attrs['score'])
        if not lvl2results.has_key(level):
            lvl2results[level] = list()
        lvl2results[level].append((fname, fname_attrs, result_attrs, win_rate, score))

levels = lvl2results.keys()
levels.sort()

for level in levels:
    results = lvl2results[level]
    from_worts_to_best = sorted(results, key=lambda t: -t[4]) # sort by score (index=4)
    print '\n\n    *** level #{} ***\n'.format(level)
    for x in from_worts_to_best:
        cost = x[2].has_key('cost') and x[2]['cost'] or "$(none) "
        print ' {proc_state} I{result_attrs[iters]:<6}'\
            '  {win_rate:5.1f}% / {score:<5.1f}  [ {cost:<6}]'\
            '  [{fname_attrs[fort]:^7}] ::'\
            ' <{result_attrs[units]:>2} unt> {result_attrs[deck]}'\
            .format(
                proc_state = x[2]['fin'] and 'FIN' or 'RUN',
                fname_attrs = x[1],
                result_attrs = x[2],
                win_rate = x[3],
                score = x[4],
                cost = cost
            )
