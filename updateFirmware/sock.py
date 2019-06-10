import requests
import urllib.parse
import binascii
import paho.mqtt.client as mqtt #import the client1
import time
import json
sendTopic = ""
recDataBuffer = bytearray()
s = requests.Session()


def connect(anAddress,topicSend,topicReceive):
    global sendTopic
    print("connecting to broker")
    client.connect(anAddress)
    client.loop_start()
    client.subscribe(topicReceive)
    sendTopic = topicSend


def send(text):
    s = text
    if isinstance(text,bytearray):
        s = s.hex()
    if isinstance(text,bytes):
        s = s.hex()
    textarr = [s[i:i + 32] for i in range(0, len(s), 32)]
    for i in textarr:
        print(i)
        client.publish(sendTopic,i)
        time.sleep(0.1)

def recv(amount):
    global recDataBuffer
    while(len(recDataBuffer)<amount):
        time.sleep(0.2)
    string =  recDataBuffer[0:amount]
    recDataBuffer = recDataBuffer[amount:]
    print(string)
    return string

def close():
    s.close()

def clear():
    recDataBuffer.clear()


def on_message(client, userdata, message):
    global recDataBuffer
    data = str(message.payload.decode("utf-8"))
#    print("message received " ,data)
#    print("message topic=",message.topic)
#    print("message qos=",message.qos)
#    print("message retain flag=",message.retain)
    response = json.loads(data)
    if "SerialReceived" in response:
        print(response["SerialReceived"])
        recDataBuffer += bytearray.fromhex(response["SerialReceived"])
    print(len(recDataBuffer))
def on_log(client, userdata, level, buf):
    print("log: ",buf)


client = mqtt.Client("P1") #create new instance
client.on_message=on_message #attach function to callback
client.on_log=on_log