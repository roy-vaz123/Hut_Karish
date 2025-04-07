#!/bin/bash

KERNEL_MODULE="build/kernel_module/sniffer.ko"
DAEMON_EXEC="sudo ./build/daemon/portmon_daemon"
APP_EXEC="sudo ./build/packet_hunter/packet_hunter"

LAST_CMD=""

# Load the kernel module
function load_kernel() {
    sudo insmod "$KERNEL_MODULE"
    LAST_CMD="Loaded Kernel Module"
}

# Unload the kernel module
function unload_kernel() {
    sudo rmmod "$KERNEL_MODULE"
    LAST_CMD="Removed Kernel Module"
}

# Start the daemon in a new terminal
function start_daemon() {
    gnome-terminal -- bash -c "$DAEMON_EXEC; exec bash" &
    LAST_CMD="Started Daemon"
}

# Start the packet hunter in a new terminal
function start_packetHunter() {
    gnome-terminal -- bash -c "$APP_EXEC; exec bash" &
    LAST_CMD="Started Packet Hunter"
}

# Show kernel messages
function tail_dmesg() {
    clear
    echo "Tailing kernel messages (Ctrl+C to stop)..."
    sudo dmesg -wH
}

# Show the menu
function show_menu() {
    clear
    echo "========= Packet Monitor Menu ========="
    echo "1. Load Kernel Module"
    echo "2. Unload Kernel Module"
    echo "3. Start Daemon"
    echo "4. Start PacketHunter (client)"
    echo "5. Tail dmesg (kernel module output)"
    echo "6. Exit"
    echo "======================================="
    echo
    echo "Last Command: $LAST_CMD"
    echo
}

# Check gnome-terminal is available
if ! command -v gnome-terminal &> /dev/null; then
    echo "gnome-terminal not found. Please install it or use xterm."
    exit 1
fi

# Menu loop
while true; do
    show_menu
    read -p "Select an option: " choice
    case $choice in
        1) load_kernel ;;
        2) unload_kernel ;;
        3) start_daemon ;;
        4) start_packetHunter ;;
        5) tail_dmesg ;;
        6) echo "Exiting..."; exit 0 ;;
        *) echo "Invalid option"; sleep 1 ;;
    esac
done
