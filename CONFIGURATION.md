# Guide de Configuration - Chauffage Solaire Spa

## Table des matières
1. [Configuration initiale](#configuration-initiale)
2. [Structure de la configuration](#structure-de-la-configuration)
3. [Modification des paramètres](#modification-des-paramètres)
4. [Stockage en mémoire flash](#stockage-en-mémoire-flash)
5. [Restauration et sauvegarde](#restauration-et-sauvegarde)

---

## Configuration initiale

### Avant le premier téléversement

**Méthode 1: Modifier le fichier `include/config.h`** (Recommandé)

Ouvrez `include/config.h` et modifiez le constructeur `SpaConfig()`:

```cpp
SpaConfig() {
  tempDifferenceThreshold = 5.0;      // Votre valeur
  minPanelTemp = 25.0;                // Votre valeur
  maxSpaTemp = 40.0;                  // Votre valeur
  strcpy(wifiSSID, "VotreSSID");      // ⬅️ MODIFIER ICI
  strcpy(wifiPassword, "VotrePass");   // ⬅️ MODIFIER ICI
  manualOverride = false;
  pumpState = false;
}
```

**Avantages**:
- Configuration prise en compte dès le premier démarrage
- Pas besoin de reconfigurer après une réinitialisation
- Backup dans le code source

---

## Structure de la configuration

### SpaConfig (structure principale)

```cpp
struct SpaConfig {
  // Paramètres de température
  float tempDifferenceThreshold;  // Écart Panel-Spa pour activation (°C)
  float minPanelTemp;             // Température minimum panneaux (°C)
  float maxSpaTemp;               // Température maximum spa (°C)

  // Paramètres WiFi
  char wifiSSID[64];              // Nom du réseau
  char wifiPassword[64];          // Mot de passe

  // État du système
  bool manualOverride;            // Mode manuel actif
  bool pumpState;                 // État pompe en mode manuel
};
```

### Valeurs par défaut

| Paramètre | Valeur par défaut | Plage recommandée |
|-----------|-------------------|-------------------|
| `tempDifferenceThreshold` | 5.0°C | 2.0 - 10.0°C |
| `minPanelTemp` | 25.0°C | 20.0 - 40.0°C |
| `maxSpaTemp` | 40.0°C | 35.0 - 42.0°C |
| `wifiSSID` | "YOUR_WIFI_SSID" | - |
| `wifiPassword` | "YOUR_WIFI_PASSWORD" | - |
| `manualOverride` | false | true/false |
| `pumpState` | false | true/false |

---

## Modification des paramètres

### Via l'interface Web

#### 1. Paramètres de température

**Chemin**: Interface web → Configuration → Onglet "Températures"

- Modifiez les valeurs dans les champs
- Cliquez sur **"💾 Enregistrer"**
- ✅ Sauvegarde immédiate en mémoire flash
- ✅ Aucun redémarrage nécessaire

**Exemple**:
```
Différence de température: 7.0°C
Température minimum panneaux: 30.0°C
Température maximum spa: 38.0°C
```

#### 2. Paramètres WiFi

**Chemin**: Interface web → Configuration → Onglet "WiFi"

- Saisissez le nouveau SSID
- Saisissez le nouveau mot de passe
- Cliquez sur **"💾 Enregistrer WiFi"**
- ⚠️ **Redémarrage requis** pour appliquer

**Note**: L'ancienne connexion reste active jusqu'au redémarrage.

#### 3. Contrôle manuel de la pompe

**Chemin**: Interface web → Contrôle manuel

- **✅ Marche forcée**: Active `manualOverride = true` et `pumpState = true`
- **❌ Arrêt forcé**: Active `manualOverride = true` et `pumpState = false`
- **🔄 Mode automatique**: Désactive `manualOverride = false`

✅ L'état est automatiquement sauvegardé et restauré après coupure de courant.

### Via le moniteur série

Le système affiche la configuration au démarrage:

```
╔════════════════════════════════════════╗
║      Current Configuration             ║
╚════════════════════════════════════════╝

[Temperature Parameters]
  Temp difference threshold: 5.0°C
  Min panel temp: 25.0°C
  Max spa temp: 40.0°C

[WiFi Settings]
  SSID: MonReseau
  Password: ****

[Pump State]
  Manual override: NO
  Pump state: OFF
════════════════════════════════════════
```

---

## Stockage en mémoire flash

### Namespace

Les données sont stockées dans le namespace `"spa-control"` de la mémoire NVS (Non-Volatile Storage) de l'ESP32.

### Clés de stockage

| Clé | Type | Description |
|-----|------|-------------|
| `tempDiff` | float | Différence de température |
| `minPanel` | float | Température minimum panneaux |
| `maxSpa` | float | Température maximum spa |
| `wifiSSID` | string | SSID WiFi |
| `wifiPass` | string | Mot de passe WiFi |
| `manualMode` | bool | Mode manuel actif |
| `pumpState` | bool | État de la pompe |

### Capacité

La mémoire NVS de l'ESP32 peut contenir plusieurs dizaines de kilo-octets de données. La configuration actuelle utilise moins de 200 octets.

### Durabilité

⚠️ La mémoire flash a une durée de vie limitée en nombre de cycles d'écriture (typiquement 100,000 cycles).

Le système minimise les écritures:
- Sauvegarde uniquement lors de modifications
- Pas de sauvegarde périodique automatique
- Sauvegarde ciblée (seulement les paramètres modifiés)

---

## Restauration et sauvegarde

### Chargement automatique au démarrage

```cpp
// Séquence de démarrage
1. Lecture de la mémoire flash
2. Si succès → utilise les valeurs chargées
3. Si échec → utilise les valeurs par défaut du constructeur
4. Affichage de la configuration dans le moniteur série
```

**Exemple de log**:
```
[Initializing Configuration]
✓ Configuration loaded from flash

╔════════════════════════════════════════╗
║      Current Configuration             ║
╚════════════════════════════════════════╝
...
```

### Méthodes de sauvegarde

#### 1. `save()` - Sauvegarde complète
```cpp
configManager.save(config);
```
Sauvegarde tous les paramètres (température, WiFi, état pompe).

#### 2. `saveTempParams()` - Paramètres de température uniquement
```cpp
configManager.saveTempParams(config);
```
Utilisé lors de la modification des seuils de température via le web.

#### 3. `saveWiFi()` - Identifiants WiFi uniquement
```cpp
configManager.saveWiFi(config);
```
Utilisé lors de la modification du WiFi via le web.

#### 4. `savePumpState()` - État de la pompe uniquement
```cpp
configManager.savePumpState(config);
```
Utilisé lors du changement de mode manuel/automatique.

### Réinitialisation

#### Via l'interface Web

**Chemin**: Interface web → Configuration → Onglet "Système" → "🔄 Réinitialiser"

- Efface toutes les données de la mémoire flash
- Restaure les valeurs par défaut du constructeur `SpaConfig()`
- ⚠️ Redémarrage recommandé

#### Via le code

```cpp
configManager.reset();  // Efface tout
SpaConfig defaultConfig;  // Crée config par défaut
config = defaultConfig;   // Applique les valeurs par défaut
```

---

## Cas d'usage courants

### Scénario 1: Premier démarrage

```
1. L'ESP32 démarre
2. Aucune configuration en flash → utilise valeurs par défaut
3. Se connecte au WiFi configuré dans config.h
4. Vous accédez à l'interface web
5. Vous configurez les paramètres
6. Sauvegarde automatique en flash
7. Au prochain démarrage → configuration restaurée
```

### Scénario 2: Changement de réseau WiFi

```
1. Accédez à l'interface web sur l'ancien réseau
2. Configuration → WiFi
3. Saisissez nouveau SSID et mot de passe
4. Cliquez "Enregistrer WiFi"
5. Redémarrez l'ESP32 (via web ou bouton reset)
6. L'ESP32 se connecte au nouveau réseau
```

### Scénario 3: Coupure de courant

```
Mode automatique:
1. Coupure → ESP32 s'éteint
2. Retour du courant → ESP32 redémarre
3. Configuration chargée depuis la flash
4. Mode automatique restauré
5. Fonctionnement normal

Mode manuel (pompe ON):
1. Coupure → pompe s'arrête (évidemment)
2. Retour du courant → ESP32 redémarre
3. Configuration chargée: manualOverride=true, pumpState=true
4. Pompe réactivée immédiatement
5. Continue en mode manuel
```

### Scénario 4: Problème de capteur

```
1. Un capteur est défectueux
2. Le système détecte la valeur DEVICE_DISCONNECTED_C
3. Température mise à 0.0°C
4. Message dans le moniteur série: "⚠ X sensor disconnected!"
5. La pompe ne s'active pas (température invalide)
6. Configuration reste intacte
```

---

## Bonnes pratiques

### ✅ Faire

- Toujours noter votre configuration avant réinitialisation
- Sauvegarder les identifiants WiFi quelque part
- Tester les nouveaux paramètres progressivement
- Vérifier les logs dans le moniteur série
- Utiliser des valeurs raisonnables pour les températures

### ❌ Éviter

- Modifier les paramètres en continu (usure de la flash)
- Utiliser des valeurs extrêmes de température
- Réinitialiser fréquemment sans raison
- Oublier de redémarrer après changement WiFi
- Désactiver les limites de sécurité

---

## Dépannage de la configuration

### Problème: Configuration non sauvegardée

**Symptômes**: Les paramètres reviennent aux valeurs par défaut après redémarrage.

**Solutions**:
1. Vérifier les messages dans le moniteur série:
   - Cherchez "✓ Configuration saved"
   - Cherchez "Failed to open preferences"
2. Si échec d'écriture → mémoire flash potentiellement défectueuse
3. Essayer de réinitialiser: `configManager.reset()`
4. En dernier recours: reflasher l'ESP32 complètement

### Problème: WiFi ne fonctionne pas après modification

**Solutions**:
1. Vérifier que l'ESP32 a bien été redémarré
2. Vérifier le SSID (sensible à la casse)
3. Vérifier que le réseau est en 2.4 GHz
4. Consulter le moniteur série:
   ```
   [Connecting to WiFi]
   SSID: NouveauSSID
   Connecting..................
   ✗ WiFi connection failed!
   ```
5. Si nécessaire, reflasher avec les bons identifiants dans `config.h`

### Problème: Pompe ne restaure pas son état manuel

**Vérification**:
1. Moniteur série au démarrage:
   ```
   Restored manual pump state: ON
   ```
2. Si ce message n'apparaît pas → `manualOverride` n'était pas `true`
3. Vérifier dans l'interface web que le mode manuel est bien activé

---

## Référence API ConfigManager

### Constructeur
```cpp
ConfigManager configManager;
```

### Méthodes publiques

#### `bool load(SpaConfig& config)`
Charge la configuration depuis la mémoire flash.
- **Retour**: `true` si succès, `false` si échec
- **Effet**: Remplit la structure `config` avec les valeurs chargées

#### `bool save(const SpaConfig& config)`
Sauvegarde toute la configuration dans la flash.
- **Retour**: `true` si succès, `false` si échec

#### `bool saveTempParams(const SpaConfig& config)`
Sauvegarde uniquement les paramètres de température.

#### `bool saveWiFi(const SpaConfig& config)`
Sauvegarde uniquement les identifiants WiFi.

#### `bool savePumpState(const SpaConfig& config)`
Sauvegarde uniquement l'état de la pompe et le mode manuel.

#### `bool reset()`
Efface toute la configuration de la flash.

#### `void printConfig(const SpaConfig& config)`
Affiche la configuration dans le moniteur série (formaté).

---

## Exemple d'utilisation avancée

### Créer un profil "été" et "hiver"

Vous pouvez créer deux configurations et les charger selon la saison:

```cpp
// Dans main.cpp

SpaConfig summerConfig;
summerConfig.tempDifferenceThreshold = 3.0;  // Plus sensible l'été
summerConfig.minPanelTemp = 20.0;            // Activation plus tôt
summerConfig.maxSpaTemp = 38.0;              // Limite plus basse

SpaConfig winterConfig;
winterConfig.tempDifferenceThreshold = 8.0;  // Moins sensible l'hiver
winterConfig.minPanelTemp = 30.0;            // Besoin de plus de chaleur
winterConfig.maxSpaTemp = 42.0;              // Limite plus haute

// Charger selon la saison (à implémenter)
if (isSummer) {
  config = summerConfig;
} else {
  config = winterConfig;
}
configManager.save(config);
```

---

**Version du guide**: 2.0  
**Dernière mise à jour**: 2026-04-13
