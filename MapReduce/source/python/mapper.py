#!/usr/bin/env python


import sys

line = sys.stdin.readline()
while line:
  a = line.split()  
  for i in range(1, len(a)-1):
    for j in range(i+1, len(a)):
      sa = sorted([a[0], a[i], a[j]])
      print '%s %s %s,%d' % (sa[0], sa[1], sa[2], 1)
  line = sys.stdin.readline()
