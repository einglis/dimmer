#!/usr/bin/env python3

import re
import serial
import serial.tools.list_ports

import time
from time import sleep
from threading import Event
from threading import Thread


def handle_scene( scene ):
    pass


BLACK = "\033[0;30m"
RED = "\033[0;31m"
GREEN = "\033[0;32m"
BROWN = "\033[0;33m"
BLUE = "\033[0;34m"
PURPLE = "\033[0;35m"
CYAN = "\033[0;36m"
LIGHT_GRAY = "\033[0;37m"
DARK_GRAY = "\033[1;30m"
LIGHT_RED = "\033[1;31m"
LIGHT_GREEN = "\033[1;32m"
YELLOW = "\033[1;33m"
LIGHT_BLUE = "\033[1;34m"
LIGHT_PURPLE = "\033[1;35m"
LIGHT_CYAN = "\033[1;36m"
LIGHT_WHITE = "\033[1;37m"
BOLD = "\033[1m"
FAINT = "\033[2m"
ITALIC = "\033[3m"
UNDERLINE = "\033[4m"
BLINK = "\033[5m"
NEGATIVE = "\033[7m"
CROSSED = "\033[9m"
END = "\033[0m"




def find_and_open():
    print(f"{YELLOW}looking for devices...{END}")
    rako = None
    dimmer = None

    for p in serial.tools.list_ports.comports():

        print("looking for devices on '%s'" % (p.device))

        try:
            ser = serial.Serial(p.device, 9600, timeout=1,
                bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE)
        except:
            print(f"{LIGHT_RED}failed to open '%s'{END}" % (p.device))
            continue

        id = ""
        start_time = time.monotonic();

        ser.write('!'.encode()); # provoke a response
        while time.monotonic() - start_time < 2: # very impatient:
            cmd = ser.read(1).decode() # XXXEDD: exception?!
            if cmd == '!':
                id = ser.readline().decode().rstrip()
                print("ID: '%s'" % (id))
                break # while

        if re.match("rakoshim v1.\\d", id):
            print(f"{LIGHT_CYAN}found Rako interface on '%s'{END}" % (p.device))
            rako = ser
        elif re.match("dimmer v2.\\d", id):
            print(f"{LIGHT_CYAN}found Dimmer interface on '%s'{END}" % (p.device))
            dimmer = ser
        else:
            print("unrecognised or no ID on '%s' (%s)" % (p.device, id))
            ser.close()

    return (rako, dimmer)









class Peripheral():
    comms_timeout_secs = 15 # 150% of expected interval

    def __init__(self):
        self.serial = None
        self.last_cmd = '?'
        self.last_comms = 0;


    def name_match( self, target ):
        return False
    def rx( self, cmd, now ):
        return False
    def tx( self, cmd ):
        print(f"sending cmd {cmd}")
        try:
            self.serial.write( cmd.encode() )
        except e:
            print(e)
            pass


    def __thread_fn( self, dead_event ):
        print(f"{BROWN}{self.name} loop start{END}")
        #self.loop(dead_event)
        self.last_comms = time.monotonic()

        while not dead_event.is_set():
            try:
                cmd = self.serial.read(1).decode();
            except:
                print(f"{LIGHT_RED}ERROR: read error - {self.name} interface lost?{END}")
                break # while

            now = time.monotonic()

            if self.rx(cmd, now):
                self.last_comms = now
                self.last_cmd = cmd

            if now - self.last_comms > self.comms_timeout_secs:
                print(f"ERROR: {self.name} heartbeat lost - restarting")
                break # while


        if dead_event.is_set():
            print(f"{RED}WARNING: {self.name} loop dead{END}")
        else:
            print(f"{RED}WARNING: {self.name} loop end{END}")
            dead_event.set() # ensure the others die




    def run( self, dead_event ):
        self.thread = Thread( target=self.__thread_fn, args=(dead_event,) )
        self.thread.start()
    def wait( self ):
        self.thread.join()




class Rako( Peripheral ):
    name = "Rako"
    comms_timeout_secs = 15

    def fns( self, scene, updown ):
        self.scene_fn = scene
        self.updown_fn = updown


    def name_match( self, target ):
        return re.match("rakoshim v1.\\d", target)

    def rx( self, cmd, now ):
        from web import handle_up_down, button_event

        new_or_slow = cmd != self.last_cmd or now - self.last_comms > 0.25
        continuation = cmd == self.last_cmd.lower()

        if cmd == '.':
            print(f"{self.name} heartbeat at {now}")

        elif cmd == '!':
            try:
                id = self.serial.readline().decode().rstrips()
                print(f"Unsolicited ID: '{id}'")
            except:
                print("ERROR: failed to read unsolicited ID")
                return False # consider killing

        elif cmd == '0':
            print("OFF %s" %(now), flush=True)
            if new_or_slow:
                self.scene_fn(0)

        elif cmd >= '1' and cmd <= '4':
            if new_or_slow:
                print("SCENE %s %s" % (cmd, now), flush=True)
                self.scene_fn(int(cmd))

        elif cmd == 'U':
            if new_or_slow:
                print("UP %s" % (now), flush=True)
                self.updown_fn(8)

        elif cmd == 'D':
            if new_or_slow:
                print("DOWN %s" % (now), flush=True)
                self.updown_fn(-8)

        elif cmd == 'u':
            if continuation:
                print("more up %s" % (now), flush=True)
                self.updown_fn(8)

        elif cmd == 'd':
            if continuation:
                print("more down %s" % (now), flush=True)
                self.updown_fn(-8)

        else:
            return False

        return True




class Dimmer( Peripheral ):
    name = "Dimmer"

    def name_match( self, target ):
        return re.match("dimmer v2.\\d", target)



    def rx( self, cmd, now ):
        if cmd == '.':
            print(f"{self.name} heartbeat at {now}")
        else:
            return False

        return True







rako = Rako()
dimmer = Dimmer()
peripherals = [ rako ]
peripherals = [ rako, dimmer ]


def find_and_open_alt( peripherals ):
    print(f"{YELLOW}looking for devices {[p.name for p in peripherals]}...{END}")

    for port in serial.tools.list_ports.comports():
        print("looking for devices on '%s'" % (port.device))

        try:
            ser = serial.Serial(port.device, 9600, timeout=1,
                bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE)
        except:
            print(f"{LIGHT_RED}failed to open '%s'{END}" % (port.device))
            continue

        id = ""
        start_time = time.monotonic();

        try:
            while time.monotonic() - start_time < 2: # very impatient:
                #ser.write('!'.encode());
                cmd = ser.read(1).decode() # XXXEDD: exception?!
                if cmd == '!':
                    id = ser.readline().decode().rstrip()
                    print("ID: '%s'" % (id))
                    break # while
        except:
            print(f"{LIGHT_RED}failed to read '%s'{END}" % (port.device))
            continue


        name_match = False
        for p in peripherals:
            if p.name_match( id ):
                print(f"{LIGHT_CYAN}found {p.name} interface on '%s'{END}" % (port.device))
                name_match = True
                p.serial = ser
                break

        if not name_match:
            print("unrecognised or no ID on '%s' (%s)" % (port.device, id))
            ser.close()





class PeripheralThread( Thread ):
    def run(self):
        while True:
            find_and_open_alt( peripherals )

            dead_event = Event()
            for p in peripherals:
                p.run( dead_event )
            for p in peripherals:
                p.wait( )

            for p in peripherals:
                p.serial = None
                    # release the serial handles so the device notices a new connection

            sleep(1) # rate limit



if __name__ == "__main__":
    PeripheralThread( ).start()



