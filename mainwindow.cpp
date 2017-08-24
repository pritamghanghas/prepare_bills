#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QDebug>
#include <QFile>
#include <QDate>
#include <QTextDocument>
#include <QPrinter>
#include <QtWebKit>
//#include <QWebView>
//#include <QWebEngineView>

const QString itemLine("<tr class=\"item\"> <td>$itemNamePlaceholder</td> <td>$price</td> <td>$quantity</td> <td>$total</td> </tr>\n");
const QString shippingLine("<tr class=\"shipping\"> <td></td> <td></td> <td>Shipping Charges:</td> <td> $shipping </td> </tr>");
const QString billNumberPrefix = "E";
const double TAX_RATE(5.25);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    totalSold = 0;
    totalTaxCollected = 0;
    ui->billDateEdit->setDate(QDate::currentDate());
    on_checkBox_toggled(false);

    m_settings = new QSettings("hoverbirds", "invoice", this);
    loadSettings();
}

void MainWindow::loadSettings()
{
    QString defaultPath  = "/home";
    ui->email_path_edit->setText(m_settings->value("mail-directory", defaultPath).toString());
    ui->invoice_template_edit->setText(m_settings->value("invoice-template", defaultPath).toString());
    ui->signature_path_edit->setText(m_settings->value("signature-file", defaultPath).toString());
    ui->start_number_edit->setText(m_settings->value("invoice_number", "1").toString());
    ui->cgst_field->setText(m_settings->value("cgst_tax_rate", "9").toString());
    ui->sgst_field->setText(m_settings->value("sgst_tax_rate", "9").toString());
    ui->gst_number->setText(m_settings->value("gstin").toString());
    ui->companyName->setText(m_settings->value("company_name").toString());
}

void MainWindow::saveSetttings()
{
    m_settings->setValue("mail-directory", ui->email_path_edit->text());
    m_settings->setValue("invoice-template", ui->invoice_template_edit->text());
    m_settings->setValue("signature-file", ui->signature_path_edit->text());
    m_settings->setValue("invoice_number", ui->start_number_edit->text());
    m_settings->setValue("cgst_tax_rate", ui->cgst_field->text());
    m_settings->setValue("sgst_tax_rate", ui->sgst_field->text());
    m_settings->setValue("company_name", ui->companyName->text());
}

MainWindow::~MainWindow()
{
    saveSetttings();
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    ui->email_path_edit->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);

    qDebug() << "dir selected" << dir;
    ui->email_path_edit->setText(dir);
}

void MainWindow::on_start_button_clicked()
{
    // clean up old state if any
    dateToSold.clear();
    totalSold = 0;
    totalTaxCollected = 0;
    dateToFiles.clear();

    QDir dir(ui->email_path_edit->text());

    dir.setSorting(QDir::Name);
    QStringList nameFilters;
    nameFilters << "*.txt";
    QStringList allFiles = dir.entryList(nameFilters);

    Q_FOREACH(QString filename, allFiles) {
        QString dateString = filename.split("-").first();
        dateToFiles[dateString] << ui->email_path_edit->text() + "/" + filename;
    }

    processFiles();
}

void MainWindow::processFiles()
{
    Q_FOREACH (QString date, dateToFiles.keys()) {
        qDebug() << "processing date: " << date;
    }

    Q_FOREACH (QString date, dateToFiles.keys()) {
        qDebug() << "processing files: " << dateToFiles.value(date);
    }


    Q_FOREACH (QString date, dateToFiles.keys()) {
        QStringList filesToProcess = dateToFiles.value(date);
        processDateFiles(date, filesToProcess);
    }

    printpdf();

    ui->value_label->setText(QString(ui->value_label->text()).arg(totalSold).arg(totalTaxCollected));

    writeTotalTxt();
}


void MainWindow::writeTotalTxt()
{
     QFile totalFile(ui->email_path_edit->text() + "/pdfs/totals.txt");

     if (!totalFile.open(QFile::ReadWrite)) {
         qDebug("failed to open file for  writing");
         return;
     }

     QTextStream out(&totalFile);
     out << "Total sold including tax: " << totalSold << "\n";
     out << "Total tax collected: " << totalTaxCollected << "\n";

}


