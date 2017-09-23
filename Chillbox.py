#!/usr/bin/python

import spidev
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
import sys
import signal
#Import our local classes
from modules.pid import PID
from modules.outputs import outputs

#defines 
PID_SAMPLE_RATE     =   2
P                   =   40
I                   =   6.00
D                   =   3.00

thermo_sensor = None

#New outputs object
out = outputs()

def signal_handler(signal, frame):
        print('\nStopping Cooler PWM')
        out.stopCooler()
        print('You pressed Ctrl+C!')
        sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

if __name__ == "__main__":

    # Open database connection
    db = MySQLdb.connect("192.168.2.101","fridg","fridgpw","chillbox" )
    # prepare a cursor object using cursor() method
    cursor = db.cursor()
    # Prepare SQL query to INSERT a record into the database.
    sensor = Adafruit_DHT.DHT11
    #Connected to raspberry pi GPIO3
    pin = 3

    #New PID object
    pid = PID(P, I, D)
    pid.clear()
    pid.setSampleTime(PID_SAMPLE_RATE)

    out.initCooler()

    #Open serial port for Arduino communication  @ 115200
    arduinoComm = serial.Serial("/dev/ttyUSB0", 115200, rtscts=0, timeout=5)

    #Timers
    DataUpdateTimer = 0
    PIDSampleRateTimer = 0
    
    thermo_sensor = Max6675(0, 0)
    while True:

        Flow = 0.0
        FlowPPS = 0.0
        SystemState = 3
        
        fp = open("setpoint", mode = 'r')
        setpoint = int(fp.read())
        pid.SetPoint = setpoint
        fp.close()
        #Get the info from Arduino
        try:
            #Get the next line sent by arduino ( this is blocking, timeout is 5 secs )
            jsonData = arduinoComm.readline()
            ArduinoData = json.loads(jsonData)
            Flow = ArduinoData["Flow"]
            FlowPPS = ArduinoData["FlowPPS"]
            SystemState = ArduinoData["State"]
            ValidData = True
        except:
            ValidData = False
            print "Can't read from arduino ..."

        # PID PROCESS
        if ( time.time() - PIDSampleRateTimer ) > PID_SAMPLE_RATE :
            PIDSampleRateTimer = time.time()
            #Send the feedback value
            pidTemperature = thermo_sensor.temperature
            pid.update(pidTemperature)
            #Get the output value
            pid_output = 0 - pid.output
            #Trim output
            if pid_output > 100 :
                pid_output = 100
            elif pid_output < 0 :
                pid_output = 0

            out.setCooler(pid_output)
	    
        #Debug string
        print "SP: " + str(pid.SetPoint) + " Temp: " + str(thermo_sensor.temperature) + " PID Out: " + str(pid.output) + " PWM Out: " + str(pid_output)
        
        #SQL UPDATE PROCESS
        if ValidData:
            if ( time.time() - DataUpdateTimer ) > 30 :
                #SQL Query
                query = "INSERT INTO `temperature` (`time`,`thermocouple`,`Flow`,`FlowPPS`,`SystemState`,`Setpoint`) VALUES (now(),%s,%s,%s,%s,%s);"
                args = (thermo_sensor.temperature,Flow,FlowPPS,SystemState,pid.SetPoint)

                if thermo_sensor is not None:

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
                
                #Sleep 30 seconds
                DataUpdateTimer = time.time()
