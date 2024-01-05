
import os
import sys
import time



def upcontainer(hostname):
    output = os.popen(f"ssh masters2 'cd /root/mcmpi/docker/wperf22-app; python3 utils.py up {hostname} > /dev/null 2>&1'").read().strip()
    #print(output)


#print(sys.argv)
#['spawn-container.py', '10']
size = int(sys.argv[1])

#idx = int(open("container-idx").read().strip())
idx, qt = open("container-idx").read().strip().split("-")
# 1-1 = 1 instance of server01 already spawned

max_per_host = 2
idx = int(idx)
qt = int(qt)

for i in range(size):
    if qt < max_per_host:
        server = f"server{idx:02}"
        qt += 1
    else:
        idx+=1
        server = f"server{idx:02}"
        qt = 1
    #print(f"# to start {server}")
    upcontainer(server)
    while True:
        try:
            ret = os.system(f"ssh {server} hostname > /dev/null 2>&1")
            if ret == 0:
                break
        except:
            time.sleep(1)
    print(server)

with open("container-idx", "w") as f:
    f.write(f"{idx}-{qt}")













