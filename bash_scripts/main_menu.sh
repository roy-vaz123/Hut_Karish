#!/bin/bash

KERNEL_MODULE="build/kernel_module/sniffer.ko"
DAEMON_SERVICE="portmon_daemon"
APP_EXEC="sudo ./build/packet_hunter/packet_hunter"

LAST_CMD=""

# Load the kernel module
function load_kernel() {
    if sudo insmod "$KERNEL_MODULE"; then
        LAST_CMD="Loaded Kernel Module"
    else
        LAST_CMD="Error: Failed to load Kernel Module"
    fi
}

# Unload the kernel module
function unload_kernel() {
    if sudo rmmod "$KERNEL_MODULE"; then
        LAST_CMD="Removed Kernel Module"
    else
        LAST_CMD="Error: Failed to remove Kernel Module"
    fi
}

# Start the daemon via systemd
function start_daemon() {
    if sudo systemctl start "$DAEMON_SERVICE"; then
        LAST_CMD="Started Daemon"
    else
        LAST_CMD="Error: Failed to start Daemon"
    fi
}

# Stop the daemon via systemd
function stop_daemon() {
    if sudo systemctl stop "$DAEMON_SERVICE"; then
        LAST_CMD="Stopped Daemon"
    else
        LAST_CMD="Error: Failed to stop Daemon"
    fi
}

# Start the packet hunter in a new terminal
function start_packetHunter() {
    if gnome-terminal -- bash -c "$APP_EXEC; exec bash" &>/dev/null; then
        LAST_CMD="Started Packet Hunter"
    else
        LAST_CMD="Error: Failed to start Packet Hunter"
    fi
}

# Show daemon logs
function view_daemon_logs() {
    clear
    echo "Tailing daemon logs (Ctrl+C to stop)..."
    sudo journalctl -u "$DAEMON_SERVICE" -f
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
    echo "4. Stop Daemon"
    echo "5. Start PacketHunter (client)"
    echo "6. Tail dmesg (kernel module output)"
    echo "7. View Daemon Logs"
    echo "8. Exit"
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
        4) stop_daemon ;;
        5) start_packetHunter ;;
        6) tail_dmesg ;;
        7) view_daemon_logs ;;
        8) echo "Exiting..."; exit 0 ;;
        *) echo "Invalid option"; sleep 1 ;;
    esac
done
