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

get_file-port_from_file() {
    if [ -f "file-port.txt" ]; then
        fileport=$(<file-port.txt)
        echo "$fileport"
    else
        echo "File fileport.txt does not exist."
        return 1
    fi
}

port_value=$(get_port_from_file)
file_port_value=$(get_file-port_from_file)

echo ""
echo "Port : $port_value"
echo "File port : $file_port_value"
echo "IP : $ip_address"
echo ""

gcc -o client client.c
if [ $? -eq 0 ]; then
    echo "La compilation s'est déroulée avec succès."
    ./client $ip_address $port_value $file_port_value
else
    echo "Erreur lors de la compilation."
fi

