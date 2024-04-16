#!/bin/bash

# Récupère l'adresse IP de l'interface par défaut
ip_address=$(hostname -I)

get_port_from_file() {
    if [ -f "port.txt" ]; then
        port=$(<port.txt)
        echo "$port"
    else
        echo "File port.txt does not exist."
        return 1
    fi
}

port_value=$(get_port_from_file)

echo ""
echo "Port: $port_value"
echo "IP: $ip_address"
echo ""

gcc -o client client.c

./client $ip_address $port_value