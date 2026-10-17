#include "mainwindow.h"
#include "barchartwidget.h"
#include "lotmodel.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTextDocument>
#include <QVBoxLayout>
#include <cmath>

namespace {

// Demande journalière des 7 derniers jours (à remplacer par vos vraies données)
const QVector<int> kDemand7d{35000, 37000, 34000, 38000, 36000, 37500, 34500};

const char *kStyle = R"(
QMainWindow, QWidget#content { background:#e8f3fa; }
QWidget#sidebar { background:#0a4a8a; }
QLabel#appName { color:white; font-size:16px; font-weight:bold; }
QPushButton#nav { color:white; text-align:left; padding:9px 12px; border:none;
                  border-radius:4px; background:transparent; font-size:12px; }
QPushButton#nav:hover { background:#1a5fa8; }
QPushButton#nav:checked { background:#00aeda; font-weight:bold; }
QLabel#pageTitle { color:#0b4a8a; font-size:18px; font-weight:bold; }
QLabel#subtitle { color:#5b7089; font-size:11px; }
QFrame#card { background:white; border:1px solid #d3e4f0; border-radius:6px; }
QLabel#cardTitle { color:#0b3d73; font-weight:bold; font-size:12px; }
QLabel#fieldLabel { color:#5b7089; font-size:11px; }
QLabel#stockName { color:#0b3d73; font-size:11px; }
QLabel#stockPct { color:#0b3d73; font-weight:bold; font-size:11px; }
QLineEdit, QComboBox, QSpinBox, QDateTimeEdit {
    border:1px solid #c9dbe8; border-radius:4px; padding:5px 8px;
    background:white; color:#0b3d73; min-height:18px; }
QLineEdit:focus, QSpinBox:focus, QDateTimeEdit:focus { border:1px solid #00aeda; }
QTableView { border:none; background:white; alternate-background-color:#f1f6fa;
             selection-background-color:#d8f0fa; selection-color:#0b3d73;
             color:#0b3d73; outline:0; }
QTableView::item { padding:4px 6px; }
QHeaderView::section { background:#0d4b8b; color:white; border:none;
                       padding:8px 8px; font-weight:bold; }
QPushButton#primary { background:#00aeda; color:white; border:none;
                      border-radius:4px; padding:8px 14px; font-weight:bold; }
QPushButton#primary:hover { background:#0097bf; }
QPushButton#danger { background:#e74c3c; color:white; border:none;
                     border-radius:4px; padding:8px 14px; font-weight:bold; }
QPushButton#danger:hover { background:#c0392b; }
QStatusBar { background:#e8f3fa; color:#0b3d73; }
)";

// Pastille colorée dans la colonne « Statut »
class StatusDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *p, const QStyleOptionViewItem &opt,
               const QModelIndex &idx) const override {
        QStyleOptionViewItem o(opt);
        initStyleOption(&o, idx);
        const QString text = o.text;
        o.text.clear();
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &o, p, o.widget);

        QColor c("#5d7fa3");
        if (text == "Terminé")       c = QColor("#2ecc71");
        else if (text == "En cours") c = QColor("#f39c12");

        QFont f = o.font;
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF() * 0.9);
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->setFont(f);
        const int w = QFontMetrics(f).horizontalAdvance(text) + 18;
        const QRect r(o.rect.left() + 6, o.rect.center().y() - 10, w, 20);
        p->setPen(Qt::NoPen);
        p->setBrush(c);
        p->drawRoundedRect(r, 10, 10);
        p->setPen(Qt::white);
        p->drawText(r, Qt::AlignCenter, text);
        p->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
        QSize s = QStyledItemDelegate::sizeHint(opt, idx);
        s.setHeight(qMax(s.height(), 30));
        return s;
    }
};

QPixmap makeLogo(int s) {
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#00aeda"));
    p.drawEllipse(0, 0, s, s);
    QPainterPath path;
    const qreal c = s / 2.0;
    path.moveTo(c, s * 0.2);
    path.cubicTo(c + s * 0.05, s * 0.32, s * 0.74, s * 0.45, s * 0.74, s * 0.6);
    path.arcTo(QRectF(s * 0.26, s * 0.36, s * 0.48, s * 0.48), 0, -180);
    path.cubicTo(s * 0.26, s * 0.45, c - s * 0.05, s * 0.32, c, s * 0.2);
    p.setBrush(Qt::white);
    p.drawPath(path);
    return pm;
}