void MainWindow::processDateFiles(QString &date, QStringList &filetoProcess)
{
    QVariantMap customerIndexedSold;
    Q_FOREACH ( const QString &file, filetoProcess)
    {
        QFile newFile(file);
        qDebug() << "trying to openfile" << file;
        if (!newFile.open(QFile::ReadOnly)) {
            qDebug("something wrong with file");
            return;
        }

        QVariantMap customerMap;
        QString line = newFile.readLine();
        bool foundAddress = false;
        QStringList address;
        while (!newFile.atEnd()) {
            if (line.contains("You should always leave feedback")) {
                line = newFile.readLine();
                line = newFile.readLine().trimmed();
                qDebug() << "item name: " << line;
                customerMap.insert("item", line);
                continue;
            }
            if (line.contains("Sale price")) {
                QString price = line.split("Rs. ").last().trimmed();
                qDebug() << "price: " << price;
                customerMap.insert("price", price);
            }

            if (line.contains("Quantity sold")) {
                QString quantity = line.split(":").last().trimmed();
//                quantity = quantity.trimmed();
                qDebug() << "quantity: " << quantity;
                customerMap.insert("quantity", quantity);
            }

            if (line.contains("Buyer's shipping address:")) {
                foundAddress = true;
                line = newFile.readLine();
            }

            if (foundAddress) {
                while(!line.contains("Sell another Item")) {
                    address << line.trimmed();
                    line = newFile.readLine();
                }
                qDebug() << "address: " << address;
                customerMap.insert("address", address);
                if (!customerIndexedSold.contains(address.first())) {
                        QVariantList sameCustomerList;
                        sameCustomerList << QVariant(customerMap);
                        customerIndexedSold.insert(address.first(), sameCustomerList);
                } else {
                    QVariantList sameCustomerList = customerIndexedSold.value(address.first()).toList();
                    sameCustomerList <<  QVariant(customerMap);
                    customerIndexedSold.insert(address.first(), sameCustomerList);
                }

                customerMap.clear();
                address.clear();
                foundAddress = false;
            }

            line = newFile.readLine();
            qDebug() << "still looping through";
        }
    }
    dateToSold.insert(date, customerIndexedSold);
    qDebug() << "\n\n\n==========================================\n\n\n";
    qDebug() << dateToSold;
}

void MainWindow::printpdf()
{
//    QDir targetDir = QDir(ui->email_path_edit->text() + "/pdfs");
//    if (!targetDir.exists()) {
//        targetDir.mkpath(targetDir.path());
//    }

    Q_FOREACH(QVariant date, dateToSold.keys()) {
        QDate billDate = QDate::fromString(date.toString(), "yyyyMMdd");
        qDebug() << "string date: " << date.toString() << "date object: " << billDate;
        QVariantMap customerMap = dateToSold.value(date.toString()).toMap();
        Q_FOREACH(QVariant biller, customerMap.keys()) {
            QString billerName = biller.toString();
            QVariantList sameCustomerList = customerMap.value(biller.toString()).toList();
            printCustomerBill(billDate, sameCustomerList);
        }
    }
}

void MainWindow::printCustomerBill(const QDate &date, QVariantList sameCustomerList)
{
    QString billingAddress;
    QList<ItemEntries> itemDescriptions;
    Q_FOREACH(const QVariant itemData, sameCustomerList) {
        QVariantMap itemDetails = itemData.toMap();
        if (billingAddress.isEmpty()) {
            QStringList billingStringList = itemDetails.value("address").toStringList();
            Q_FOREACH (const QString line, billingStringList) {
                billingAddress += line;
                billingAddress += "<br>";
            }
        }

        ItemEntries itemDesc;

        itemDesc.itemName = itemDetails.value("item").toString();

        itemDesc.quantity = itemDetails.value("quantity").toInt();

        itemDesc.price  = itemDetails.value("price").toString().replace(",", "").toDouble();

        itemDescriptions << itemDesc;
    }

    billHtmlSave(date,billingAddress,itemDescriptions, ui->cgst_field->text().toFloat(), ui->sgst_field->text().toFloat());
}

