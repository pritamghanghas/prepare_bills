#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariantMap>
#include <QSettings>

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

private:
    void loadSettings();
    void saveSetttings();
    void processFiles();
    void processDateFiles(QString &date, QStringList &filetoProcess);
    void printpdf();
    void printCustomerBill(const QDate &date, QVariantList sameCustomerList);
//    void billHtmlSave();
    void delay(int seconds);
    void writeTotalTxt();

    Ui::MainWindow *ui;


    QVariantMap dateToSold;

    int totalSold, totalTaxCollected;

    QMap<QString, QStringList> dateToFiles;

    QSettings *m_settings;
};

#endif // MAINWINDOW_H