QString lowerFirst(const QString &s) { return s.left(1).toLower() + s.mid(1); }

} // namespace

// ===========================================================================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("WaterFlow — Gestion de la production et des stocks");
    resize(1180, 780);
    setMinimumSize(1020, 700);

    m_model = new LotModel(this);
    m_proxy = new LotFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(LotModel::SortRole);
    m_proxy->setDynamicSortFilter(true);

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_path = dir + "/lots.json";

    buildUi();

    if (!m_model->load(m_path)) {
        m_model->seedDemo();
        m_model->save(m_path);
    }
    // toute modification -> sauvegarde + rafraîchissement du tableau de bord
    connect(m_model, &LotModel::lotsChanged, this, [this] {
        m_model->save(m_path);
        refreshDashboard();
    });

    applySort();
    refreshDashboard();
    if (m_proxy->rowCount() > 0)
        m_table->selectRow(0);
    else
        resetForm();
}

// ---------------------------------------------------------------------------
QFrame *MainWindow::makeCard() {
    auto *f = new QFrame;
    f->setObjectName("card");
    return f;
}

void MainWindow::buildUi() {
    qApp->setStyleSheet(kStyle);

    auto *central = new QWidget;
    auto *h = new QHBoxLayout(central);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    h->addWidget(buildSidebar());

    auto *stack = new QStackedWidget;
    stack->setObjectName("content");
    const QStringList pages{"Tableau de bord", "Personnel", "Équipements",
                            "Production & stocks", "Commandes", "Livraisons",
                            "Statistiques", "Alertes"};
    for (const QString &p : pages)
        stack->addWidget(p == "Production & stocks" ? buildProductionPage()
                                                    : buildPlaceholderPage(p));
    h->addWidget(stack, 1);
    setCentralWidget(central);

    // navigation
    auto *group = central->findChild<QButtonGroup *>("navGroup");
    connect(group, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);
    stack->setCurrentIndex(3);
    statusBar()->showMessage("Prêt");
}

QWidget *MainWindow::buildSidebar() {
    auto *side = new QWidget;
    side->setObjectName("sidebar");
    side->setFixedWidth(138);
    auto *v = new QVBoxLayout(side);
    v->setContentsMargins(8, 14, 8, 14);
    v->setSpacing(4);

    auto *brand = new QHBoxLayout;
    brand->setContentsMargins(6, 0, 0, 0);
    auto *logo = new QLabel;
    logo->setPixmap(makeLogo(26));
    auto *name = new QLabel("WaterFlow");
    name->setObjectName("appName");
    brand->addWidget(logo);
    brand->addWidget(name);
    brand->addStretch();
    v->addLayout(brand);
    v->addSpacing(14);

    auto *group = new QButtonGroup(side);
    group->setObjectName("navGroup");
    group->setExclusive(true);
    const QStringList items{"Tableau de bord", "Personnel", "Équipements",
                            "Production & stocks", "Commandes", "Livraisons",
                            "Statistiques", "Alertes"};
    for (int i = 0; i < items.size(); ++i) {
        auto *b = new QPushButton(items[i]);
        b->setObjectName("nav");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setChecked(i == 3);
        group->addButton(b, i);
        v->addWidget(b);
    }
    v->addStretch();
    return side;
}

QWidget *MainWindow::buildPlaceholderPage(const QString &title) {
    auto *w = new QWidget;
    auto *v = new QVBoxLayout(w);
    v->setContentsMargins(16, 14, 16, 14);
    auto *t = new QLabel(title);
    t->setObjectName("pageTitle");
    auto *msg = new QLabel("Module en cours de développement.");
    msg->setObjectName("subtitle");
    msg->setAlignment(Qt::AlignCenter);
    v->addWidget(t);
    v->addStretch();
    v->addWidget(msg);
    v->addStretch();
    return w;
}

