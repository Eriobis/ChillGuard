#!/usr/bin/python

import Adafruit_DHT
import MySQLdb
import httplib, urllib
from RPiSensors.max6675 import Max6675
from time import localtime, strftime
# download from http://code.google.com/p/psutil/
import psutil
import time
import serial
import json

humidity = None
temperature = None
thermo_sensor = None

def SendThingSpeak():
    params = urllib.urlencode({'field1': temperature, 'field2': humidity, 'field3': thermo_sensor.temperature, 'key': 'X0KHPVC95732ZMYV'})
    headers = {"Content-type": "application/x-www-form-urlencoded", "Accept": "text/plain"}
    conn = httplib.HTTPConnection("api.thingspeak.com:80")

    try:
        conn.request("POST", "/update", params, headers)
        response = conn.getresponse()
        data = response.read()
        conn.close()
    except:
        print "connection failed"

# sleep for 16 seconds (api limit of 15 secs)
if __name__ == "__main__":
    import spidev

    # Open database connection
    db = MySQLdb.connect("192.168.2.101","fridg","fridgpw","chillbox" )
    # prepare a cursor object using cursor() method
    cursor = db.cursor()
    # Prepare SQL query to INSERT a record into the database.
    sensor = Adafruit_DHT.DHT11
    #Connected to raspberry pi GPIO3
    pin = 3

    #Open serial port for Arduino communication  @ 115200
    arduinoComm = serial.Serial("/dev/ttyUSB0", 115200)

    while True:
        # Try to grab a sensor reading.  Use the read_retry method which will retry up
        # to 15 times to get a sensor reading (waiting 2 seconds between each retry).
        humidity, temperature = Adafruit_DHT.read_retry(sensor, pin)
        # Note that sometimes you won't get a reading and
        # the results will be null (because Linux can't
        # guarantee the timing of calls to read the sensor).
        # If this happens try again!
        thermo_sensor = Max6675(0, 0)
        Flow = 0.0
        FlowPPS = 0.0
        SystemState = 3
        #Get the info from Arduino
        try:
            #Get the next line sent by arduino ( this is blocking with a timeout of 1 sec )
            jsonData = arduinoComm.readline()
            ArduinoData = json.loads(jsonData)
            Flow = ArduinoData["Flow"]
            FlowPPS = ArduinoData["FlowPPS"]
            SystemState = ArduinoData["State"]
        except:
            print "Can't read from arduino ..."

        #SQL Query
        query = "INSERT INTO `temperature` (`time`,`temperature`,`humidity`,`thermocouple`,`Flow`,`FlowPPS`,`SystemState`) VALUES (now(),%s,%s,%s,%s,%s,%s);"
        args = (temperature,humidity,thermo_sensor.temperature,Flow,FlowPPS,SystemState)

        if humidity is not None and temperature is not None and thermo_sensor is not None:
            #print('Temp={0:0.1f}*C  Humidity={1:0.1f}%'.format(temperature, humidity))
            #print thermo_sensor.temperature
            #print query
            try:
                # Execute the SQL command
                cursor.execute(query,args)
                # Commit your changes in the database
                db.commit()
                print "OK"
            except:
                # Rollback in case there is any error
                db.rollback()
                print "Rollback - db error"
        
        #Sent to thingspeak
        #SendThingSpeak()
        
        #Sleep 30 seconds
        time.sleep(30)

