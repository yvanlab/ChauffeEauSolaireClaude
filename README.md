# Chauffage Eau Solaire Spa

Système de contrôle automatique de chauffage solaire pour spa basé sur ESP32 avec gestion de configuration persistante et interface web complète.

## Caractéristiques

- **Capteurs de température**: 3 x DS18B20 sur GPIO4
  - Capteur 1: Température de l'air
  - Capteur 2: Température de l'eau du spa
  - Capteur 3: Température de l'eau des panneaux solaires

- **Contrôle de pompe**: Relais sur GPIO14

- **Interface Web**: Configuration et surveillance via navigateur
  - Interface HTML stockée sur **LittleFS** (modifiable sans recompilation)
  - Affichage temps réel des températures
  - Contrôle manuel de la pompe
  - Configuration des paramètres
  - Modification des identifiants WiFi
  - Réinitialisation de la configuration

- **Gestion de configuration persistante**:
  - **Configuration JSON** dans LittleFS (human-readable)
  - Deux fichiers: `temp_config.json` et `wifi_config.json`
  - Sauvegarde automatique lors des modifications
  - Restauration au démarrage
  - Configuration WiFi modifiable via web
  - État de la pompe conservé en cas de coupure
  - Facile à sauvegarder et restaurer

- **Accès réseau amélioré**:
  - **mDNS** activé: accès par nom au lieu d'IP
  - Par défaut: `http://chauffeSpa.local`
  - Hostname personnalisable
  - Fonctionne sur tous les systèmes modernes
  
- **Architecture modulaire**:
  - Code séparé en modules (config, webserver, main, logger)
  - Interface web dans fichier HTML séparé
  - Plus de 1300 lignes de code organisé

- **Système de journalisation**:
  - Logs centralisés dans serial monitor et interface web
  - 4 niveaux: INFO, WARNING, ERROR, SUCCESS
  - Derniers 100 événements affichés sur la page web
  - Auto-refresh toutes les 3 secondes
  - Code couleur par niveau de gravité

## Architecture du projet

```
ChauffeEauSolaireClaude/
├── platformio.ini          # Configuration PlatformIO
├── data/                   # Fichiers du système de fichiers (LittleFS)
│   ├── index.html         # Interface web
│   ├── temp_config.json   # ⭐ Configuration températures (JSON)
│   └── wifi_config.json   # ⭐ Configuration WiFi (JSON)
├── include/
│   ├── config.h           # Gestion configuration JSON
│   ├── webserver.h        # Serveur web
│   └── logger.h           # ⭐ Système de journalisation
├── src/
│   ├── main.cpp           # Programme principal + mDNS
│   ├── config.cpp         # Lecteur/écriture JSON
│   ├── webserver.cpp      # Implémentation serveur web
│   └── logger.cpp         # ⭐ Implémentation journalisation
├── README.md              # Cette documentation
├── QUICKSTART.md          # Guide démarrage rapide (5 min)
├── CONFIGURATION.md       # Guide de configuration détaillé
├── JSON_CONFIG.md         # Guide configuration JSON + mDNS
├── LOGGING.md             # ⭐ Guide système de journalisation
├── FILESYSTEM.md          # Guide LittleFS et upload HTML
├── CHANGELOG_JSON_MDNS.md # Historique JSON + mDNS
└── WIRING.txt             # Schémas de câblage détaillés
```

## Logique de contrôle

La pompe s'active automatiquement lorsque **toutes** les conditions suivantes sont remplies:

1. ✅ Température des panneaux ≥ température minimum configurée
2. ✅ Température du spa < température maximum configurée
3. ✅ (Température panneaux - Température spa) ≥ différence configurée

**Hystérésis**: Une fois la pompe activée, elle ne s'arrête que si la différence descend sous (seuil - 1°C) pour éviter les cycles marche/arrêt fréquents.

**Sécurité**: Si la température du spa atteint ou dépasse le maximum configuré, la pompe s'arrête immédiatement, même si les autres conditions sont remplies.

## Installation

### 1. Matériel requis

- ESP32 Development Board
- 3 x capteurs DS18B20 (1 standard + 2 sondes étanches)
- 1 x résistance 4.7kΩ (pull-up pour OneWire)
- 1 x module relais 5V ou 3.3V
- Câbles de connexion
- Boîtier étanche (IP65 minimum)
- Alimentation 5V pour ESP32

### 2. Câblage

Voir le fichier **WIRING.txt** pour les schémas détaillés.

**Résumé rapide:**

```
DS18B20 (tous les trois):
  VCC → 3.3V
  GND → GND
  DATA → GPIO4 (avec résistance 4.7kΩ entre DATA et VCC)

Relais:
  VCC → 5V (ou 3.3V)
  GND → GND
  IN → GPIO14
```

### 3. Première configuration

