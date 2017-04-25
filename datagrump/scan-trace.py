from collections import defaultdict

trace = open('/usr/local/share/mahimahi/traces/Verizon-LTE-short.down', 'r')

capacity_over_time = defaultdict(int)
for line in trace:
    time = int(line.strip())
    capacity_over_time[time] += 1

for time, capacity in capacity_over_time.iteritems():
    print time, capacity
