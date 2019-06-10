#!/usr/bin/python

#import libraries
import time
import sock as sock
from  intelhex import IntelHex

ih = IntelHex()
# define a rfcomm bluetooth connection
sock = sock
# connect to the bluetooth device
sock.connect('192.168.1.8','/kitchen/overhead/cmnd/serialsend5','/kitchen/overhead/tele/RESULT')

# get in sync with the avr
sock.send('44494d424f4f543a0d0a')
time.sleep(0.5)
sock.clear()
for i in range(5):
    print("syncing")
    sock.send('3020') # stk_get_sync
#	sock.send('') # stk_crc_eop
    time.sleep(0.05)

# receive sync ack
print( "receiving sync ack")
insync = sock.recv(1) # stk_insync
ok = sock.recv(1) # stk_ok

# check received ack
if insync == b'14' and ok == b'10':
    print( "insync")
sock.clear()
# get the major version of the bootloader
print( "getting the major version of the bootloader")
sock.send('418120') # stk_get_parameter
#sock.send('') # stk_sw_major
#sock.send('') # sync_crc_eop
time.sleep(0.05)

# receive bootlader major version
print( "receiving bootloader major version")
insync = sock.recv(1) # stk_insync
major = sock.recv(1) # stk_sw_mjaor
ok = sock.recv(1) # stk_ok

# check received sync ack
if insync == '\x14' and ok == '\x10':
    print ("insync")

# get the minor version of the bootloader
print( "getting the minor version of the bootloader")
sock.send('418220') # stk_get_parameter
#sock.send('\x82') # stk_sw_minor
#sock.send('\x20') # sync_crc_eop
time.sleep(0.05)

# receive bootlader minor version
print( "receiving bootloader minor version")
insync = sock.recv(1) # stk_insync
minor = sock.recv(1) # stk_sw_minor
ok = sock.recv(1) # stk_ok

# check received sync ack
if insync == '\x14' and ok == '\x10':
    print ("insync")

print ("bootloader version %s.%s" % (ord(major), ord(minor)))

# enter programming mode
print ("entering programming mode")
sock.send('5020') # stk_enter_progmode
#sock.send('\x20') # sync_crc_eop
time.sleep(0.05)

# receive sync ack
print( "receiving sync ack")
insync = sock.recv(1) # stk_insync
ok = sock.recv(1) # stk_ok

# check received sync ack
if insync == '\x14' and ok == '\x10':
    print ("insync")

# get device signature
print ("getting device signature")
sock.send('7520') # stk_read_sign
#sock.send('\x20') # sync_crc_eop

# receive device signature
print ("receiving device signature")
insync = sock.recv(1) # stk_insync
signature = sock.recv(3) # device
ok = sock.recv(1) # stk_ok

# check received sync ack
if insync == '\x14' and ok == '\x10':
    print ("insync")

print(signature)
#print ("device signature %s-%s-%s" % (ord(signature[0]), ord(signature[1]), ord(signature[2])))

# start with page address 0
address = 0

# open the hex file
ih.loadhex("a.hex")
print(ih.minaddr())
print(ih.maxaddr())
chunk = 128
while True:
    # calculate page address
    laddress = int((address/2) % 256)
    haddress = int((address/2) / 256)


    # load page address
    print("loading page address")
    nums = [85,  laddress, haddress, 32]
    addrba = bytearray(nums)
    sock.send(addrba) # sync_crc_eop
    time.sleep(0.01)

    # receive sync ack
    print ("receiving sync ack")
    insync = sock.recv(1) # stk_insync
    ok = sock.recv(1) # stk_ok

    # check received sync ack
    if insync == '\x14' and ok == '\x10':
        print( "insync")

    data = ""
    # the hex in the file is represented in char
    # so we have to merge 2 chars into one byte
    # 16 bytes in a line, 16 * 8 = 128
    data = ih[address:address+chunk].tobinstr()
    size = int(len(data))
    print(data)
    print("sending program page to write", str(size), "laddress", laddress, "haddress", haddress)
    nums = [100,  0, size, 70]
    sizestr = bytearray(nums)
#	sizestr[2] = size.hex()
    sock.send(sizestr) # stk_program_page
    sock.send(data)
    sock.send(bytearray([32]))
    #sock.send('\x20') # sync_crc_eop
    #time.sleep(0.01)

    # receive sync ack
    print ("receiving sync ack")
    insync = sock.recv(1) # stk_insync
    ok = sock.recv(1) # stk_ok

    # check received sync ack
    if insync == b'14' and ok == b'10':
        print( "insync")

    # when the whole program was uploaded
    if size != chunk:
        break
    address += chunk #addressing in stk500 goes per word.

# close the hex file


# load page address
#print "loading page address"
#sock.send('\x55') # stk_load_address
#sock.send('\x00')
#sock.send('\x00')
#sock.send('\x20') # sync_crc_eop
#time.sleep(0.05)

# receive sync ack
#print "receiving sync ack"
#insync = sock.recv(1) # stk_insync
#ok = sock.recv(1) # stk_ok

# check received sync ack
#if insync == '\x14' and ok == '\x10':
#  print "insync"

# send read program page
#print "sending program page to read"
#sock.send('\x74') # stk_read_page
#sock.send('\x00') # page size
#sock.send('\x80') # page size
#sock.send('\x46') # flash memory, 'f'
#sock.send('\x20') # sync_crc_eop
#time.sleep(0.05)

#print len(sock.recv(128))

# leave programming mode
print("leaving programming mode")
sock.send('5120') # stk_leave_progmode
#sock.send('\x20') # sync_crc_eop
time.sleep(0.05)

# receive sync ack
print( "receiving sync ack")
insync = sock.recv(1) # stk_insync
ok = sock.recv(1) # stk_ok

# check received sync ack
if insync == b'14' and ok == b'10':
    print( "insync")

# close the bluetooth connection
sock.close()
