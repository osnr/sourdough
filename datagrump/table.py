import sys, re

f = open(sys.argv[1], 'r')

results = []
winsize = -1
throughput = -1
for line in f:
    winmatch = re.search(r'WINDOW_SIZE=([0-9]+)', line)
    if winmatch:
        winsize = int(winmatch.group(1))
        continue

    tpmatch = re.search(r'Average throughput: ([0-9\.]+) Mbits/s', line)
    if tpmatch:
        throughput = float(tpmatch.group(1))
        continue
    delaymatch = re.search(r'95th percentile signal delay: ([0-9]+) ms', line)
    if delaymatch:
        signaldelay = float(delaymatch.group(1))
        if signaldelay > 4000: continue # skip

        results.append((winsize, throughput, signaldelay))
        continue

from tabulate import tabulate
print tabulate(results, headers=["Window", "TP (Mbits/s)", "Delay (ms)"])

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
plt.scatter(*zip(*[(delay, tp) for _, tp, delay in results]))

for winsize, tp, delay in results:
    plt.annotate('%d' % winsize, (delay, tp))

plt.gca().invert_xaxis()
plt.title('Fixed window size trades off throughput and delay')
plt.xlabel('95th percentile signal delay (ms)')
plt.ylabel('Throughput (Mbits/s)')
plt.savefig('fig.png')
