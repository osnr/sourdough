import sys, re

print sys.argv[1]

fromsendtoack = []

def forline(msg):
    sent = re.search(r'At time ([0-9]+) sent datagram ([0-9]+)', msg)
    if sent:
        time, num = int(sent.group(1)), int(sent.group(2))
        return

    windowsize = re.search(r'At time ([0-9]+) window size is ([0-9]+)', msg)
    if windowsize:
        time, win = int(windowsize.group(1)), int(windowsize.group(2))
        return

    ack = re.search(r'At time ([0-9]+) received ack for datagram ([0-9]+) \(send @ time ([0-9]+), received @ time ([0-9]+) by receiver\'s clock\)', msg)
    if ack:
        acktime, num, senttime, recvtime = int(ack.group(1)), int(ack.group(2)), int(ack.group(3)), int(ack.group(4))
        fromsendtoack.append(acktime - senttime)
        return

    print 'no match', msg

acks = open(sys.argv[1]).read().strip().split('\n')
nums = [forline(ack) for ack in acks]

import numpy as np
import matplotlib.pyplot as plt
plt.hist(fromsendtoack, bins=sorted(list(set(fromsendtoack))))
plt.show()



