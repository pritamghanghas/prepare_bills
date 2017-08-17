#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QSettings>


class ItemEntries
{
public:
    QString itemName;
    int quantity;
    double price;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_start_button_clicked();


    void on_invoice_button_clicked();

    void on_signature_button_clicked();

    void on_checkBox_toggled(bool checked);

    void on_manualGenerateButton_clicked();

    void on_addItemButton_clicked();

private:
    void loadSettings();
    void saveSetttings();
    void processFiles();
    void processDateFiles(QString &date, QStringList &filetoProcess);
    void printpdf();
    void printCustomerBill(const QDate &date, QVariantList sameCustomerList);
    void billHtmlSave(const QDate &date, QString htmlBillingAddress,
                     QList<ItemEntries> &itemDescriptions, double cgstTaxRate, double sgstTaxRate, double shipping = 0);
    void delay(int seconds);
    void writeTotalTxt();

    Ui::MainWindow *ui;


    QVariantMap dateToSold;

    int totalSold, totalTaxCollected;

    QMap<QString, QStringList> dateToFiles;

    QList<ItemEntries> m_manualItemDescs;

    QSettings *m_settings;
};



#endif // MAINWINDOW_H