void MainWindow::billHtmlSave(const QDate &date, QString htmlBillingAddress,
                              QList<ItemEntries> &itemDescriptions, double cgstTaxRate, double sgstTaxRate, double shipping)
{
    QFile file(ui->invoice_template_edit->text());
    if(!file.open(QFile::ReadOnly)) {
        qDebug() << "something wrong with html template file";
        return;
    }

    QString htmlString = file.readAll();

    htmlString.replace("$date", date.toString("MMMM d, yyyy"));
    htmlString.replace("$billing_address", htmlBillingAddress);

    double subtotal = 0;
    QString htmlItemLine;

    Q_FOREACH(const ItemEntries &itemDesc, itemDescriptions) {

        // convert price including tax to price without tax
        double price = itemDesc.price/(1.0+(cgstTaxRate+sgstTaxRate)/100);

        double total = price*itemDesc.quantity;

        subtotal += total;

        htmlItemLine += itemLine;
        htmlItemLine.replace("$itemNamePlaceholder", itemDesc.itemName);
        htmlItemLine.replace("$quantity", QString("%1").arg(itemDesc.quantity));
        htmlItemLine.replace("$price", QString("%1").arg(price, 0, 'f', 2));
        htmlItemLine.replace("$total", QString("%1").arg(total, 0, 'f', 2));

    }


    double cgst_taxes = subtotal*(cgstTaxRate/100);
    double sgst_taxes = subtotal*(sgstTaxRate/100);
    double grandTotal = subtotal + cgst_taxes + sgst_taxes;

    htmlString.replace("$company_name", ui->companyName->text());
    htmlString.replace("$gstin", ui->gst_number->text());
    htmlString.replace("$items_tags", htmlItemLine);
    htmlString.replace("$subtotal", QString("%1").arg(subtotal, 0, 'f', 2));
    htmlString.replace("$cgstTaxRate", QString("%1%").arg(cgstTaxRate, 0, 'f', 2));
    htmlString.replace("$sgstTaxRate", QString("%1%").arg(sgstTaxRate, 0, 'f', 2));
    htmlString.replace("$cgst_taxes", QString("%1").arg(cgst_taxes, 0, 'f', 2));
    htmlString.replace("$sgst_taxes", QString("%1").arg(cgst_taxes, 0, 'f', 2));


    // shipping is a special case and is used only when shipping is non zero
    QString shippingChargesLine;
    if(shipping) {
        shippingChargesLine = shippingLine;
        shippingChargesLine.replace("$shipping", QString("%1").arg(shipping));
    }
    htmlString.replace("$shipping_line", shippingChargesLine);

    htmlString.replace("$grandtotal", QString("%1").arg(grandTotal+shipping, 0, 'f', 2));
    htmlString.replace("$signature", "file://" + ui->signature_path_edit->text());
    htmlString.replace("$invoice_number", billNumberPrefix + ui->start_number_edit->text());

    // udate session totals
    totalSold += grandTotal;
    totalTaxCollected += cgst_taxes;
    totalTaxCollected += sgst_taxes;

    QDir targetDir = QDir(ui->email_path_edit->text() + "/pdfs");
    if (!targetDir.exists()) {
        qDebug() << "creating dir: " << targetDir.path();
        targetDir.mkpath(targetDir.path());
    }

    QString printFile = ui->email_path_edit->text() + "/pdfs/bill_no_" + billNumberPrefix + ui->start_number_edit->text() + ".pdf";
    qDebug() << "pdf file name: " << printFile;


//#if 0
    // webview printing
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFileName(printFile);

    // Create webview and load html source
    QWebView webview;
    webview.setHtml(htmlString);
    webview.show();
    delay(1);

    // Create PDF
    webview.print(&printer);
//#endif

#if 0 // webengine printing
    QWebEngineView webview;
    webview.setHtml(htmlString);
    webview.show();
//    delay(4);
    webview.page()->printToPdf(printFile);
#endif

#if 0 //html file printing
    QString htmlFile = ui->email_path_edit->text() + "/pdfs/bill_no_" + ui->start_number_edit->text() + ".html";

    QFile fileToWrite(htmlFile);
    if (!fileToWrite.open(QFile::ReadWrite)) {
        qDebug() << "can't open file for writing" << htmlFile;
    }
    fileToWrite.write(htmlString.toStdString().c_str());
#endif


    //upade bill number in prepartion for next bill
    int currentBillNo = ui->start_number_edit->text().toInt();
    currentBillNo++;
    ui->start_number_edit->setText(QString("%1").arg(currentBillNo));
}