// ---------------------------------------------------------------------------
QWidget *MainWindow::buildProductionPage() {
    auto *page = new QWidget;
    auto *root = new QVBoxLayout(page);
    root->setContentsMargins(16, 14, 16, 12);
    root->setSpacing(10);

    // --- en-tête
    auto *head = new QHBoxLayout;
    auto *title = new QLabel("Gestion de la production et des stocks");
    title->setObjectName("pageTitle");
    m_subtitle = new QLabel;
    m_subtitle->setObjectName("subtitle");
    head->addWidget(title);
    head->addStretch();
    head->addWidget(m_subtitle);
    root->addLayout(head);

    // --- bandeau d'alerte
    m_banner = new QLabel;
    m_banner->setWordWrap(true);
    m_banner->setTextFormat(Qt::RichText);
    root->addWidget(m_banner);

    // --- barre d'outils
    auto *bar = new QHBoxLayout;
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Rechercher un lot (produit, ligne)...");
    m_productFilter = new QComboBox;
    m_productFilter->addItem("Tous les produits");
    m_productFilter->addItems(WF::products());
    m_statusFilter = new QComboBox;
    m_statusFilter->addItem("Tous les statuts");
    m_statusFilter->addItems(WF::statuses());
    m_sortCombo = new QComboBox;
    m_sortCombo->addItems({"Trier par date", "Trier par ID", "Trier par produit", "Trier par stock"});
    auto *btnPdf = new QPushButton("Exporter en PDF");
    btnPdf->setObjectName("primary");
    btnPdf->setCursor(Qt::PointingHandCursor);
    bar->addWidget(m_search, 1);
    bar->addWidget(m_productFilter);
    bar->addWidget(m_statusFilter);
    bar->addWidget(m_sortCombo);
    bar->addWidget(btnPdf);
    root->addLayout(bar);

    connect(m_search, &QLineEdit::textChanged, this, &MainWindow::applyFilters);
    connect(m_productFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::applyFilters);
    connect(m_statusFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::applyFilters);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::applySort);
    connect(btnPdf, &QPushButton::clicked, this, &MainWindow::exportPdf);

    // --- zone centrale : table + formulaire
    auto *mid = new QHBoxLayout;
    mid->setSpacing(10);

    QFrame *lotsCard = makeCard();
    auto *lv = new QVBoxLayout(lotsCard);
    lv->setContentsMargins(10, 10, 10, 10);
    auto *lt = new QLabel("Lots de production");
    lt->setObjectName("cardTitle");
    lv->addWidget(lt);

    m_table = new QTableView;
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->horizontalHeader()->setHighlightSections(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(LotModel::ColProduct, QHeaderView::Stretch);
    m_table->setColumnWidth(LotModel::ColId, 62);
    m_table->setColumnWidth(LotModel::ColDate, 104);
    m_table->setColumnWidth(LotModel::ColLine, 72);
    m_table->setColumnWidth(LotModel::ColProduced, 100);
    m_table->setColumnWidth(LotModel::ColStock, 72);
    m_table->setColumnWidth(LotModel::ColThreshold, 66);
    m_table->setColumnWidth(LotModel::ColStatus, 96);
    m_table->setItemDelegateForColumn(LotModel::ColStatus, new StatusDelegate(m_table));
    lv->addWidget(m_table, 1);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex &cur, const QModelIndex &) { onCurrentRowChanged(cur); });
    mid->addWidget(lotsCard, 1);

    // formulaire
    QFrame *formCard = makeCard();
    formCard->setFixedWidth(290);
    auto *fv = new QVBoxLayout(formCard);
    fv->setContentsMargins(12, 10, 12, 12);
    auto *ft = new QLabel("Lot — Modifier");
    ft->setObjectName("cardTitle");
    fv->addWidget(ft);

    f_product = new QComboBox;   f_product->addItems(WF::products());
    f_line = new QComboBox;      f_line->addItems(WF::lines());
    f_status = new QComboBox;    f_status->addItems(WF::statuses());
    auto makeSpin = [](int max) {
        auto *s = new QSpinBox;
        s->setRange(0, max);
        s->setLocale(QLocale(QLocale::French, QLocale::France));
        s->setGroupSeparatorShown(true);
        s->setButtonSymbols(QAbstractSpinBox::NoButtons);
        return s;
    };
    f_produced = makeSpin(1000000);
    f_stock = makeSpin(1000000);
    f_threshold = makeSpin(1000000);
    f_operator = new QLineEdit;
    f_date = new QDateTimeEdit(QDateTime::currentDateTime());
    f_date->setDisplayFormat("dd/MM/yyyy HH:mm");
    f_date->setCalendarPopup(true);

    auto *grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(3);
    auto addField = [&](int row, int col, const QString &label, QWidget *w) {
        auto *l = new QLabel(label);
        l->setObjectName("fieldLabel");
        grid->addWidget(l, row * 2, col);
        grid->addWidget(w, row * 2 + 1, col);
    };
    addField(0, 0, "Produit", f_product);          addField(0, 1, "Ligne", f_line);
    addField(1, 0, "Quantité produite", f_produced); addField(1, 1, "Stock", f_stock);
    addField(2, 0, "Seuil d'alerte", f_threshold); addField(2, 1, "Opérateur", f_operator);
    addField(3, 0, "Date / heure", f_date);        addField(3, 1, "Statut", f_status);
    fv->addLayout(grid);
    fv->addSpacing(6);

    auto *btns = new QHBoxLayout;
    auto *bAdd = new QPushButton("Ajouter");
    auto *bSave = new QPushButton("Enregistrer");
    auto *bDel = new QPushButton("Supprimer");
    bAdd->setObjectName("primary");
    bSave->setObjectName("primary");
    bDel->setObjectName("danger");
    for (auto *b : {bAdd, bSave, bDel}) {
        b->setCursor(Qt::PointingHandCursor);
        btns->addWidget(b);
    }
    fv->addLayout(btns);
    fv->addStretch();
    connect(bAdd, &QPushButton::clicked, this, &MainWindow::addLot);
    connect(bSave, &QPushButton::clicked, this, &MainWindow::saveLot);
    connect(bDel, &QPushButton::clicked, this, &MainWindow::deleteLot);
    mid->addWidget(formCard);
    root->addLayout(mid, 1);

    // --- ligne du bas : stocks / graphique / prévision
    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(10);

    QFrame *stockCard = makeCard();
    auto *sv = new QVBoxLayout(stockCard);
    sv->setContentsMargins(12, 10, 12, 10);
    auto *st = new QLabel("Niveaux de stock");
    st->setObjectName("cardTitle");
    sv->addWidget(st);
    for (const QString &prod : WF::products()) {
        auto *row = new QHBoxLayout;
        auto *n = new QLabel(prod);
        n->setObjectName("stockName");
        auto *pct = new QLabel("0%");
        pct->setObjectName("stockPct");
        pct->setAlignment(Qt::AlignRight);
        row->addWidget(n);
        row->addStretch();
        row->addWidget(pct);
        sv->addLayout(row);
        auto *pb = new QProgressBar;
        pb->setRange(0, 100);
        pb->setTextVisible(false);
        pb->setFixedHeight(9);
        sv->addWidget(pb);
        m_bars << pb;
        m_pctLabels << pct;
    }
    bottom->addWidget(stockCard, 1);

    QFrame *chartCard = makeCard();
    auto *cv = new QVBoxLayout(chartCard);
    cv->setContentsMargins(12, 10, 12, 8);
    auto *ct = new QLabel("Production par jour (milliers d'unités)");
    ct->setObjectName("cardTitle");
    cv->addWidget(ct);
    m_chart = new BarChartWidget;
    cv->addWidget(m_chart, 1);
    bottom->addWidget(chartCard, 1);

    QFrame *fcCard = makeCard();
    auto *fcv = new QVBoxLayout(fcCard);
    fcv->setContentsMargins(12, 10, 12, 10);
    auto *fct = new QLabel("Prévision de production");
    fct->setObjectName("cardTitle");
    fcv->addWidget(fct);
    m_forecastText = new QLabel;
    m_forecastText->setWordWrap(true);
    m_forecastText->setTextFormat(Qt::RichText);
    m_forecastText->setStyleSheet("color:#0b3d73;");
    fcv->addWidget(m_forecastText);
    m_recoLabel = new QLabel;
    m_recoLabel->setWordWrap(true);
    m_recoLabel->setTextFormat(Qt::RichText);
    m_recoLabel->setStyleSheet("color:#0b3d73; background:#eef7fc; border-left:3px solid #00aeda;"
                               "padding:8px 10px; border-radius:3px;");
    fcv->addWidget(m_recoLabel);
    fcv->addStretch();
    bottom->addWidget(fcCard, 1);

    for (QFrame *c : {stockCard, chartCard, fcCard})
        c->setMinimumHeight(205);
    root->addLayout(bottom);
    return page;
}

