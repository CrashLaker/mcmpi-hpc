import sys
import os
import re 




def main(args):
    
    action = args[0]

    if action == "clean":
        main(["stopall"])
        main(["up", "server01"])
    elif action == "stopall":
        output = os.popen("docker-compose ps").read()
        print(output)
        for line in output.split("\n"):
            #wperf22-app_server02_1
            m = re.findall("docker-(server\d+).*", line)
            if m:
                server = m[0]
                if "Up" in line:
                    os.system(f"docker-compose stop {server}")
    elif action == "up":
        os.system(f"docker-compose up -d {args[1]}")






if __name__ == '__main__':
    if len(sys.argv) > 1:
        args = sys.argv[1:]
    else:
        args = [""]
        args = ["stopall"]
        #args = ["up", "server03"]
    print("args", args)
    main(args)