#### Configuration WiFi (avant téléversement)

**Méthode recommandée**: Éditer directement le fichier JSON

1. Ouvrir `data/wifi_config.json`:
```json
{
  "ssid": "VotreSSID",
  "password": "VotreMotDePasse",
  "hostname": "chauffeSpa"
}
```

2. Modifier les valeurs:
   - `ssid`: Nom de votre réseau WiFi
   - `password`: Mot de passe WiFi
   - `hostname`: Nom pour accéder au système (ex: `monSpa`, `chauffage`, etc.)

3. **(Optionnel)** Configurer les températures dans `data/temp_config.json`:
```json
{
  "tempDifferenceThreshold": 5.0,
  "minPanelTemp": 25.0,
  "maxSpaTemp": 40.0,
  "manualOverride": false,
  "pumpState": false
}
```

**Note**: Vous pouvez aussi modifier ces paramètres via l'interface web une fois le système démarré.

### 4. Compilation et téléversement

**⚠️ Important**: Ce projet utilise LittleFS pour stocker l'interface web. Vous devez téléverser **deux choses**:

```bash
cd C:\dev\ChauffeEauSolaireClaude

# 1. Téléverser le programme (code C++)
pio run --target upload

# 2. Téléverser le système de fichiers (interface web HTML)
pio run --target uploadfs

# 3. Moniteur série pour voir les logs
pio device monitor
```

**Note**: Le téléversement du filesystem (`uploadfs`) est nécessaire pour que l'interface web fonctionne. Voir **FILESYSTEM.md** pour plus de détails.

### 5. Identification des capteurs

Au premier démarrage, le système affiche dans le moniteur série:
```
Found 3 DS18B20 sensors

Sensor addresses detected:
  [0] Air sensor:   28 AA 12 34 56 78 90 AB
  [1] Spa sensor:   28 BB 23 45 67 89 01 BC
  [2] Panel sensor: 28 CC 34 56 78 90 12 CD
```

**Important**: Vérifiez que chaque capteur correspond à la bonne mesure:
- Index [0] doit mesurer l'air
- Index [1] doit mesurer l'eau du spa
- Index [2] doit mesurer l'eau des panneaux

Si ce n'est pas le cas, réorganisez physiquement les capteurs ou notez leurs adresses et modifiez l'attribution dans `main.cpp`.

## Utilisation

### Interface Web

1. Connectez-vous au réseau WiFi configuré

