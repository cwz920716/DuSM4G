import os

# Stage0: Prepare
template = file('template.tcl', 'r')

while True:
  line = template.readline()
  if line == '--END\n':
    break
  print line,

# Stage1: topology
fin = file('rock6461.topo', 'r')
edges = []
nEdge = 0
nNode = 0
while True:
    line = fin.readline()
    if len(line) == 0:
        break
    tuples = line.split()
    if tuples[0] == 'N':
        nNode = int(tuples[1])
    elif tuples[0] == 'E':
        nEdge = nEdge + 1
        edge = {}
        edge['u'] = int(tuples[1]) - 1
        edge['v'] = int(tuples[2]) - 1
        edge['w'] = float(tuples[3])
        edges.append(edge)

print 'for {set i 0} {$i < %d} {incr i} {' % nNode
print '    set n($i) [$ns node]'
print '    set d($i) [new Agent/DuSM]'
print '    $ns attach-agent $n($i) $d($i)'
print '    $d($i) addr $i'
print '}'
print '$d(0) init-gc'

for i in range(0, nEdge):
    print '$ns duplex-link $n(%d) $n(%d) 1Gb 0ms DropTail' % (edges[i]['u'], edges[i]['v'])


largest = 0
print
trace = file('trace.tcl', 'r')
while True:
    line = trace.readline()
    if line == '--END\n':
        break
    tuples = line.split()
    time = float(tuples[2])
    if time > largest:
        largest = time
    print line,
largest = largest + 1
 
print 'for {set i 0} {$i < %d} {incr i} {' % nNode
print '$ns at %f \"$d($i) dump-mcast\"' % largest
print '}'

for i in range(0, nEdge):
    print '$ns at %f \"$d(%s) dump-link-stat %s\"' % (largest, edges[i]['u'], edges[i]['v'])
    print '$ns at %f \"$d(%s) dump-link-stat %s\"' % (largest, edges[i]['v'], edges[i]['u'])


print 
print '$ns at %lf \"finish\"' % (largest + 1)
print "$ns run"
