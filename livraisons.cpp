#include "livraisons.h"
#include "ui_livraisons.h"
#include <QColor>
#include <QMessageBox>
#include <QFileDialog>
#include <QPrinter>
#include <QTextDocument>

// Numéros des colonnes du modèle
// (0 à 5 sont visibles, 6 à 9 sont cachées mais servent à stocker le reste du formulaire)
enum Colonne { ID, COMMANDE, CHAUFFEUR, ZONE, DEPART, STATUT,
               VEHICULE, ADR_DEPART, ADR_LIVRAISON, ARRIVEE, NB_COLONNES };

static const QString FORMAT_DATE = "dd/MM/yyyy HH:mm";

Livraisons::Livraisons(QWidget *parent) : QWidget(parent), ui(new Ui::Livraisons)
{
    ui->setupUi(this);

    // ----- Modèle du tableau -----
    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"ID", "Commande", "Chauffeur", "Zone", "Départ", "Statut",
                                      "Véhicule", "Adresse départ", "Adresse livraison", "Livraison prévue"});

    // Données d'exemple
    const QList<QStringList> rows = {
        {"501", "101", "Ali Ben Salah", "Tunis Nord", "02/10/2026 08:00", "En cours",
         "123 TUN 4567", "Dépôt Charguia", "Lac 1", "02/10/2026 11:00"},
        {"502", "102", "Sami Trabelsi", "Ariana",     "02/10/2026 09:30", "Planifiée",
         "145 TUN 8910", "Dépôt Charguia", "Ariana Ville", "02/10/2026 12:00"},
        {"503", "103", "Ali Ben Salah", "Ben Arous",  "30/09/2026 07:00", "Livrée",
         "123 TUN 4567", "Dépôt Charguia", "Ben Arous Centre", "30/09/2026 10:00"},
        {"504", "104", "Nizar Gharbi",  "La Marsa",   "01/10/2026 10:00", "Retardée",
         "200 TUN 1122", "Dépôt Charguia", "La Marsa Plage", "01/10/2026 13:00"},
    };
    for (const QStringList &r : rows) {
        QList<QStandardItem *> ligne;
        for (const QString &texte : r)
            ligne << new QStandardItem(texte);
        ligne[STATUT]->setForeground(couleurStatut(r[STATUT]));   // couleur du statut
        model->appendRow(ligne);
    }
    ui->tableView->setModel(model);
    for (int c = VEHICULE; c < NB_COLONNES; ++c)
        ui->tableView->setColumnHidden(c, true);                  // colonnes cachées

    // ----- Connexion des boutons aux fonctions -----
    connect(ui->btnAjouter,    &QPushButton::clicked, this, &Livraisons::ajouter);
    connect(ui->btnModifier,   &QPushButton::clicked, this, &Livraisons::modifier);
    connect(ui->btnSupprimer,  &QPushButton::clicked, this, &Livraisons::supprimer);
    connect(ui->btnRechercher, &QPushButton::clicked, this, &Livraisons::rechercher);
    connect(ui->btnPdf,        &QPushButton::clicked, this, &Livraisons::exporterPDF);
    connect(ui->lineEditRecherche, &QLineEdit::returnPressed, this, &Livraisons::rechercher); // touche Entrée
    connect(ui->tableView,     &QTableView::clicked,  this, &Livraisons::remplirFormulaire);
}

Livraisons::~Livraisons() { delete ui; }

// Couleur selon le statut
QColor Livraisons::couleurStatut(const QString &statut) const
{
    if (statut == "En cours")  return QColor("#00B4D8");
    if (statut == "Planifiée") return QColor("#F39C12");
    if (statut == "Livrée")    return QColor("#2ECC71");
    return QColor("#E74C3C");                                     // Retardée / Échouée
}

// Copie les valeurs du formulaire dans la ligne r du tableau
void Livraisons::ecrireLigne(int r)
{
    model->item(r, COMMANDE)->setText(ui->comboCommande->currentText());
    model->item(r, CHAUFFEUR)->setText(ui->lineEditChauffeur->text());
    model->item(r, ZONE)->setText(ui->comboZone->currentText());
    model->item(r, DEPART)->setText(ui->dateTimeEditDepart->dateTime().toString(FORMAT_DATE));
    model->item(r, STATUT)->setText(ui->comboStatut->currentText());
    model->item(r, STATUT)->setForeground(couleurStatut(ui->comboStatut->currentText()));
    model->item(r, VEHICULE)->setText(ui->lineEditVehicule->text());
    model->item(r, ADR_DEPART)->setText(ui->lineEditDepart->text());
    model->item(r, ADR_LIVRAISON)->setText(ui->lineEditArrivee->text());
    model->item(r, ARRIVEE)->setText(ui->dateTimeEditArrivee->dateTime().toString(FORMAT_DATE));
}

void Livraisons::viderFormulaire()
{
    ui->lineEditChauffeur->clear();
    ui->lineEditVehicule->clear();
    ui->lineEditDepart->clear();
    ui->lineEditArrivee->clear();
}

