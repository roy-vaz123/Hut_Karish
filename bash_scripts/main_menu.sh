#!/bin/bash

KERNEL_MODULE="packet_monitor.ko"
DAEMON_EXEC="./portmon_daemon"
APP_EXEC="./packetHunter"

DAEMON_PID=""
APP_PID=""

function load_kernel() {
    echo "Loading kernel module..."
    sudo insmod $KERNEL_MODULE
}

function unload_kernel() {
    echo "Unloading kernel module..."
    sudo rmmod ${KERNEL_MODULE%.ko}
}

function start_daemon() {
    echo "Starting daemon..."
    $DAEMON_EXEC &
    DAEMON_PID=$!
    echo "Daemon started with PID $DAEMON_PID"
}

function stop_daemon() {
    echo "Stopping daemon..."
    if [[ -n "$DAEMON_PID" ]]; then
        kill -SIGTERM $DAEMON_PID
        wait $DAEMON_PID
        echo "Daemon stopped."
    else
        echo "No daemon PID saved. Looking up..."
        pkill -SIGTERM -f $DAEMON_EXEC
    fi
}

function start_packetHunter() {
    echo "Starting packetHunter in a new terminal..."
    gnome-terminal -- bash -c "$APP_EXEC; exec bash" &
    # You could also use xterm or another terminal
}

function stop_packetHunter() {
    echo "Stopping packetHunter..."
    pkill -SIGINT -f $APP_EXEC
}

function show_menu() {
    clear
    echo "========= Packet Monitor Menu ========="
    echo "1. Load Kernel Module"
    echo "2. Unload Kernel Module"
    echo "3. Start Daemon"
    echo "4. Stop Daemon"
    echo "5. Start PacketHunter (client)"
    echo "6. Stop PacketHunter"
    echo "7. Exit"
    echo "======================================="
}

while true; do
    show_menu
    read -p "Select an option: " choice
    case $choice in
        1) load_kernel ;;
        2) unload_kernel ;;
        3) start_daemon ;;
        4) stop_daemon ;;
        5) start_packetHunter ;;
        6) stop_packetHunter ;;
        7) echo "Exiting..."; exit 0 ;;
        *) echo "Invalid option"; sleep 1 ;;
    esac
done
