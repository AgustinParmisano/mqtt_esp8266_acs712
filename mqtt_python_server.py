#Python code LEDblink.py
import paho.mqtt.publish as publish
import paho.mqtt.client as mqtt
import time
from flask import Flask
from flask.ext.pymongo import PyMongo
import json

app = Flask(__name__)

app.config['MONGO_DBNAME'] = 'restdb'
app.config['MONGO_URI'] = 'mongodb://localhost:27017/restdb'

mongo = PyMongo(app)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("esp8266status")

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

    list = json.loads(msg.payload)

    for key,value in list.iteritems():
        print ("")
        print key, value

    add_measure(list);

def on_subscribe(client, userdata,mid, granted_qos):
    print "userdata : " +str(userdata)

def on_publish(mosq, obj, mid):
    print("mid: " + str(mid))


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_start()

def add_measure(list):
  measure = mongo.db.measures
  ip = list['ip']
  time = list['time']
  name = list['name']
  data = list['data']
  measure_id = measure.insert({'ip': ip, 'time': time, 'name': name, 'data': data})
  new_measure = measure.find_one({'_id': measure_id })
  output = {'name' : new_measure['name'], 'data' : new_measure['data']}
  return jsonify({'result' : output})

@app.route("/")
def hello():
    return "Hello World!"

@app.route("/data")
def get_all_measures():
    print("Sending 2...")
    publish.single("ledStatus", "2", hostname="localhost")

    measure = mongo.db.mueasures
    output = []
    for m in measure.find():
        output.append({'name' : m['name'], 'data' : m['data']})
    return jsonify({'result' : output})

@app.route("/off")
def relay_ff():
   print("Sending 1...")
   publish.single("ledStatus", "1", hostname="localhost")
   return "Relay Off!"

@app.route("/on")
def relay_on():
    print("Sending 0...")
    publish.single("ledStatus", "0", hostname="localhost")
    return "Relay On!"

if __name__ == "__main__":
    app.run()
