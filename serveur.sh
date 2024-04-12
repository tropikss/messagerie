#!/bin/bash

# Récupère l'adresse IP de l'interface par défaut
ip_address=$(hostname -I)

# Vérifie si l'adresse IP est non vide
if [ -n "$ip_address" ]; then
    # Affiche l'adresse IP à l'écran
    echo "Adresse IP: $ip_address"

    # Stocke l'adresse IP dans un fichier texte
    echo "$ip_address" > adresse_ip.txt

    echo "Adresse IP enregistrée dans le fichier adresse_ip.txt."
else
    echo "Impossible de récupérer l'adresse IP."
fi

gcc -o serveur serveur.c

./serveur