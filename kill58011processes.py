import os
import subprocess


process = subprocess.Popen(["netstat", "-tulpn"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

output = process.communicate()[0].split(b"\n")

inter = []
for ln in output:
    if b"58011" in ln:
        inter.append(ln)
        

inter2 = []
for ln in inter:
    splitted = list(filter(lambda x: x != b'', ln.split(b" ")))[-1].split(b"/")[0]
    os.system(b"kill -9 " + splitted)
