#!/usr/bin/env python

def fibonacci():
    last = 1
    current = 1
    while(True):
        yield current
        nxt = last + current
        last = current
        current = nxt


N = 10
print('Two fibonacci sequences generated in parallel:')
generator1 = fibonacci()
print('seq #1: %d' % generator1.next())
print('seq #1: %d' % generator1.next())

generator2 = fibonacci()
for i in range(N):
    print('seq #1: %d' % generator1.next())
    print('seq #2:       %d' % generator2.next())