// ---------------------------------------------------------------------------
//  Formulaire
// ---------------------------------------------------------------------------
void MainWindow::fillForm(const Lot &l) {
    f_product->setCurrentText(l.product);
    f_line->setCurrentText(l.line);
    f_produced->setValue(l.produced);
    f_stock->setValue(l.stock);
    f_threshold->setValue(l.threshold);
    f_operator->setText(l.operatorName);
    f_date->setDateTime(l.dateTime);
    f_status->setCurrentText(l.status);
}

void MainWindow::resetForm() {
    f_product->setCurrentIndex(0);
    f_line->setCurrentIndex(0);
    f_produced->setValue(0);
    f_stock->setValue(0);
    f_threshold->setValue(0);
    f_operator->clear();
    f_date->setDateTime(QDateTime::currentDateTime());
    f_status->setCurrentText("En cours");
    m_currentId = -1;
}

bool MainWindow::readForm(Lot &l) {
    auto warn = [this](const QString &msg) {
        QMessageBox::warning(this, "Données invalides", msg);
        return false;
    };
    if (f_operator->text().trimmed().isEmpty())
        return warn("Veuillez renseigner l'opérateur.");
    if (f_produced->value() <= 0)
        return warn("La quantité produite doit être supérieure à 0.");
    const int maxS = WF::maxStock(f_product->currentText());
    if (f_stock->value() > maxS)
        return warn(QString("Le stock dépasse la capacité maximale de %1 (%2 unités).")
                        .arg(f_product->currentText(), WF::fmt(maxS)));
    l.dateTime = f_date->dateTime();
    l.line = f_line->currentText();
    l.product = f_product->currentText();
    l.produced = f_produced->value();
    l.stock = f_stock->value();
    l.threshold = f_threshold->value();
    l.operatorName = f_operator->text().trimmed();
    l.status = f_status->currentText();
    return true;
}

