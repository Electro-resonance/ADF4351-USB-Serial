#!/usr/bin/python3
#
#  sig_gen_sender.py
#  
#  Author:  Martin Timms
#  Date:    23rd July 2023.
#  Contributors:
#  Version: 1.0
#
#  Released into the public domain.
#  
#  License: MIT License
#
#  Description: Example script to run on a client PC to send frequency commands to up to 
#  16 signal generators via the net_multiple_serial.sh running as a server on a host machine
#  such as a RPI connected to multiple signal generators. 
#  This script sends to the host and port defined in config.ini via tcp/ip network socket.
#  Peridically it sends an initalising sequence to the signal generators.
#
#  Note that the remote end forwards on frequency commands to the signal generators it 
#  has detected and automatically igniores commands to signal generators not yet connected.

#!/usr/bin/python3

import socket
import configparser
import time

class SignalGeneratorNetworkSender:
    def __init__(self):
        self.ip_address = ""
        self.port = 0
        self.frequencies = []
        self.phases = []
        self.amplitudes = []
        self.prev_frequencies = []
        self.prev_phases = []
        self.prev_amplitudes = []
        self.interval = 0
        self.init_commands = ""
        self.extra_commands = {}
        self.sock = None
        self.init_sent_time = 0
        self.read_config()

    def read_config(self):
        config = configparser.ConfigParser()
        config.read('sig_gen_config.ini')
        self.ip_address = config['Server']['IP']
        self.port = int(config['Server']['Port'])
        self.frequencies = config['Signals']['Frequencies'].replace(' ', '').split(',')
        self.phases = config['Signals']['Phases'].replace(' ', '').split(',')
        self.amplitudes = config['Signals']['Amplitudes'].replace(' ', '').split(',')
        self.interval = int(config['SignalGeneratorSetup']['Interval'])  # Get the interval in seconds
        self.init_commands = config['SignalGeneratorSetup']['InitCmds'].replace('"', '')
        for key, value in config['SignalGeneratorSetup'].items():
            if key.startswith('cmd'):
                self.extra_commands[key] = value.replace('"', '')
        for index in range(len(self.frequencies)):
            self.prev_frequencies.append(-1)
            self.prev_phases.append(-1)
            self.prev_amplitudes.append(-1)

    def send_command(self, command):
        self.sock.sendall(command.encode())
        print("sending command:", command.replace('\n', ''))

    def send_init(self):
        # Send the initial commands for each signal generator
        print("*** Sending signal generator initialiser ***")
        for index in range(len(self.frequencies)):
            lines = self.init_commands.split(",")
            for line in lines:
                line_cmd = f"{index:02}{line}\n"
                self.send_command(line_cmd)

    def send_extra_commands(self):
        # Send the extra commands for each signal generator
        print("*** Sending extra commands ***")
        for key, value in self.extra_commands.items():
            index = int(key[3:])  # Extract the address from the key (cmd00, cmd01, ...)
            lines = value.split(",")
            for line in lines:
                line_cmd = f"{index:02}{line}\n"
                self.send_command(line_cmd)

    def send_freq_phase_amplitudes(self,set_all=True):
        for index in range(len(self.frequencies)):
            # Set the frequency for each signal generator
            if(set_all or self.frequencies[index]!=self.prev_frequencies[index]):
                frequency_cmd = f"{index:02}f{self.frequencies[index]}\n"
                self.send_command(frequency_cmd)
                self.prev_frequencies[index]=self.frequencies[index]
            if(set_all or self.phases[index]!=self.prev_phases[index]):
                phase_cmd = f"{index:02}p{self.phases[index]}\n"
                self.send_command(phase_cmd)
                self.prev_phases[index]=self.phases[index]
            if(set_all or self.amplitudes[index]!=self.prev_amplitudes[index]):         
                amplitude_cmd = f"{index:02}a{self.amplitudes[index]}\n"
                self.send_command(amplitude_cmd)
                self.prev_amplitudes[index]=self.amplitudes[index]

    def set_signal(self,channel,freq,phase,amplitude):
        self.frequencies[channel]=freq
        self.phases[channel]=phase
        self.amplitudes[channel]=amplitude

    def connect(self):
        if(self.sock==None):
            # Create a TCP/IP socket
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Connect to the server
            self.sock.connect((self.ip_address, self.port))
            print(f"Connected to {self.ip_address}:{self.port}")

    def run(self,set_all=True):
        try:
            self.connect()
            current_time = time.time()
            elapsed_time = current_time - self.init_sent_time
            if elapsed_time > self.interval:
                # Send the initial commands and extra commands
                self.send_init()
                self.send_extra_commands()
                print("*** Initialised ***")
                self.init_sent_time = current_time
            self.send_freq_phase_amplitudes(set_all)
        except Exception as e:
            print(f"Error: {e}")
            self.sock.close()
            self.sock=None
            return False
        return True;


if __name__ == "__main__":
    sender = SignalGeneratorNetworkSender()
    while(True):
        if(sender.run(set_all=True)==False):
            print("Reconnecting after a delay...")
            time.sleep(2)
        time.sleep(1)
