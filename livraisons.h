#pragma once
#include <QWidget>
#include <QStandardItemModel>

namespace Ui { class Livraisons; }

class Livraisons : public QWidget
{
    Q_OBJECT
public:
    explicit Livraisons(QWidget *parent = nullptr);
    ~Livraisons();

private slots:
    void ajouter();                                   // bouton Ajouter
    void modifier();                                  // bouton Modifier
    void supprimer();                                 // bouton Supprimer
    void rechercher();                                // bouton Rechercher
    void exporterPDF();                               // bouton Bordereau PDF
    void remplirFormulaire(const QModelIndex &index); // clic sur une ligne du tableau

private:
    void ecrireLigne(int r);                          // copie le formulaire dans la ligne r
    void viderFormulaire();
    QColor couleurStatut(const QString &statut) const;

    Ui::Livraisons *ui;
    QStandardItemModel *model;
    int prochainId = 505;                             // prochain ID de livraison
};
