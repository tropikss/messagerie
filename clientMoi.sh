#!/bin/zsh

# Récupère l'adresse IP de l'interface réseau principale
ip_address=$(ipconfig getifaddr en0)

get_port_from_file() {
    if [ -f "port.txt" ]; then
        port=$(cat port.txt)
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
