import RPi.GPIO as GPIO

class outputs:
    """Outputs Class
    """
    def __init__(self):
        GPIO.setmode(GPIO.BOARD)
        #Init the cooler pin
    
    def initCooler(self):
        GPIO.setup(12, GPIO.OUT)
        self.coolerPin = GPIO.PWM(12, 5000)
        self.coolerPin.start(100)
        self.coolerPin.ChangeFrequency(100)  # where 0.0 <= dc <= 100.0
        
    def setCooler(self, value):
        if ( value > 100 ):
            value = 100
        elif value < 0:
            value = 0
        self.coolerPin.ChangeDutyCycle(100 - value)

    def stopCooler(self):
        self.coolerPin.stop()
        GPIO.cleanup()
