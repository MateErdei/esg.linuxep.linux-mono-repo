import optparse
import yaml

op = optparse.OptionParser()
(options, args) = op.parse_args()

f = file(args[0], 'r')
t = f.read()
f.close()
y = yaml.safe_load(t)

r = {}
for c in y['components']:
    for w in c['warehouses']:
        l = r.setdefault(w['id'], [])
        l.append(c['instance-name'])

for w in r:
    print "%s contains:" % w
    for c in r[w]:
        print "    %s" % c
