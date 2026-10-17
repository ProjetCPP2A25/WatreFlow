#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QDateEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QVector>

struct Equipment {
    int id;
    QString name;
    QString type;
    QString location;
    QDate commissioning;
    int frequency;
    QDate lastMaintenance;
    QString state;
    QString manager;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QVector<Equipment> equipment;
    QTableWidget *table;
    QLineEdit *searchEdit;
    QComboBox *typeFilter;
    QComboBox *stateFilter;
    QLabel *totalValue;
    QLabel *serviceValue;
    QLabel *maintenanceValue;
    QLabel *brokenValue;
    QLabel *nextMaintenanceValue;

    QLineEdit *nameEdit;
    QComboBox *typeEdit;
    QLineEdit *locationEdit;
    QDateEdit *commissioningEdit;
    QSpinBox *frequencyEdit;
    QDateEdit *lastMaintenanceEdit;
    QComboBox *stateEdit;
    QLineEdit *managerEdit;
    int editingId = -1;

    QWidget *buildEquipmentPage();
    QWidget *buildDashboardPage();
    QWidget *buildMaintenancePage();
    QWidget *buildStatisticsPage();
    void setupStyle();
    void seedData();
    void refreshTable();
    void refreshMetrics();
    void refreshFilters();
    void clearForm();
    void loadSelectedIntoForm();
    void saveEquipment();
    void deleteEquipment();
    void showPage(QWidget *page, const QString &title);
    QDate nextMaintenance(const Equipment &e) const;
    QString stateColor(const QString &state) const;
};

#endif
