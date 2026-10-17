#pragma once
#include <QMainWindow>
#include <QVector>

class QComboBox;
class QDateTimeEdit;
class QFrame;
class QLabel;
class QLineEdit;
class QModelIndex;
class QProgressBar;
class QSpinBox;
class QTableView;
class BarChartWidget;
class LotModel;
class LotFilterProxy;
struct Lot;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onCurrentRowChanged(const QModelIndex &current);
    void addLot();
    void saveLot();
    void deleteLot();
    void exportPdf();
    void applyFilters();
    void applySort();
    void refreshDashboard();

private:
    void buildUi();
    QWidget *buildSidebar();
    QWidget *buildProductionPage();
    QWidget *buildPlaceholderPage(const QString &title);
    QFrame *makeCard();

    void fillForm(const Lot &lot);
    void resetForm();
    bool readForm(Lot &out);
    void selectLotById(int id);

    LotModel *m_model = nullptr;
    LotFilterProxy *m_proxy = nullptr;
    QString m_path;
    int m_currentId = -1;

    // liste
    QTableView *m_table = nullptr;
    QLineEdit *m_search = nullptr;
    QComboBox *m_productFilter = nullptr, *m_statusFilter = nullptr, *m_sortCombo = nullptr;

    // formulaire
    QComboBox *f_product = nullptr, *f_line = nullptr, *f_status = nullptr;
    QSpinBox *f_produced = nullptr, *f_stock = nullptr, *f_threshold = nullptr;
    QLineEdit *f_operator = nullptr;
    QDateTimeEdit *f_date = nullptr;

    // tableau de bord
    QLabel *m_subtitle = nullptr, *m_banner = nullptr;
    QVector<QProgressBar *> m_bars;
    QVector<QLabel *> m_pctLabels;
    BarChartWidget *m_chart = nullptr;
    QLabel *m_forecastText = nullptr, *m_recoLabel = nullptr;
};
