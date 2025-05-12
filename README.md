# Ransomware-Projet
## ProManager - Ransomware Pédagogique

```
.
├── client_decrypt.c
├── ranswomware.c
├── server_pardon.c
└── TP
    └── Projet
        ├── main.c
        ├── notes.md
        └── rapport.txt
```


### 1. ransomware

Ce script est l'agent principal qui surveille le systme et declenche le chiffrement.

Fonctionnalitees principale:

   -  Surveille le dossier TP/ toutes les 5 secondes pour détecter la création du dossier Projet/
   -  Démarre un compte à rebours dès que le dossier est détecté
   -  Après le delai (30 secondes en mode test, 1 heure en production), chiffre tous les fichiers ciblés
   -  Utilise l'algorithme AES-256-CBC pour le chiffrement
   -  Génère une clé et un vecteur d'initialisation (IV) aléatoires
   -  Envoie la clé et l'IV au serveur
   -  Crée un fichier RANÇON.txt expliquant la situation à l'utilisateur

### 2. serveur_pardon

ce serveur gère les clees de chiffrement et le processus de "pardon".

Fonctionnalités principales:

   -  ecoute les connexions sur le port 4242
   -  Stocke les clés et IV envoyés par le ransomware
   -  sauvegarde les clés sur le disque pour persistance
   -  evalue les messages d'excuse des utilisateurs
   -  Renvoie la clé et l'IV si les excuses sont acceptables (minimum 20 caractères)

### 3. client_decrypt

Cet outil permet à l'utilisateur de récupérer ses fichiers après avoir présenté ses excuses.

Fonctionnalités principales:

   -  Se connecte au serveur_pardon
   -  Permet a l'utilisateur de saisir un message d'excuse
   -  Reçoit la clé et l'IV si les excuses sont accepter
   -  Déchiffre tous les fichiers .enc dans le dossier Projet/
   -  Restaure les fichiers originaux et supprime les version chiffré

Fonctionnement du système

  -   L'utilisateur crée un dossier "Projet" dans TP/
  -   Le ransomware détecte ce dossier et démarre un compte à rebours
  -   Si le délai expire, tous les fichiers sont chiffrés et un fichier RANÇON.txt est créé
  -   L'utilisateur doit exécuter client_decrypt et présenter des excuse
  -   Si les excuses sont accepter, les fichiers sont déchiffre