void MainWindow::selectLotById(int id) {
    const int srcRow = m_model->rowOfId(id);
    if (srcRow < 0)
        return;
    const QModelIndex px = m_proxy->mapFromSource(m_model->index(srcRow, 0));
    if (px.isValid()) {
        m_table->selectRow(px.row());
        m_table->scrollTo(px);
    }
}

void MainWindow::onCurrentRowChanged(const QModelIndex &cur) {
    if (!cur.isValid()) {
        m_currentId = -1;
        return;
    }
    const Lot &l = m_model->lotAt(m_proxy->mapToSource(cur).row());
    m_currentId = l.id;
    fillForm(l);
}

// ---------------------------------------------------------------------------
//  Actions CRUD
// ---------------------------------------------------------------------------
void MainWindow::addLot() {
    Lot l;
    if (!readForm(l))
        return;
    l.id = m_model->nextId();
    m_model->addLot(l);
    selectLotById(l.id);
    statusBar()->showMessage(QString("Lot %1 ajouté").arg(l.id), 4000);
}

void MainWindow::saveLot() {
    const int row = m_model->rowOfId(m_currentId);
    if (row < 0) {
        QMessageBox::information(this, "Enregistrer", "Sélectionnez d'abord un lot dans la liste.");
        return;
    }
    Lot l;
    if (!readForm(l))
        return;
    l.id = m_currentId;
    m_model->updateLot(row, l);
    statusBar()->showMessage(QString("Lot %1 enregistré").arg(l.id), 4000);
}

