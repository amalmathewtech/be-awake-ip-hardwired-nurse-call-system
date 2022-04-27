from kivy.config import Config
Config.set("graphics", "show_cursor", 1)

from kivy.lang import Builder
from kivymd.app import MDApp
from kivy.clock import Clock
from kivy.uix.dropdown import DropDown

from kivy.uix.screenmanager import ScreenManager, Screen, WipeTransition, FadeTransition, NoTransition
from kivy.uix.gridlayout import GridLayout
from kivy.uix.floatlayout import FloatLayout


from kivy.uix.button import Button
from kivy.uix.label import Label
from kivy.uix.widget import Widget
from kivy.graphics import Color, Ellipse, Rectangle, RoundedRectangle

from kivy.uix.image import Image
from kivy.properties import StringProperty
from kivy.properties import ObjectProperty
from kivy.properties import ListProperty
from kivy.properties import BooleanProperty

from kivy.core.window import Window
Window.clearcolor = (1, 1, 1, 1)
Window.maximize()


import threading
port_thread_lock = threading.Lock()
import socket
from threading import Thread
import time
from time import sleep
import sys

hostname = socket.gethostname()
ip_address = socket.gethostbyname(hostname)
print(ip_address)
UDP_IP = ip_address
UDP_PORT = 49153

exit = False


Builder.load_file('main.kv')


class MainScreen(Screen, FloatLayout):

    def __init__(self, **kwargs):
        super(MainScreen, self).__init__(**kwargs)
        self.orientation = "vertical"
        global exit
        portNum = 49153
        global udpRxThreadHandle
        udpRxThreadHandle = threading.Thread(target=self.rxThread)
        udpRxThreadHandle.start()
        sleep(.1)

    def rxThread(self):
        # Generate a UDP socket
        rxSocket = socket.socket(socket.AF_INET,  # Internet
                                 socket.SOCK_DGRAM)  # UDP

        # Bind to any available address on port *portNum*
        rxSocket.bind((UDP_IP, UDP_PORT))

        while not exit:
            print(exit)
            try:
                # Attempt to receive up to 1024 bytes of data
                data, addr = rxSocket.recvfrom(1024)
                
                print("received message: {}".format(data))
                string = " "
                length = len(data)
                #print(chr(data[3]))
           
                if(chr(data[3]) == '0'):
                    room_number = 'room_'+chr(data[1])
                    getattr(self.ids, room_number).default_color = (0.92,0.125,0.164,1)
                else:
                    room_number = 'room_'+chr(data[1])
                    getattr(self.ids, room_number).default_color =  (.058,.34,.83,1)
                    
                 
            except socket.error:
                # If no data is received, you get here, but it's not an error
                # Ignore and continue
                pass
            sleep(.1)
            
           
    
        
    def on_enter(self):
        if ip_address == '127.0.0.1':
            self.ids.ULS_lbl_conn_sts.text = 'Network status:'+"[b][color=#f04713] Not connected [/b][/color]"
        else:
            self.ids.ULS_lbl_conn_sts.text = 'Network status:'+"[b][color=#03962d] Connected,[/b][/color]"+" IP Address:" + "[b][color=#223ce6]"+ip_address+"[/b][/color]"
         
class NurseCallSystem(MDApp):
    icon = 'icon.png'
    title = 'Be Awake! Nurse Call System '
    def build(self):
        sm = ScreenManager(transition=NoTransition())
        sm.add_widget(MainScreen(name='tanks_screen'))
        return sm
        
    def on_stop(self):
        print("Closing Window")
        Window.close()
        # To Do : on existing from app, udp reception thread must be killed.
        
NurseCallSystem().run()


