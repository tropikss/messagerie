#!/usr/bin/bash

# Vérifier si le fichier port.txt existe
if [ ! -f "port.txt" ]; then
    echo "1234" > port.txt  # Si non, créer le fichier avec la valeur 1234
else
    # Si oui, récupérer la valeur du fichier, l'incrémenter de 1 et la réécrire dans le fichier
    current_port=$(<port.txt)
    new_port=$((current_port + 1))
    echo "$new_port" > port.txt
fi

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
ip_address=$(hostname -I)

if [ $? -eq 0 ]; then
    echo ""
    echo "Port: $port_value"
    echo "IP: $ip_address"
    echo ""

    gcc -o serveur serveur.c
    ./serveur $port_value
else
    echo "Error: Unable to get port value."
fi
