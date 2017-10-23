import paho.mqtt.client as mqtt #import the client1
import time 

broker_address="localhost" 

client = mqtt.Client("P1") #create new instance

client.connect(broker_address) #connect to broker
c=0
m=0
while True:
	c+=1
	m+=1	
	print("Publishing message %s to device %s" % (m,c))
	client.publish("device/" + str(c) + "/receive","message number: " + str(m))#publish
	if c >= 5:
		print("Topic is device/device_number(1-5)/receive")
		c=0
	time.sleep(5)