2. Accédez à l'interface de **deux façons**:
   
   **Par nom (recommandé - mDNS)**:
   ```
   http://chauffeSpa.local
   ```
   (ou le hostname que vous avez configuré dans `wifi_config.json`)
   
   **Par adresse IP**:
   ```
   http://192.168.1.xxx
   ```
   (notez l'IP affichée dans le moniteur série)

3. L'interface affiche:
   - **Températures en temps réel** (rafraîchissement automatique toutes les 2 secondes)
   - **État de la pompe** avec indicateur visuel animé
   - **Mode de fonctionnement** (Automatique ou Manuel)
   - **Journaux système** en bas de page avec tous les événements

### Onglets de configuration

#### 🌡️ Températures
- **Différence de température**: Écart minimum Panel-Spa pour activer (défaut: 5°C)
- **Température minimum panneaux**: Seuil d'activation minimum (défaut: 25°C)
- **Température maximum spa**: Limite de sécurité (défaut: 40°C)

#### 📶 WiFi
- Modifier le SSID et mot de passe
- **Important**: Redémarrer l'ESP32 après modification pour appliquer

#### ⚙️ Système
- **Réinitialiser**: Restaure tous les paramètres aux valeurs par défaut
- **Redémarrer**: Redémarre l'ESP32 (utile après changement WiFi)

### Contrôle manuel

Trois modes disponibles:

- **✅ Marche forcée**: La pompe reste allumée en permanence
- **❌ Arrêt forcé**: La pompe reste éteinte en permanence
- **🔄 Mode automatique**: Contrôle basé sur les températures et paramètres

**Note**: L'état manuel est sauvegardé et restauré après redémarrage.

## Gestion de la configuration

### Sauvegarde automatique

Le système sauvegarde automatiquement dans la mémoire flash:
- Paramètres de température (à chaque modification)
- Identifiants WiFi (à chaque modification)
- État de la pompe en mode manuel (à chaque changement)

### Restauration au démarrage

Au démarrage, le système:
1. Charge la configuration depuis la flash
2. Affiche les paramètres dans le moniteur série
3. Restaure l'état de la pompe si en mode manuel
4. Se connecte au WiFi avec les identifiants sauvegardés

### Moniteur série et journaux web

Le système affiche en continu dans le **serial monitor**:
```
Air: 22.5°C | Spa: 30.2°C | Panel: 38.7°C | Diff: 8.5°C | Pump: ON  | Mode: AUTO

[00:05:30] OK Pump ACTIVATED (Panel 38.7°C > Spa 30.2°C + 5.0°C threshold)
[00:05:45] INFO Temperature parameters updated: diff=7.0, min=30.0, max=38.0
[00:06:12] WARN WiFi credentials updated - restart required
```

Les mêmes événements sont visibles dans la section **"Journaux système"** en bas de la page web avec:
- Code couleur par niveau (bleu=info, jaune=warning, rouge=erreur, vert=succès)
- Auto-refresh toutes les 3 secondes
- Boutons: Effacer, Actualiser, Auto-scroll

## Dépannage

### Aucun capteur détecté
- ✓ Vérifier le câblage (GPIO4, 3.3V, GND)
- ✓ Vérifier la résistance pull-up 4.7kΩ entre DATA et 3.3V
- ✓ Tester les capteurs un par un
- ✓ Vérifier la polarité des capteurs

### WiFi ne se connecte pas
- ✓ Vérifier SSID et mot de passe dans la configuration
- ✓ S'assurer que le réseau est en 2.4 GHz (ESP32 ne supporte pas le 5 GHz)
- ✓ Vérifier la portée du signal WiFi
- ✓ Utiliser le moniteur série pour voir les messages d'erreur

### La pompe ne démarre pas
- ✓ Vérifier le relais (connexion GPIO14)
- ✓ Vérifier l'alimentation du relais
- ✓ Vérifier les paramètres de température dans l'interface web
- ✓ S'assurer que le mode n'est pas en "Arrêt forcé"
- ✓ Consulter le moniteur série pour les logs

### Configuration perdue après redémarrage
- ✓ Vérifier que les paramètres sont bien enregistrés (bouton "Enregistrer")
- ✓ Observer les messages "✓ Configuration saved" dans le moniteur série
- ✓ Si le problème persiste, la mémoire flash pourrait être défectueuse

### L'interface web ne répond pas
- ✓ Vérifier la connexion WiFi
- ✓ Vérifier l'adresse IP dans le moniteur série
- ✓ Essayer de redémarrer l'ESP32
- ✓ Vider le cache du navigateur

## Sécurité

- ✅ Vérification constante de la température maximum du spa
- ✅ Arrêt automatique en cas de dépassement
- ✅ Hystérésis pour éviter l'usure de la pompe
- ✅ État sauvegardé en cas de coupure de courant
- ✅ Contrôle manuel d'urgence disponible
- ⚠️ **Important**: Utiliser un relais adapté à la puissance de la pompe
- ⚠️ **Important**: Respecter les normes électriques locales
- ⚠️ **Important**: Prévoir une protection par fusible

## Développement futur

Améliorations possibles:
- [x] ~~Graphiques historiques des températures~~ ✅ **Implémenté en v2.4**
- [ ] Notifications push/email en cas d'anomalie
- [ ] Intégration domotique (MQTT, Home Assistant)
- [ ] Mode économie d'énergie avec deep sleep
- [ ] Planification horaire (activer/désactiver par plages)
- [ ] OTA (mise à jour over-the-air)
- [ ] API REST pour intégration externe
- [ ] Affichage LCD local des températures
- [ ] Alarme sonore en cas de problème capteur
- [ ] Multi-langues (FR/EN/ES)

## Support et contribution

Pour signaler un bug ou demander une fonctionnalité, créez une issue sur le dépôt du projet.

## Licence

Ce projet est fourni "tel quel" sans garantie. Utilisez-le à vos propres risques.

---

**Version**: 2.4  
**Date**: 2026-04-20  
**Auteur**: Claude + ylabrit

## Nouveautés Version 2.4

📊 **Graphique des températures 24h**
- Nouvel onglet "Graphique 24h" dans l'interface web
- Visualisation interactive de l'évolution des 3 températures
- Historique de 24 heures (1 point par minute = 1440 points max)
- Affichage en heures réelles (08:00, 12:00, 16:00...)
- Rafraîchissement automatique toutes les 30 secondes
- Stockage circulaire en mémoire (pas de persistence après redémarrage)

🧠 **Monitoring RAM en temps réel**
- Affichage de l'utilisation de la mémoire RAM dans l'onglet Système
- Barre de progression visuelle avec code couleur
- Statistiques détaillées (libre, utilisé, minimum, fragmentation)
- Taille du buffer historique affichée (23 KB)
- Mise à jour automatique toutes les 10 secondes

🖥️ **Simulateur Web amélioré**
- Double-cliquez sur `launch_simulator.bat` pour lancer le simulateur
- Historique 24h pré-généré avec cycles réalistes
- Testez l'interface complète sans matériel ESP32

Voir `CHANGELOG_GRAPH.md` et `RAM_MONITORING.md` pour les détails complets.
