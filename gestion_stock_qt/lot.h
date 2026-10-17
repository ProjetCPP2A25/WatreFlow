#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
//  Un lot de production
// ---------------------------------------------------------------------------
struct Lot {
    int       id = 0;
    QDateTime dateTime;
    QString   line;          // "Ligne 1" ...
    QString   product;       // "Eau plate 0,5 L" ...
    int       produced = 0;  // quantité produite
    int       stock = 0;     // stock après production
    int       threshold = 0; // seuil d'alerte du lot
    QString   operatorName;
    QString   status;        // Terminé / En cours / Planifié

    QJsonObject toJson() const;
    static Lot  fromJson(const QJsonObject &o);
};

// ---------------------------------------------------------------------------
//  Paramètres métier (à adapter facilement)
// ---------------------------------------------------------------------------
namespace WF {

inline const QStringList &products() {
    static const QStringList p{"Eau plate 0,5 L", "Eau plate 1,5 L",
                               "Eau gazeuse 0,5 L", "Bonbonne 19 L"};
    return p;
}
inline const QStringList &lines() {
    static const QStringList l{"Ligne 1", "Ligne 2", "Ligne 3", "Ligne 4"};
    return l;
}
inline const QStringList &statuses() {
    static const QStringList s{"Terminé", "En cours", "Planifié"};
    return s;
}

constexpr int kAlertPercent = 10;   // alerte rouge si stock < 10 % du max
constexpr int kReorderPercent = 30; // recommandation de production si < 30 %

int maxStock(const QString &product); // capacité maximale de stock par produit

inline QString fmt(int n) { // 24000 -> "24 000"
    QString s = QString::number(qAbs(n));
    for (int i = s.size() - 3; i > 0; i -= 3)
        s.insert(i, QChar(0x00A0));
    return (n < 0 ? "-" : "") + s;
}

} // namespace WF
