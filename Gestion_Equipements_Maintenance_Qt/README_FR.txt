WATERFLOW — Gestion des Équipements et de la Maintenance
========================================================

Projet d'interface de bureau en C++ / Qt Widgets, basé sur le cahier de spécification WaterFlow.

FONCTIONNALITÉS DE L'INTERFACE
- Tableau de bord avec indicateurs du parc.
- Gestion des équipements : ajout, consultation, modification et suppression (CRUD).
- Recherche textuelle et filtres par type et par état.
- Calcul automatique de la prochaine maintenance : dernière maintenance + fréquence en jours.
- Planning des maintenances et alertes d'échéance.
- Vue statistiques : répartition par état/type et indice de fiabilité simplifié.
- Export du tableau des équipements en PDF.
- Jeu de données de démonstration préchargé.

IMPORTANT
Cette version est une interface fonctionnelle de démonstration. Les données sont conservées en mémoire pendant l'exécution et sont réinitialisées au redémarrage. Aucune base de données Oracle/SQLite n'est encore connectée.

COMPILATION AVEC QT CREATOR (WINDOWS)
1. Installer Qt avec un kit MinGW 64-bit compatible.
2. Vérifier que les composants Qt Widgets et Qt PrintSupport sont installés.
3. Extraire le ZIP dans un dossier.
4. Ouvrir GestionEquipementsMaintenance.pro avec Qt Creator.
5. Choisir un kit Desktop Qt MinGW 64-bit, puis Configure Project.
6. Cliquer sur Build puis Run.

DÉPENDANCES
- Qt Core
- Qt Gui
- Qt Widgets
- Qt PrintSupport

Si vous utilisez Qt 6, Qt Creator/qmake doit pouvoir résoudre QPrinter et QPageLayout via Qt PrintSupport.
