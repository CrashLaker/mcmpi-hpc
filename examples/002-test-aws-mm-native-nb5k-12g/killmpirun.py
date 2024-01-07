import os
import base64



hosts = open("/etc/hosts").read()

hosts2 = []
for line in hosts.split("\n"):
    if "172" in line and not "controller" in line:
        hosts2.append(line.split(" ")[0])


for host in hosts2:
    cmd = "ps aux | grep mpirun | awk '{print $2}' | xargs kill -9"
    b64 = base64.b64encode(cmd.encode('utf-8')).decode('utf-8')
    os.system(f"ssh {host} \"echo '{b64}' | base64 -d | bash -x\"")
