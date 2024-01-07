import boto3
import time
import os
import sys
import json

#print("worker01")
#print("worker02")
#sys.exit(0)


c = boto3.client('ec2', region_name='us-east-1')
ec2 = boto3.resource('ec2', region_name='us-east-1')


def phead(data, lines=10, width=200):
    print("\n".join(pprint.pformat(data, indent=4, width=width).splitlines()[:lines]))

def finder_dict(doc_, tree_, defval=""):
    if type(tree_) is list:
        if tree_[0][0] == '[' and type(doc_) is list: # array case .e.g meta.[0].name
            idx = int(tree_[0].replace('[', '').replace(']', ''))
            if idx < len(doc_):
                if len(tree_) == 1:
                    return doc_[idx]
                else:
                    return finder_dict(doc_[idx], tree_[1:], defval)
            else:
                return defval
        elif tree_[0] in doc_:
            if len(tree_) == 1:
                return doc_[tree_[0]]
            else:
                return finder_dict(doc_[tree_[0]], tree_[1:], defval)
        else:
            return defval
    else:
        return finder_dict(doc_, tree_.split("."), defval)


userdata = ""

instanceName = 'node04'
instanceName = 'node03'
instanceName = 'node02'
instanceName = 'node01'
instanceName = 'worker'

indexpath = "./server-idx"

sidx, qt = open(indexpath).read().strip().split("-")
max_per_host = 8
sidx = int(sidx)
qt = int(qt)

instanceName = f"workerSpawning"

#instanceName = 'nodeCluster'
#instanceName = 'ftp-test'
prefixname = ""

if len(sys.argv) != 2:
    instancesSize = 1
else:
    instancesSize = int(sys.argv[1])

args = {
    'ImageId': 'ami-0c2b8ca1dad447f8a', # amazon linux 2,
    'ImageId': 'ami-0d401669f33b3133b', # image138
    'ImageId': 'ami-057e358a969e4466f', # image138-2
    'ImageId': 'ami-0d56611152b87c7d6', # image138-3
    'ImageId': 'ami-0622f88fb7c214d80', # image138-4
    'ImageId': 'ami-0777f562d7b1f2a77', # image138-6
    #'InstanceType': 't2.micro', # vcpu 1 mem 1
    'InstanceType': 't3.medium', # vcpu 2 mem 4
    'InstanceType': 'c6a.2xlarge', # vcpu 8 mem 16
    #'InstanceType': 'm5a.large', # vcpu 2 mem 8
    #'InstanceType': 't3a.large', # vcpu 2 mem 8
    'BlockDeviceMappings': [
        {
            'DeviceName': '/dev/xvda',
            'Ebs': {
                'DeleteOnTermination': True,
                'VolumeSize': 50,
                'VolumeType': 'gp3',
            }
        }
    ],
    'SecurityGroupIds': ['sg-030c17447880246f9'], # all
    'SubnetId': 'subnet-98bb2bfd', # us-east-1a
    'SubnetId': 'subnet-4e17fc63', # us-east-1b
    #'SubnetId': 'subnet-0c805a45', # us-east-1c

    #'SecurityGroupIds': [''], # all
    #'SubnetId': 'subnet-090e2e19e8eea932e', # privatesubnetA
    #'Placement': {
    #    'GroupName': 'hpc',
    #},
    'KeyName': 'ec2-wscad21',
    'InstanceInitiatedShutdownBehavior': 'terminate', # stop|terminate
    'UserData': userdata,
    'MaxCount': instancesSize,
    'MinCount': instancesSize,
    'TagSpecifications': [
        {
            'ResourceType': 'instance',
            'Tags': [
                {'Key': 'Name', 'Value': instanceName},
            ]
        }
    ]
}

dummy = open("aws-dummy").read() == "1"

fileis = "./instancesspawned"
instancesspawned = json.loads(open(fileis).read())
if instancesSize < 0:
    toremove = instancesspawned[instancesSize:]
    remain = instancesspawned[:instancesSize]
    print("instancesspawned", instancesspawned)
    print("toremove", toremove)
    print("remain", remain)
    for s in set(toremove):
        if not s in remain:
            print("[aws] to remove", s)
            if dummy: continue
            os.system(f"ssh {s} shutdown -h now")
            os.system(f"sed -i '/{s}/d' /etc/hosts")
            myhosts = open("/etc/hosts").read()
            for line in open("/etc/hosts"):
                if "172" in line and not "controller" in line and not "127.0.0.1" in line:
                    hostip, hostname, *other = line.split(' ') 
                    os.system(f"""
                    ssh {hostip} "echo -e '{myhosts}' > /etc/hosts"
                    """)

    sidx = remain[-1].replace("worker", "")
    qt = remain.count(remain[-1])
    with open(indexpath, "w") as f:
        f.write(f"{sidx}-{qt}")
    with open(fileis, "w") as f:
        f.write(json.dumps(remain))
    sys.exit(0)
        

