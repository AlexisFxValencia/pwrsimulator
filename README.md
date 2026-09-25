# Simulateur de Réacteur Nucléaire à Eau Pressurisée (REP / PWR)

Prototype complet comprenant un cœur de calcul en C++ (point kinetics + bilans thermiques), un serveur HTTP REST (`cpp-httplib` + `nlohmann/json`), et une IHM web moderne (Tailwind CSS + Chart.js).

## Structure du Projet

```text
pwrsimulator/
├── CMakeLists.txt          # Configuration CMake (téléchargement auto des dépendances)
├── include/
│   └── reactor.hpp         # Définition du modèle physique du REP
├── src/
│   ├── main.cpp            # Serveur web HTTP REST et boucle de simulation
│   └── reactor.cpp         # Implémentation de la cinétique des neutrons & thermique
└── public/
    └── index.html          # Interface utilisateur web interactive & graphiques temps réel
```

## Compilation

Prérequis : Un compilateur C++ supportant C++17, CMake (>= 3.14) et Git.

```bash
mkdir build
cd build
cmake ..
make
```

## Exécution

Lancez le binaire compilé depuis la racine du projet (pour que le serveur trouve `public/index.html`) :

```bash
./build/pwr_server
```

Accédez ensuite à l'IHM web dans votre navigateur :
`http://localhost:8080`

## Fonctionnalités de l'IHM
- **Graphiques Temps Réel** : Suivi de la puissance neutronique et des températures combustible/réfrigérant.
- **Barres de Contrôle** : Ajustement de la position des barres et injection de réactivité (pcm).
- **SCRAM** : Bouton d'arrêt d'urgence insérant immédiatement un anti-réactivité maximale.
- **Réinitialisation** : Remise à l'état stationnaire nominal (100% de puissance).
