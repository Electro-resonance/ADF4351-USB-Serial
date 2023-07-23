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


import socket
import configparser
import time

def read_config():
    config = configparser.ConfigParser()
    config.read('sig_gen_config.ini')
    ip_address = config['Server']['IP']
    port = int(config['Server']['Port'])
    frequencies = config['Frequencies']['Frequencies'].replace(' ', '').split(',')
    interval = int(config['SignalGeneratorSetup']['Interval'])  # Get the interval in seconds
    init_commands = config['SignalGeneratorSetup']['InitCmds'].replace('"', '')
    extra_commands = {}
    for key, value in config['SignalGeneratorSetup'].items():
        if key.startswith('cmd'):
            extra_commands[key] = value.replace('"', '')
    return ip_address, port, frequencies, interval, init_commands, extra_commands


def send_command(sock, command):
    sock.sendall(command.encode())
    print("sending command:",command.replace('\n',''))

def send_init(sock,frequencies,init_commands,extra_commands):
    # Send the initial commands for each signal generator
    print("*** Sending sig gen initialiser ***")
    for index in range(len(frequencies)):
        lines = init_commands.split(",")
        for line in lines:
            line_cmd = f"{index:02}{line}\n"
            send_command(sock, line_cmd)
    print("*** Sending extra commands ***")
    for key, value in extra_commands.items():
        index = int(key[3:])  # Extract the address from the key (cmd00, cmd01, ...)
        lines = value.split(",")
        for line in lines:
            line_cmd = f"{index:02}{line}\n"
            send_command(sock, line_cmd)
    print("*** Completed ***")

def main():
    ip_address, port, frequencies, interval, init_commands, extra_commands = read_config()
    init_sent_time = 0


    while True:
        # Create a TCP/IP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            # Connect to the server
            sock.connect((ip_address, port))
            print(f"Connected to {ip_address}:{port}")
            while(True):
                current_time = time.time()
                elapsed_time = current_time - init_sent_time
                if elapsed_time > interval:
                    #Resend the signal generator initialiser
                    send_init(sock,frequencies,init_commands,extra_commands)
                    init_sent_time = time.time()

                for index in range(len(frequencies)):    
                    # Set the frequency for each signal generator
                    frequency_cmd = f"{index:02}f{frequencies[index]}\n"
                    send_command(sock, frequency_cmd)

                time.sleep(1)

        except Exception as e:
            print(f"Error: {e}")
            print("Reconnecting after a delay...")
            time.sleep(2) 
        finally:
            # Close the socket before reconnecting or terminating the loop.
            sock.close()
            print("Socket closed.")
            time.sleep(2) 

if __name__ == "__main__":
    main()