spawned = []
toprovision = []
tospawn = 0
finaloutput = []
for i in range(instancesSize):
    if qt < max_per_host:
        nodelabel = f"worker{sidx:02d}"
        qt += 1
        #print(nodelabel)
        finaloutput.append(nodelabel)
        instancesspawned.append(nodelabel)
        spawned.append(nodelabel)
    else:
        sidx+=1
        nodelabel = f"worker{sidx:02d}"
        qt = 1
        tospawn += 1
        finaloutput.append(nodelabel)
        instancesspawned.append(nodelabel)
        spawned.append(nodelabel)
        toprovision.append(nodelabel)

if tospawn == 0:
    for i in spawned:
        print(i)
    with open(indexpath, "w") as f:
        f.write(f"{sidx}-{qt}")
    with open(fileis, "w") as f:
        f.write(json.dumps(instancesspawned))
    sys.exit(0)

args['MaxCount'] = tospawn
args['MinCount'] = tospawn


if dummy:
    rs = {
        "Instances": [i for i in range(tospawn)]
    }
else:
    rs = c.run_instances(**args)
    time.sleep(8)

#instanceid = finder_dict(rs, "Instances.[0].InstanceId")
# NetworkInterfaces [{
#'PrivateDnsName': 'ip-172-31-55-111.ec2.internal',
#'PrivateIpAddress': '172.31.55.111',
controller = ""
nodelabel = ""
for idx,doc in enumerate(rs["Instances"]):
    nodelabel = toprovision[idx]
    if dummy:
        continue
    #print(nodelabel)
    #if nodelabel == "node00": nodelabel = "controller"
    instanceid = doc["InstanceId"]
    instance = ec2.Instance(instanceid)
    instance.create_tags(Tags=[{
        "Key": "Name",
        "Value": prefixname+nodelabel,
    }])
    ipaddress = instance.public_ip_address
    priv_ipaddress = instance.private_ip_address
    if nodelabel == "controller":
        controller = f"{instance} {ipaddress} {nodelabel}"
    #print(instance, ipaddress, nodelabel)
    #print(controller)
    #172.31.49.162 worker01
    with open("/etc/hosts", "a+") as f:
        f.write(f"{priv_ipaddress} {nodelabel}\n")
    for i in range(10):
        instance = ec2.Instance(instanceid)
        #print(instance.state) #{'Code': 16, 'Name': 'running'} pending
        if instance.state['Name'] == 'running':
            os.system(f"ssh {ipaddress} \"cd /root/mcmpi; git pull\" > /dev/null 2>&1")
            #hosts = open("/etc/hosts").read()
            #cmd = f"""
            #ssh {ipaddress} "echo -e '{hosts}' >> /etc/hosts > /dev/null 2>&1"
            #"""
            #print("run command", cmd)
            os.system(f"bash -c \"until ssh {ipaddress} hostname > /dev/null 2>&1; do sleep 3; done\"")
            time.sleep(3)
            #os.system(cmd)
            os.system(f"ssh {ipaddress} \"hostnamectl set-hostname {nodelabel} --static\"")
            pwd=os.getcwd()
            pwddir="/".join(pwd.split("/")[:-1])
            os.system(f"rsync -avz --progress {pwd} {priv_ipaddress}:{pwddir} > /dev/null 2>&1")
            break
        time.sleep(5)
if not dummy:
    myhosts = open("/etc/hosts").read()
    for line in open("/etc/hosts"):
        if "172" in line and not "controller" in line and not "127.0.0.1" in line:
            hostip, hostname, *other = line.split(' ') 
            os.system(f"""
            ssh {hostip} "echo -e '{myhosts}' > /etc/hosts"
            """)
with open(indexpath, "w") as f:
    f.write(f"{sidx}-{qt}")
with open(fileis, "w") as f:
    f.write(json.dumps(instancesspawned))
for n in finaloutput:
    print(n)