void MainWindow::on_invoice_button_clicked()
{
    QString invoice_template = QFileDialog::getOpenFileName(this,
                                                          tr("Open Image"),
                                                          ui->invoice_template_edit->text(),
                                                          "*.html");

    qDebug() << "invoice template selected" << invoice_template;
    ui->invoice_template_edit->setText(invoice_template);
}

void MainWindow::on_signature_button_clicked()
{
    QString signature = QFileDialog::getOpenFileName(this,
                                                          tr("Open Image"),
                                                          ui->signature_path_edit->text(),
                                                          tr("Image Files (*.png *.jpg *.bmp)"));

    qDebug() << "signature selected" << signature;
    ui->signature_path_edit->setText(signature);
}

void MainWindow::delay(int seconds)
{
    QTime dieTime= QTime::currentTime().addSecs(seconds);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::on_checkBox_toggled(bool checked)
{
   ui->itemNameField->setEnabled(checked);
   ui->quantityField->setEnabled(checked);
   ui->priceField->setEnabled(checked);
   ui->addressField->setEnabled(checked);
   ui->manualGenerateButton->setEnabled(checked);
   ui->addItemButton->setEnabled(checked);
   ui->clear_item_button->setEnabled(checked);
   ui->shipping_edit->setEnabled(checked);
   ui->billDateEdit->setEnabled(checked);
}

void MainWindow::on_manualGenerateButton_clicked()
{
    if (m_manualItemDescs.isEmpty()) {
        ui->manualBillStatus->setText("no items have been added to this manual bill");
        return;
    }

    if (ui->addressField->toPlainText().isEmpty()) {
        ui->manualBillStatus->setText("address filed is empty");
        return;
    }


    qDebug() << "generating manual bill";

    QString billingAddress = ui->addressField->toHtml();

    billHtmlSave(ui->billDateEdit->date(), billingAddress, m_manualItemDescs,
                 ui->cgst_field->text().toFloat(), ui->sgst_field->text().toFloat(), ui->shipping_edit->text().toFloat());

    ui->manualBillStatus->setText("manual bill generated and saved");
}

void MainWindow::on_addItemButton_clicked()
{
    if (ui->itemNameField->text().isEmpty() ||
            ui->quantityField->text().isEmpty() ||
            ui->priceField->text().isEmpty())
    {
        ui->manualBillStatus->setText("missing field, please enter item details and try again");
        return;
    }

    ItemEntries itemDesc;
    itemDesc.itemName = ui->itemNameField->text();
    itemDesc.quantity = ui->quantityField->text().toInt();
    itemDesc.price = ui->priceField->text().toDouble();

    m_manualItemDescs << itemDesc;

    populateItemsEdit();
}

void MainWindow::populateItemsEdit()
{
    QString itemHtml;

    Q_FOREACH (const ItemEntries itemDesc, m_manualItemDescs)
    {
        QString newLine = itemLine;

        newLine.replace("$itemNamePlaceholder", itemDesc.itemName);
        newLine.replace("$quantity", QString("%1").arg(itemDesc.quantity));
        newLine.replace("$price", QString("%1").arg(itemDesc.price));
        newLine.replace("$total", QString(""));

        itemHtml += newLine;
    }

    ui->itemsEdit->setHtml(itemHtml);
}

void MainWindow::on_clear_item_button_clicked()
{
    m_manualItemDescs.takeLast();

    populateItemsEdit();
}
