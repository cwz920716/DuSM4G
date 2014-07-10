import os
import random

def chunk(size):
  if (size < 16):
    return 1
  elif (size < 128):
    return 64
  elif (size < 512):
    return 256
  elif (size < 2048):
    return 1024
  else:
    return 2048

def chunk_gap(size):
  if (size < 16):
    return 10
  elif (size < 128):
    return 20
  elif (size < 512):
    return 400
  elif (size < 2048):
    return 2000
  else:
    return 10 * 1000

def nextchunk(left, total):
  c = chunk(total)
  if left < c:
    return left
  else:
    return c

nhost = 315
fin = file('rock1239/group.txt', 'r');
grptfc = []
ngroup = 0
while True:
    line = fin.readline()
    if len(line) == 0:
        break
    tuples = line.split()
    tfc = int(tuples[1])
    grptfc.append(tfc)
    ngroup = ngroup + 1

fin = file('rock1239/gkid.txt', 'r')
nline = int(fin.readline())
kids = []
for i in range(0, ngroup):
    kids.append([])
while True:
    line = fin.readline()
    if len(line) == 0:
        break
    tuples = line.split()
    gid = int(tuples[0])
    kid = int(tuples[1])
    kids[gid].append(kid)

for i in range(0, ngroup):
    if len(kids[i]) <= 1 and grptfc[i] > 0:
        for j in range(0, ngroup):
           if len(kids[j]) > 1 and grptfc[j] == 0:
               tmp = grptfc[i]
               grptfc[i] = grptfc[j]
               grptfc[j] = tmp
               break
timestamp = {}
groupbase = {}
MS = 1000 * 1000
for h in range(0, nhost):
  timestamp[h] = int(random.random() * MS * 2)

for g in range(0, ngroup):
  groupbase[g] = int(random.random() * MS)

largest = 0
for g in range(0, ngroup):
  m = len(kids[g])
  t = grptfc[g]
  for h in kids[g]:
    if (t > 0):
      size_h = random.randint(0, t)
      if h == kids[g][m - 1] :
        size_h = t
      t = t - size_h
      total_h = size_h
      while size_h > 0:
        cs = nextchunk(size_h, total_h)
        if (timestamp[h] < groupbase[g]):
          timestamp[h] = groupbase[g]
        timestamp[h] = timestamp[h] + chunk_gap(cs)
        print '$ns at %f \"$d(%s) post %s %s\"' % ((int(timestamp[h])) / 1000.0, h, g, cs)
        if (timestamp[h] > largest):
          largest = timestamp[h]
        size_h = size_h - cs
print '--END'