void MainWindow::deleteLot() {
    const int row = m_model->rowOfId(m_currentId);
    if (row < 0) {
        QMessageBox::information(this, "Supprimer", "Sélectionnez d'abord un lot dans la liste.");
        return;
    }
    const int id = m_currentId;
    if (QMessageBox::question(this, "Supprimer le lot",
                              QString("Supprimer définitivement le lot %1 ?").arg(id)) != QMessageBox::Yes)
        return;
    m_model->removeLot(row);
    if (m_proxy->rowCount() == 0)
        resetForm();
    statusBar()->showMessage(QString("Lot %1 supprimé").arg(id), 4000);
}

// ---------------------------------------------------------------------------
//  Filtres / tri
// ---------------------------------------------------------------------------
void MainWindow::applyFilters() {
    m_proxy->setSearch(m_search->text());
    m_proxy->setProduct(m_productFilter->currentIndex() == 0 ? QString() : m_productFilter->currentText());
    m_proxy->setStatus(m_statusFilter->currentIndex() == 0 ? QString() : m_statusFilter->currentText());
}

void MainWindow::applySort() {
    static const int cols[] = {LotModel::ColDate, LotModel::ColId, LotModel::ColProduct, LotModel::ColStock};
    m_table->sortByColumn(cols[qBound(0, m_sortCombo->currentIndex(), 3)], Qt::AscendingOrder);
}

// ---------------------------------------------------------------------------
//  Export PDF (liste filtrée/triée telle qu'affichée)
// ---------------------------------------------------------------------------
void MainWindow::exportPdf() {
    const QString path = QFileDialog::getSaveFileName(
        this, "Exporter en PDF",
        QDir::homePath() + "/lots_production_" + QDate::currentDate().toString("yyyyMMdd") + ".pdf",
        "PDF (*.pdf)");
    if (path.isEmpty())
        return;

    QString html = "<h2 style='color:#0d4b8b'>WaterFlow — Lots de production</h2>"
                   "<p>Export du " + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm") +
                   " — " + QString::number(m_proxy->rowCount()) + " lot(s)</p>"
                   "<table width='100%' border='1' cellspacing='0' cellpadding='4'>"
                   "<tr bgcolor='#0d4b8b' style='color:white'>";
    for (int c = 0; c < LotModel::ColCount; ++c)
        html += "<th>" + m_model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString().toHtmlEscaped() + "</th>";
    html += "</tr>";
    for (int r = 0; r < m_proxy->rowCount(); ++r) {
        const Lot &l = m_model->lotAt(m_proxy->mapToSource(m_proxy->index(r, 0)).row());
        html += "<tr>";
        html += "<td>" + QString::number(l.id) + "</td>";
        html += "<td>" + l.dateTime.toString("dd/MM/yyyy HH:mm") + "</td>";
        html += "<td>" + l.line.toHtmlEscaped() + "</td>";
        html += "<td>" + l.product.toHtmlEscaped() + "</td>";
        html += "<td align='right'>" + WF::fmt(l.produced) + "</td>";
        html += "<td align='right'>" + WF::fmt(l.stock) + "</td>";
        html += "<td align='right'>" + WF::fmt(l.threshold) + "</td>";
        html += "<td>" + l.status.toHtmlEscaped() + "</td></tr>";
    }
    html += "</table>";

    QPdfWriter writer(path);
    writer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Landscape,
                                     QMarginsF(12, 12, 12, 12), QPageLayout::Millimeter));
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);
    statusBar()->showMessage("PDF exporté : " + path, 6000);
}

