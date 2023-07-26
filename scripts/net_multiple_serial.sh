#!/bin/bash
#
#  net_multiple_serial.sh
#  
#  Author:  Martin Timms
#  Date:    21st July 2023.
#  Contributors:
#  Version: 1.0
#
#  Released into the public domain.
#  
#  License: MIT License
#
#  Description: Example script to address mutliple serial ports from one network socket 
#  connection on port 8888
#  Initially scans for /dev/ttyUSB___ devices and creates a list to send to
#  Current example is single direction without response, conformation or error responses. 
#
# Example usage with multiple ADF4351 boards connected via USB serial adapters:
#
# 99 = request a scan
# 00e = send enable to first device
# 01f100000000 = set freq=100Mhz on second serial port device 
#
# Requires: 
# sudo apt-get install socat
# chmod +x net_multiple_serial.sh
#
# Example test from a Linux machine to a rpi running on ip 192.168.1.100:
#
# nc 192.168.1.100 8888
# or
# telnet 192.168.1.100 8888


# Define the TCP port to listen on
TCP_PORT=8888

# Define the baud rate for the serial ports
BAUD_RATE=115200

# Create an array to store available serial ports
declare -a serial_ports=()

# Function to scan for available serial ports
function scanSerialPorts() {
  serial_ports=()
  for port in /dev/ttyUSB*; do
    if [[ -c $port ]]; then
      serial_ports+=("$port")
    fi
  done

  # Print debug message with the ports found
  if [ "${#serial_ports[@]}" -gt 0 ]; then
    echo "Available Serial Ports:"
    for ((i = 0; i < ${#serial_ports[@]}; i++)); do
      address=$(printf "%02d" $i) # Format address as two digits [00]
      echo "[$address] ${serial_ports[$i]}"
    done
  else
    echo "No available serial ports found."
  fi
}

# Function to forward data to the selected serial port
function forwardToSerialPort() {
  # Get the index from netcat input
  index=$1

  # Check if the index is valid
  if ((index >= 0 && index < ${#serial_ports[@]})); then
    serial_port="${serial_ports[$index]}"
    # Print debug message
    echo "Sending to Serial Port: $serial_port"
    echo "Sending Line: $2"

    # Forward the text to the selected serial port (non-blocking write using socat)
    echo "$2" | socat -t 0 -T 0 -u - "$serial_port",b$BAUD_RATE,raw
  else
    echo "Invalid serial port index: $index" >&2
  fi
}

# Function to disable all devices on connected serial ports
function sendExitCommand() {
  for port in "${serial_ports[@]}"; do
    echo "Sending disable to $port"
    echo "a0\n" | socat -t 0 -T 0 -u - "$port",b$BAUD_RATE,raw
    echo "e\n" | socat -t 0 -T 0 -u - "$port",b$BAUD_RATE,raw
    echo "d\n" | socat -t 0 -T 0 -u - "$port",b$BAUD_RATE,raw
  done
  exit
}


# Print the startup message with the listening port and IP address
echo "Listening on port $TCP_PORT on IP address $(hostname -I | cut -d' ' -f1)."

# Initial scan for available serial ports at startup
scanSerialPorts

# Set up exit handler
trap sendExitCommand SIGINT

# Redirect standard error to standard output and keep running in case of errors
exec 3>&1
exec 2>&1

# Loop indefinitely
while true; do
  # Listen on TCP port and read lines of text
  nc -l -p $TCP_PORT | while IFS= read -r line; do
    # Get the address from the first two characters of the line
    address=${line:0:2}

    # Remove leading zeros from the address (if any)
    address=$((10#$address))

    # Get the remaining text after the address
    text=${line:2}

    # Check if the address is 99 to trigger a new search for ports
    if [ "$address" -eq 99 ]; then
      scanSerialPorts
    else
      # Check if the address is valid (numeric)
      if [[ $address =~ ^[0-9]+$ ]]; then
        # Print debug message
        echo "Received Address: $address"
        echo "Received Line: $text"

        # Forward the text to the appropriate serial port
        forwardToSerialPort "$address" "$text"
      else
        echo "Invalid address: $address" >&2
      fi
    fi
  done

  # If the network socket is lost, wait for a new connection
  sleep 1
done

