#!/bin/bash

# Get the port where Arduino Micro is connected
PORT=$(arduino-cli board list | awk '/Arduino Micro/ {print $1}')

# Print the prompt
echo ""
echo -en "\033[33mLaunch serial monitor?\033[0m \033[34m(y\033[0m/\033[31mN\033[0m): "
read -t 10 answer

if [[ $answer =~ ^[Yy]$ ]]; then
    if [ -e "$PORT" ]; then
        sudo killall screen 2>/dev/null
        sleep 1
        screen "$PORT" 115200
    else
        echo "Error: $PORT not found!"
        echo "Check if device is connected."
    fi
fi