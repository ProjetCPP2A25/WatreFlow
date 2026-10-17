# WaterFlow — Gestion de la production et des stocks (Qt / C++)

Application Qt Widgets qui reproduit l'interface « Production & stocks ».

## Compiler

**Qt Creator (le plus simple)** : ouvrir `CMakeLists.txt` (ou `WaterFlow.pro`), choisir un kit Qt 6 (Qt 5.15 fonctionne aussi), puis Ctrl+R.

**Ligne de commande (CMake)** :
```
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./WaterFlow          # (WaterFlow.exe sous Windows)
```

**qmake** :
```
qmake WaterFlow.pro && make
```

## Fonctionnalités
- Tableau des lots (tri par clic sur l'en-tête, pastilles de statut colorées, stock rouge si < seuil)
- Recherche texte + filtre produit + filtre statut + tri (liste déroulante)
- Formulaire « Lot — Modifier » : **Ajouter / Enregistrer / Supprimer** (avec validation)
- Bandeau d'alerte automatique si un stock passe sous 10 % du maximum
- Niveaux de stock (barres vert / orange / rouge), graphique de production par jour (dessiné à la main, sans Qt Charts)
- Prévision : moyenne mobile 7 jours + recommandation de production
- Export PDF de la liste affichée (filtrée/triée)
- Sauvegarde automatique dans un fichier JSON (`QStandardPaths::AppDataLocation/lots.json`) ; données de démo au premier lancement
- Menu latéral : seule la page « Production & stocks » est implémentée, les autres sont des pages vides à compléter

## Où modifier quoi
| Besoin | Fichier |
|---|---|
| Produits, lignes, statuts, stock max, seuils d'alerte | `lot.h`, `lot.cpp` |
| Données de démo | `LotModel::seedDemo()` dans `lotmodel.cpp` |
| Demande des 7 derniers jours / règle de recommandation | `kDemand7d` et `refreshDashboard()` dans `mainwindow.cpp` |
| Couleurs et style | `kStyle` (feuille de style QSS) dans `mainwindow.cpp` |
| Passer à SQLite | remplacer `LotModel::load/save` par des requêtes `QSqlQuery` |

## Structure
```
main.cpp            point d'entrée
mainwindow.*        interface complète + logique (CRUD, filtres, PDF, tableau de bord)
lot.*               structure Lot + paramètres métier
lotmodel.*          modèle de table + proxy de filtrage + persistance JSON
barchartwidget.*    histogramme personnalisé
```