// ---------------------------------------------------------------------------
//  Tableau de bord : alerte, niveaux de stock, graphique, prévision
// ---------------------------------------------------------------------------
void MainWindow::refreshDashboard() {
    const QVector<Lot> &lots = m_model->lots();
    const QStringList &prods = WF::products();

    // nombre de lots du jour
    int today = 0;
    for (const Lot &l : lots)
        if (l.dateTime.date() == QDate::currentDate())
            ++today;
    m_subtitle->setText(QString("Responsable production · %1 lot%2 aujourd'hui")
                            .arg(today).arg(today > 1 ? "s" : ""));

    // niveau de stock par produit = stock du dernier lot de ce produit
    QVector<int> pct(prods.size(), 0), stock(prods.size(), 0);
    for (int i = 0; i < prods.size(); ++i) {
        const Lot *last = nullptr;
        for (const Lot &l : lots) {
            if (l.product != prods[i])
                continue;
            if (!last || l.dateTime > last->dateTime ||
                (l.dateTime == last->dateTime && l.id > last->id))
                last = &l;
        }
        stock[i] = last ? last->stock : 0;
        pct[i] = qRound(100.0 * stock[i] / WF::maxStock(prods[i]));

        const QString color = pct[i] < 20 ? "#e74c3c" : (pct[i] < 50 ? "#f39c12" : "#2ecc71");
        m_bars[i]->setValue(qBound(0, pct[i], 100));
        m_bars[i]->setStyleSheet(QString("QProgressBar{background:#d9e4ec;border:none;border-radius:4px;}"
                                         "QProgressBar::chunk{background:%1;border-radius:4px;}").arg(color));
        m_pctLabels[i]->setText(QString::number(pct[i]) + "%");
    }

    // bandeau d'alerte
    QStringList low;
    for (int i = 0; i < prods.size(); ++i)
        if (pct[i] < WF::kAlertPercent)
            low << lowerFirst(prods[i]) + " à " + QString::number(pct[i]) + " %";
    if (!low.isEmpty()) {
        m_banner->setText("<b>Stock bas :</b> " + low.join(", ") + " du stock maximal, sous le seuil de " +
                          QString::number(WF::kAlertPercent) + " %. Ordre de production recommandé.");
        m_banner->setStyleSheet("background:#fdeeee; color:#0b3d73; border-left:4px solid #e74c3c;"
                                "border-radius:3px; padding:8px 10px;");
    } else {
        m_banner->setText("<b>Stocks :</b> tous les produits sont au-dessus du seuil de " +
                          QString::number(WF::kAlertPercent) + " %.");
        m_banner->setStyleSheet("background:#eafaf1; color:#0b3d73; border-left:4px solid #2ecc71;"
                                "border-radius:3px; padding:8px 10px;");
    }

    // production par jour (5 derniers jours avec production), en milliers
    QMap<QDate, int> perDay;
    for (const Lot &l : lots)
        perDay[l.dateTime.date()] += l.produced;
    static const char *dayNames[] = {"", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};
    QStringList labels;
    QVector<double> values;
    const QList<QDate> days = perDay.keys();
    for (int i = qMax(0, days.size() - 5); i < days.size(); ++i) {
        labels << dayNames[days[i].dayOfWeek()];
        values << perDay[days[i]] / 1000.0;
    }
    m_chart->setData(labels, values);

    // prévision : moyenne mobile de la demande sur 7 jours
    double sum = 0;
    for (int d : kDemand7d)
        sum += d;
    const int avg = qRound(sum / kDemand7d.size());
    m_forecastText->setText("Moyenne mobile de la demande sur 7 jours : <b>" + WF::fmt(avg) + "</b> unités/jour.");

    int worst = 0;
    for (int i = 1; i < prods.size(); ++i)
        if (pct[i] < pct[worst])
            worst = i;
    if (pct[worst] < WF::kReorderPercent) {
        // objectif : remonter à 50 % du max + couvrir 48 h de la part de demande du produit
        const int deficit = qMax(0, WF::maxStock(prods[worst]) / 2 - stock[worst]);
        const int extra = avg * 2 / prods.size();
        const int qty = int(std::ceil((deficit + extra) / 1000.0)) * 1000;
        m_recoLabel->setText("Recommandation : produire <b>" + WF::fmt(qty) + "</b> unités (" +
                             prods[worst].toHtmlEscaped() + ") d'ici 48 h.");
    } else {
        m_recoLabel->setText("Aucune production urgente : tous les stocks dépassent " +
                             QString::number(WF::kReorderPercent) + " % du maximum.");
    }
}
