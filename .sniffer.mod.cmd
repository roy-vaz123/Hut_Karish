cmd_/home/roy/projects/packet-sniffer/sniffer.mod := printf '%s\n'   sniffer.o | awk '!x[$$0]++ { print("/home/roy/projects/packet-sniffer/"$$0) }' > /home/roy/projects/packet-sniffer/sniffer.mod
