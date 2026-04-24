# Simulateur Web - Test sans ESP32

Ce simulateur permet de tester l'interface web complète **sans avoir besoin de l'ESP32 physique**.

## 🎯 Fonctionnalités simulées

✅ **Toutes les fonctionnalités de l'interface web:**
- Affichage des températures en temps réel (avec variations aléatoires)
- Contrôle de la pompe (Manuel ON/OFF/AUTO)
- Configuration des paramètres de température
- Configuration WiFi
- Scanner WiFi (retourne 6 réseaux fictifs)
- Journaux système avec horodatage
- Informations système (version, mémoire, uptime)
- Barres de progression du stockage
- **📊 Graphique 24h** avec historique réaliste des températures

## 📋 Prérequis

- **Python 3.x** installé sur votre ordinateur
- Aucune bibliothèque externe requise (utilise uniquement la bibliothèque standard)

## 🚀 Lancement du simulateur

### Méthode 1: Ligne de commande

```bash
cd C:\dev\ChauffeEauSolaireClaude
python web_simulator.py
```

### Méthode 2: Double-clic

Double-cliquez simplement sur `web_simulator.py` dans l'explorateur Windows.

## 🌐 Accès à l'interface

Une fois le simulateur démarré, ouvrez votre navigateur et accédez à:

```
http://localhost:8080
```

ou

```
http://127.0.0.1:8080
```

## 🎮 Utilisation

### Températures simulées
- **Air**: ~20.5°C (variations aléatoires ±0.2°C)
- **Spa**: ~20.1°C (variations aléatoires ±0.1°C)
- **Panneaux**: ~15.0°C (variations aléatoires ±0.5°C)
- Les températures sont mises à jour à chaque requête

### Graphique 24h
- 📊 Historique pré-généré de 24 heures (144 points espacés de 10 minutes)
- Cycle journalier réaliste:
  - **Air**: 18-28°C, plus frais la nuit, plus chaud l'après-midi
  - **Spa**: 28-35°C, se réchauffe progressivement dans l'après-midi
  - **Panneaux**: 15-50°C, pic autour de 13h (suit le soleil)
- Nouvel enregistrement toutes les minutes pendant l'exécution
- Graphique interactif avec Chart.js

### Contrôles fonctionnels
- ✅ Boutons de contrôle manuel de pompe
- ✅ Modification des paramètres de température
- ✅ Scanner WiFi (6 réseaux simulés)
- ✅ Changement de configuration WiFi
- ✅ Effacement des logs
- ✅ Toutes les actions génèrent des logs

### Informations système
- Version: 2.4
- Modèle: ESP32-SIMULATOR
- Temps de fonctionnement: Calcul en temps réel
- Stockage firmware: ~68%
- Stockage filesystem: ~1%

## 🛑 Arrêt du simulateur

Appuyez sur **Ctrl+C** dans le terminal pour arrêter le serveur.

## 🔧 Personnalisation

Vous pouvez modifier `web_simulator.py` pour:
- Changer le port (par défaut: 8080)
- Ajuster les températures initiales
- Modifier les réseaux WiFi simulés
- Ajouter des scénarios de test spécifiques

## 📝 Notes

- Le simulateur ne sauvegarde PAS les modifications (tout est en mémoire)
- Redémarrer le simulateur réinitialise tout à l'état initial
- Les données ne sont PAS synchronisées entre plusieurs onglets
- Upload OTA non simulé (pas de fichiers réels à uploader)

## 🐛 Dépannage

### Port déjà utilisé
Si le port 8080 est occupé, modifiez la dernière ligne de `web_simulator.py`:
```python
run_simulator(8081)  # Changez le numéro de port
```

### Fichier index.html non trouvé
Assurez-vous d'être dans le bon répertoire:
```bash
cd C:\dev\ChauffeEauSolaireClaude
```

### Le navigateur ne charge pas la page
- Vérifiez que le simulateur est bien lancé
- Essayez http://127.0.0.1:8080 au lieu de localhost
- Videz le cache du navigateur (Ctrl+F5)

## 🎓 Cas d'usage

### Développement de l'interface
- Testez rapidement des modifications CSS/HTML/JavaScript
- Pas besoin de recompiler et uploader à chaque changement
- Développement plus rapide

### Démonstration
- Montrez l'interface à quelqu'un sans avoir l'ESP32
- Faites des captures d'écran
- Créez des vidéos de démonstration

### Tests fonctionnels
- Vérifiez que tous les boutons fonctionnent
- Testez les formulaires
- Validez les interactions utilisateur

## 📸 Exemple de sortie

```
============================================================
  ESP32 Solar Spa Controller - Web Simulator
============================================================

  Server running at: http://localhost:8080
  Or access via:     http://127.0.0.1:8080

  Press Ctrl+C to stop the simulator

============================================================
```

---

**Astuce**: Gardez le terminal ouvert pendant que vous utilisez l'interface web. Vous pouvez voir les requêtes HTTP en temps réel si besoin.
