#!/usr/bin/env python

from operator import itemgetter
import sys

line0 = sys.stdin.readline()
if line0:
  key0, val0 = line0.split(',')
  c = 1

line = sys.stdin.readline()
while line:
  key, val = line.split(',')
  if key == key0:
    c+=1
  elif c == 3:
    k = key0.split()
    print '<%s,%s,%s>' % (k[0],k[1],k[2])
    print '<%s,%s,%s>' % (k[1],k[0],k[2])
    print '<%s,%s,%s>' % (k[2],k[0],k[1])
    c = 1
    key0 = key
    val0 = val
  else:
    c = 1 
    key0 = key
    val0 = val
  line = sys.stdin.readline()
if c == 3:
  k = key0.split()
  print '<%s,%s,%s>' % (k[0],k[1],k[2])
  print '<%s,%s,%s>' % (k[1],k[0],k[2])
  print '<%s,%s,%s>' % (k[2],k[0],k[1])