// ---------- AJOUTER ----------
void Livraisons::ajouter()
{
    if (ui->lineEditChauffeur->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Entrez le nom du chauffeur !");
        return;
    }
    if (ui->dateTimeEditArrivee->dateTime() <= ui->dateTimeEditDepart->dateTime()) {
        QMessageBox::warning(this, "Erreur", "La livraison doit être après le départ !");
        return;
    }

    // On crée une ligne vide, puis on la remplit avec le formulaire
    QList<QStandardItem *> ligne;
    for (int c = 0; c < NB_COLONNES; ++c)
        ligne << new QStandardItem("");
    ligne[ID]->setText(QString::number(prochainId++));
    model->appendRow(ligne);
    ecrireLigne(model->rowCount() - 1);

    viderFormulaire();
}

// ---------- MODIFIER ----------
void Livraisons::modifier()
{
    int r = ui->tableView->currentIndex().row();
    if (r < 0) {
        QMessageBox::warning(this, "Erreur", "Sélectionnez une ligne !");
        return;
    }
    if (ui->lineEditChauffeur->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Entrez le nom du chauffeur !");
        return;
    }
    ecrireLigne(r);
}

// ---------- SUPPRIMER ----------
void Livraisons::supprimer()
{
    int r = ui->tableView->currentIndex().row();
    if (r < 0) {
        QMessageBox::warning(this, "Erreur", "Sélectionnez une ligne !");
        return;
    }
    auto rep = QMessageBox::question(this, "Confirmation", "Supprimer cette livraison ?");
    if (rep == QMessageBox::Yes) {
        model->removeRow(r);
        viderFormulaire();
    }
}

// ---------- RECHERCHER ----------
void Livraisons::rechercher()
{
    QString texte  = ui->lineEditRecherche->text().trimmed().toLower();
    QString statut = ui->comboFiltreStatut->currentText();
    bool tous = (ui->comboFiltreStatut->currentIndex() == 0);     // "Tous les statuts"

    for (int r = 0; r < model->rowCount(); ++r) {
        QString chauffeur = model->item(r, CHAUFFEUR)->text().toLower();
        QString zone      = model->item(r, ZONE)->text().toLower();
        QString st        = model->item(r, STATUT)->text();

        bool okTexte  = chauffeur.contains(texte) || zone.contains(texte);
        bool okStatut = tous || st == statut;

        ui->tableView->setRowHidden(r, !(okTexte && okStatut));   // cache les lignes qui ne correspondent pas
    }
}

// ---------- CLIC SUR UNE LIGNE : remplit le formulaire ----------
void Livraisons::remplirFormulaire(const QModelIndex &index)
{
    int r = index.row();
    ui->comboCommande->setCurrentText(model->item(r, COMMANDE)->text());
    ui->lineEditChauffeur->setText(model->item(r, CHAUFFEUR)->text());
    ui->comboZone->setCurrentText(model->item(r, ZONE)->text());
    ui->comboStatut->setCurrentText(model->item(r, STATUT)->text());
    ui->lineEditVehicule->setText(model->item(r, VEHICULE)->text());
    ui->lineEditDepart->setText(model->item(r, ADR_DEPART)->text());
    ui->lineEditArrivee->setText(model->item(r, ADR_LIVRAISON)->text());
    ui->dateTimeEditDepart->setDateTime(QDateTime::fromString(model->item(r, DEPART)->text(), FORMAT_DATE));
    ui->dateTimeEditArrivee->setDateTime(QDateTime::fromString(model->item(r, ARRIVEE)->text(), FORMAT_DATE));
}

// ---------- BORDEREAU PDF ----------
void Livraisons::exporterPDF()
{
    int r = ui->tableView->currentIndex().row();
    if (r < 0) {
        QMessageBox::warning(this, "Erreur", "Sélectionnez une livraison !");
        return;
    }
    QString fichier = QFileDialog::getSaveFileName(this, "Enregistrer le bordereau",
                                                   "bordereau_" + model->item(r, ID)->text() + ".pdf",
                                                   "PDF (*.pdf)");
    if (fichier.isEmpty()) return;

    auto t = [&](int c) { return model->item(r, c)->text().toHtmlEscaped(); };

    QString html = "<h1 style='color:#0A4D8C'>WaterFlow - Bordereau de livraison</h1>"
                   "<p><b>N° livraison :</b> "      + t(ID) + "</p>"
                   "<p><b>Commande :</b> "          + t(COMMANDE) + "</p>"
                   "<p><b>Chauffeur :</b> "         + t(CHAUFFEUR) + "</p>"
                   "<p><b>Véhicule :</b> "          + t(VEHICULE) + "</p>"
                   "<p><b>Zone :</b> "              + t(ZONE) + "</p>"
                   "<p><b>Adresse de départ :</b> " + t(ADR_DEPART) + "</p>"
                   "<p><b>Adresse de livraison :</b> " + t(ADR_LIVRAISON) + "</p>"
                   "<p><b>Départ prévu :</b> "      + t(DEPART) + "</p>"
                   "<p><b>Livraison prévue :</b> "  + t(ARRIVEE) + "</p>"
                   "<p><b>Statut :</b> "            + t(STATUT) + "</p>";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fichier);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, "PDF", "Bordereau enregistré !");
}
