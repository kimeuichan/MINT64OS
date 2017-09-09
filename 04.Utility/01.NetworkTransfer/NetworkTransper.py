from socket import *
import sys, os

HOST = '127.0.0.1'
PORT = 4444
ADDR = (HOST, PORT)
BUFSIZE = 16
TARGET = 'asciiart.txt'

so = socket(AF_INET, SOCK_STREAM)

try:
	so.connect(ADDR)
except Exception as e:
	print("connection fail")
	sys.exit()

print("connection success")

tempsize = 0
totalsize = 0
filesize = 0

fp = open(TARGET, 'rb')

stat = os.stat(TARGET)
filesize = stat.st_size

print('file size' + str(filesize))
so.send(str(filesize).encode())

while(totalsize <= filesize):
	if filesize - totalsize == 0:
		break
	result = so.recv(BUFSIZE)
	readsize = min([BUFSIZE, filesize - totalsize])
	data = fp.read(readsize)
	print(data)
	so.send(data)
	totalsize += readsize

so.close()
fp.close()

print("file send succcess")


