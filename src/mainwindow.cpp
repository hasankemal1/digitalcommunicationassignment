#include <ui_mainwindow.h>
#include "mainwindow.h"
#include <QFileDialog>
#include <QDir>
#include "readfile.h"
#include <fft.h>       // Header file for fft class
#include <fftworker.h> // Header file for FFTWorker class
#include <iostream>
#include <QMouseEvent>
#include <QMessageBox>
#include <rawdatatowavfile.h> // Header file for rawdatatowavfile function/class
#include <struct.h>           // Required struct definitions (sampleData etc.)
#include "qcustomplot.h"      // Required for QCustomPlot
#include <algorithm>          // Added for std::copy
#include <cmath>              // For std::sqrt and std::abs
#include <Filter1.h>

#include <QMediaDevices>
#include <QAudioDevice>
#include <QtMultimedia/qaudioinput.h>
#include <circular.h>
QString greenStyle = R"(
QProgressBar::chunk {
    background-color: #00FF00;
}
)";
QString redStyle = R"(
QProgressBar::chunk {
    background-color: #FF0000;
}
)";
struct Peak {
    double magnitude;
    size_t index;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fftWorker(nullptr)
    , filterWorker(nullptr)
    , workerThread(nullptr)
    , progressDialog(nullptr)
    , mPositionMarker(nullptr)
    , m_coordText(nullptr)
    , mouse_x(-1.0)      // mouse_x initialized
    , mouse_x1(-1.0)     // mouse_x1 initialized
    , m_triggerFilterProcessing_Button7(false)
    , m_filterTypeForButton7Sequence(0)
	, realtimeWorker(nullptr)
{
    flag = false;
    flag2 = false;
    ui->setupUi(this);
    oke = false;
    oke2 = false;
    ui->label->setText("No directory selected");
    flag3 = false;
   
    QObject::connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::pushbutton);
    QObject::connect(ui->comboBox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        ui->label->setText("Selected file: " + text);
    });
    QObject::connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::pushbutton2);
    QObject::connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::pushbutton4);
    QObject::connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::pushbutton3);
    QObject::connect(ui->pushButton_5, &QRadioButton::clicked, this, &MainWindow::pushbutton5);
    QObject::connect(ui->pushButton_6, &QPushButton::clicked, this, &MainWindow::pushbutton6);
    QObject::connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::pushbutton7);
    QObject::connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::pushbutton8);
    QObject::connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::pushbutton9);
    QObject::connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::pushbutton10);
    QObject::connect(ui->pushButton_11, &QPushButton::clicked, this, &MainWindow::pushbutton11);
    workerThread = new QThread(this);

    fftWorker = new FFTWorker();
    fftWorker->moveToThread(workerThread);
    connect(fftWorker, &FFTWorker::finished, this, &MainWindow::onFFTFinished);
    connect(workerThread, &QThread::finished, fftWorker, &QObject::deleteLater);

    filterWorker = new Filter1();
    filterWorker->moveToThread(workerThread);
    connect(filterWorker, &Filter1::finished, this, &MainWindow::onFFTFinished);
    connect(filterWorker, &Filter1::spectrum, this, &MainWindow::spectrum);


	realtimeWorker = new realtime();
	realtimeWorker->moveToThread(workerThread);
    connect(realtimeWorker, &realtime::datastream, this, &MainWindow::realtimeFinished);
    connect(ui->pushButton_11, &QPushButton::clicked, realtimeWorker, &realtime::stop);

    connect(workerThread, &QThread::finished, filterWorker, &QObject::deleteLater);

    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    connect(ui->custom, &QCustomPlot::mousePress, this, &MainWindow::onMousePress);
    connect(ui->custom, &QCustomPlot::mouseMove, this, &MainWindow::onMousePress);
    ui->custom->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    workerThread->start();
    m_coordText = new QCPItemText(ui->custom);
    m_coordText->setLayer("overlay"); 
    m_coordText->setPen(QPen(Qt::black));
    m_coordText->setPositionAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_coordText->setPadding(QMargins(5, 2, 5, 2)); 
    m_coordText->setVisible(false); 
    if (ui->label_9) { 
       
        ui->label_9->setAlignment(Qt::AlignCenter); 

       
        QFont font = ui->label_9->font();
        font.setPointSize(36); 
        font.setBold(true);
        ui->label_9->setFont(font);

        
        ui->label_9->setStyleSheet(
            "QLabel {"
            "  background-color: #2C3E50;"
            "  color: #2ECC71;"           
            "  border: 2px solid #34495E;"
            "  border-radius: 8px;"       
            "  padding: 10px;"            
            "}"
        );
    
        ui->label_9->clear();
        ui->label_9->setVisible(false);
      
        QString filePath;

        
        QString appPath = QCoreApplication::applicationDirPath();
        qDebug() << "Uygulama çalýþma dizini (QCoreApplication::applicationDirPath()):" << appPath;

        QDir appDir(appPath);
        QString resourceSubDirName = "resources"; 
        qDebug() << "Kaynak alt dizin adý:" << resourceSubDirName;

        QString resourceDirPath = appDir.filePath(resourceSubDirName);
        qDebug() << "Hesaplanan kaynak dizin yolu (resourceDirPath):" << resourceDirPath;

        
        QDir checkResourceDir(resourceDirPath);
        if (!checkResourceDir.exists()) {
            qDebug() << "Kaynak dizini (" << resourceSubDirName << ") bulunamadý veya geçerli deðil:" << resourceDirPath;
            
        }

        filePath = QDir(resourceDirPath).filePath("a.csv");
        qDebug() << "CSV dosyasý için hedeflenen tam yol (filePath):" << filePath;

        QFile file(filePath);
        if (!file.exists()) {
            qDebug() << "CSV dosyasý bulunamadý:" << filePath;
            QMessageBox::critical(this, tr("Dosya Hatasý"), tr("CSV dosyasý bulunamadý: '%1'").arg(filePath));
            
            return;
        }

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "CSV dosyasý açýlamadý:" << filePath << " Hata:" << file.errorString();
            QMessageBox::critical(this, tr("Dosya Hatasý"), tr("CSV dosyasý '%1' açýlamadý: %2").arg(filePath, file.errorString()));
            return;
        }

        QTextStream in(&file);
        qDebug() << "CSV Dosyasý Okunuyor:" << filePath;

        QStringList headerFields; 
        if (!in.atEnd()) {
            QString headerLine = in.readLine();
            headerFields = headerLine.split(';'); 
            qDebug() << "Baþlýklar:" << headerFields;
        }

        
        csvData.reserve(128);
		csvData2.reserve(128); 

        int dataRowNumber = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.trimmed().isEmpty()) { 
                continue;
            }
            QStringList fields = line.split(';'); 

           
            if (fields.size() >= 3) { 
                QString freqStr = fields.at(2).trimmed();
                freqStr.replace(',', '.');
                csvData.append(fields.at(1).trimmed()); 
                csvData2.append(freqStr.toFloat());  

                ui->comboBox_4->addItem(fields.at(1).trimmed()+"-"+freqStr);
            }   
            else {
                qDebug() << "Uyarý: Satýr" << dataRowNumber + 1 << "beklenenden az sütun içeriyor (" << fields.size() << "sütun bulundu). Atlama yapýlýyor olabilir.";
              
            }
            dataRowNumber++;
        }
        file.close();

        qDebug() << "CSV okuma tamamlandý. Toplam" << dataRowNumber << "veri satýrý iþlendi.";
        qDebug() << "Ýlk sütundan okunan deðer sayýsý:" << csvData.size();
        qDebug() << "Ýkinci sütundan okunan deðer sayýsý:" << csvData2.size();

        
        for (int i = 0; i < 5; ++i) {
            qDebug() << "Satýr" << i + 1 << "- Sütun 1:" << csvData.at(i) << ", Sütun 2:" << csvData2.at(i);
        }

      
    }
    QList<QAudioDevice> inputDevicesInfo =
		QMediaDevices::audioInputs();
    for (const QAudioDevice device : inputDevicesInfo) {

        ui->comboBox_5->addItem(device.description(), QVariant::fromValue(device));
    }   


  
           
    
    
    
        qDebug() << "Audio input devices loaded into comboBox_5.";
        
        realtimePlot = new QCustomPlot(ui->custom5);
        realtimePlot->setAntialiasedElements(QCP::aeNone);
        realtimePlot->setFixedSize(340, 290);
        realtimePlot->setWindowTitle("FFT Amplitude Spectrum");
        realtimePlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        realtimePlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
        realtimePlot->axisRect()->setRangeZoomFactor(0.9);  // daha yumuþak zoom

		realtimePlot2 = new QCustomPlot(ui->custom6);
		realtimePlot2->setFixedSize(340, 290);
		realtimePlot2->setWindowTitle("time");
        freq_resolution12 = 48000 / 24000;
		
		
       
}


    void MainWindow::pushbutton10()
    { 
		float note = ui->comboBox_4->currentText().split("-").last().toFloat();
		qDebug() << "Selected note frequency:" << note;
        QVariant  audioDevice  = ui->comboBox_5->currentData();
        
        QMetaObject::invokeMethod(realtimeWorker, "init", Qt::QueuedConnection,
            Q_ARG(QVariant , audioDevice),
            Q_ARG(float, note));

    }

    void MainWindow::realtimeFinished(const std::vector<double>& result, const std::vector<double>& time, const std::vector<double>& firres, unsigned int fftSizeUsed) {
        if (result.empty()) {
            qDebug() << "FFT sonucu boþ geldi!";
            return;
        }

        double note = ui->comboBox_4->currentText().split("-").last().toFloat();
        QVector<double> xData;
        QVector<double> yData;
        xData.resize(result.size());
        yData.resize(result.size());
        if (ui->radioButton_8->isChecked()) {

            for (int i = 0; i < result.size() / 24; ++i) {
                xData[i] = static_cast<double>(i) * freq_resolution12;
                yData[i] = result[i] * firres[i];
            }

            auto x = *std::max_element(yData.begin(), yData.end());
            auto y = std::distance(yData.begin(), std::max_element(yData.begin(), yData.end())) * freq_resolution12;
            qDebug() << "Max value in yData:" << x << "index" << y;

            std::vector<Peak> peaks;
            for (size_t i = 0; i < yData.size(); ++i) {
                peaks.push_back({ yData[i], i });
            }
            // Magnitude'a göre azalan sýrala
            std::sort(peaks.begin(), peaks.end(), [](const Peak& a, const Peak& b) {
                return a.magnitude > b.magnitude;
                });

            // Ýlk üç tepeyi al
            for (int i = 0; i < 3 && i < static_cast<int>(peaks.size()); ++i) {
                double freq = peaks[i].index * freq_resolution12;
                double mag = peaks[i].magnitude;
                qDebug() << "Peak" << i + 1 << ": Frequency =" << freq << "Hz, Magnitude =" << mag;
            }
            double t = std::abs(note - peaks[0].index * freq_resolution12 );
            int index = 0;
            for (int i = 0; i < 3; i++) {
                double temp = std::abs(note - peaks[i].index * freq_resolution12 );


                if (temp <= t) {
                    t = temp;
					index = peaks[i].index * freq_resolution12;
                }
            }
			qDebug() << "Minimum frequency difference from note:" << t<<"-"<<index;


           
           double centError= (note - index);
              qDebug() << "cent" << centError;
              if (std::abs(centError) <= 50) {
                  if (centError <-1.50) {
                    
                  
                      ui->progressBar_2->setValue(0);
                      ui->progressBar->setStyleSheet(greenStyle);
                      ui->label_10->setStyleSheet(redStyle);
                      ui->progressBar->setValue(std::abs(centError+1.5));
                  }
                  else if (centError > 1.5) {
                      
                      ui->progressBar->setValue(0);
                      ui->progressBar_2->setStyleSheet(redStyle);
            	ui->label_10->setStyleSheet(redStyle);
                       ui->progressBar_2->setValue(std::abs(centError-1.5));
                   }
                   else if (centError <= 1.5 || centError>=-1.5 ) {
                       ui->label_10->setAutoFillBackground(true);
                       QPalette palette = ui->label_10->palette();
                       palette.setColor(QPalette::Window, QColor(Qt::green));
                       ui->label_10->setPalette(palette);
                       ui->progressBar->setValue(0);
                       ui->progressBar_2->setValue(0);
            
                   }
                  
            
               }
               else {
                   ui->pushButton_11->click();
                   QMessageBox::warning(this, "Note Mismatch", "The detected frequency is significantly different from the selected note. Please check your audio input.");
                  
                   
               
               }
        }
       
        else {
           
            for (int i = 0; i < result.size(); ++i) {
                xData[i] = static_cast<double>(i) * freq_resolution12;
                yData[i] = result[i];
            }
        }
        realtimePlot->clearGraphs();
        realtimePlot->addGraph();
        realtimePlot->graph()->addData(xData, yData);
        realtimePlot->xAxis->setLabel("Frequency (Hz)");
        realtimePlot->yAxis->setLabel("Amplitude");
        realtimePlot->rescaleAxes();
        realtimePlot->replot(QCustomPlot::rpQueuedReplot);
        
		
        QVector<double> txData(time.size());
        QVector<double> tyData(time.begin(), time.end());
        for (int i = 0; i < time.size(); ++i) {
            txData[i] = static_cast<double>(i) / 24000;
            

        }
    
        realtimePlot2->clearGraphs();
        realtimePlot2->addGraph();
        realtimePlot2->graph()->addData(txData, tyData);
        realtimePlot2->xAxis->setLabel("Time (s)"); // Changed label from "time (Hz)" to "Time (s)"
        realtimePlot2->yAxis->setLabel("Amplitude");
        realtimePlot2->rescaleAxes();
        realtimePlot2->replot(QCustomPlot::rpQueuedReplot);
    
    
   
    
    
    
    
    }



   void  MainWindow::pushbutton11()
    {
       
	  
    }


MainWindow::~MainWindow()
{
    if (workerThread && workerThread->isRunning()) {
        workerThread->quit();
        workerThread->wait();
    }

    if (progressDialog) {
        progressDialog->close();
        delete progressDialog;
        progressDialog = nullptr;
    }
    if (flag2) {
       
    }
    delete ui;
}
void MainWindow::pushbutton6() {

    flag2 = true;

    pushbutton();

}
void MainWindow::pushbutton8() {
    flag3 = true;
    pushbutton6();
}

void MainWindow::pushbutton9() {
    flag3 = true; 
    pushbutton7();
}

void MainWindow::pushbutton7()
{
    flag2 = true; 
    
    m_triggerFilterProcessing_Button7 = false;
    m_filterTypeForButton7Sequence = 0; 

   
    if (flag3) { 
        m_filterTypeForButton7Sequence = 24;
    }
    else { 
        m_filterTypeForButton7Sequence = 23;
    }

    
    pushbutton2();

    
    if (!oke) {
        QMessageBox::warning(this, "Operation Failed", "Could not initiate audio processing for filtering. Please select a valid audio file first or check data.");
        flag2 = false;
        m_filterTypeForButton7Sequence = 0;
    }
    
}

void MainWindow::spectrum(const std::vector<float>& result, int inverse, unsigned int fftSizeUsed) {
    if (inverse == 31) {
        double a = 0;
        for (size_t i = 0; i < (result.size() / 2); ++i) {
            if ((2 * i + 1) < result.size()) { 
                double re = result[2 * i];     
                double im = result[2 * i + 1]; 
                a += std::sqrt(std::pow(re, 2.0) + std::pow(im, 2.0));
            }
        }
        double b = 0;
       
        for (size_t i = 0; i < fftData.size() / 2; ++i) {
            if ((2 * i + 1) < fftData.size()) { 
                double re = fftData[2 * i];     
                double im = fftData[2 * i + 1]; 
                b += std::sqrt(std::pow(re, 2.0) + std::pow(im, 2.0));
            }
        }
        qDebug() << "Original total magnitude (b):" << b << "Filtered total magnitude (a):" << a;
        double ratio = (a != 0) ? (b - a) / a : (b == 0 ? 0.0 : -1.0); 

        qDebug() << "Ratio:" << ratio;
        if (ratio < 15 && ratio >= 0) { 
            ui->label_3->setAutoFillBackground(true);
            QPalette palette = ui->label_3->palette();
            palette.setColor(QPalette::Window, QColor(Qt::green));
            ui->label_3->setPalette(palette);
            ui->label_4->setAutoFillBackground(true);
            QPalette palette2 = ui->label_4->palette();
            palette2.setColor(QPalette::Window, QColor(Qt::red));
            ui->label_4->setPalette(palette2);
        }
        else {
            ui->label_4->setAutoFillBackground(true);
            QPalette palette = ui->label_4->palette();
            palette.setColor(QPalette::Window, QColor(Qt::green));
            ui->label_4->setPalette(palette);
            ui->label_3->setAutoFillBackground(true);
            QPalette palette2 = ui->label_3->palette();
            palette2.setColor(QPalette::Window, QColor(Qt::red));
            ui->label_3->setPalette(palette2);
        }
    }
    else if (inverse == -2) {
        float tol = 20.0f; // Tolerans artýrýldý
        const std::vector<int> row_freqs = { 697, 770, 852, 941 };
        const std::vector<int> col_freqs = { 1209, 1336, 1477 };
         std::vector<std::vector<char>> dtmf_lut = {
            {'1', '2', '3'},
            {'4', '5', '6'},
            {'7', '8', '9'},
            {'*', '0', '#'}
        };

        

        int row = -1; // Match not found indicator
        int col = -1; // Match not found indicator

      
        for (size_t i = 0; i < row_freqs.size(); ++i) {
            if (std::abs(result[0] - row_freqs[i]) < tol) {
                row = static_cast<int>(i);
                break;
            }
        }
        for (size_t j = 0; j < col_freqs.size(); ++j) {
            if (std::abs(result[1] - col_freqs[j]) < tol) {
                col = static_cast<int>(j);
                break;
            }
        }

        // If the above order didn't work, try swapping frequencies
        if (row == -1 || col == -1) {
            qDebug() << "DTMF: Initial pass failed (row=" << row << ", col=" << col << "). Trying swapped frequencies.";
            int temp_row = -1;
            int temp_col = -1;
            // Try matching result[1] as row_freq and result[0] as col_freq
            for (size_t i = 0; i < row_freqs.size(); ++i) {
                if (std::abs(result[1] - row_freqs[i]) < tol) {
                    temp_row = static_cast<int>(i);
                    break;
                }
            }
            for (size_t j = 0; j < col_freqs.size(); ++j) {
                if (std::abs(result[0] - col_freqs[j]) < tol) {
                    temp_col = static_cast<int>(j);
                    break;
                }
            }
            if (temp_row != -1 && temp_col != -1) {
                row = temp_row;
                col = temp_col;
                qDebug() << "DTMF: Swapped match successful (row=" << row << ", col=" << col << ").";
            }
            else {
                qDebug() << "DTMF: Swapped match also failed (temp_row=" << temp_row << ", temp_col=" << temp_col << ").";
            }
		
        }
        if (ui->label_9) { // Yeni QLabel'ý baþlangýçta temizle ve gizle
            ui->label_9->clear();
            ui->label_9->setVisible(false);
        }
        if (row != -1 && col != -1) {
            if (static_cast<size_t>(row) < dtmf_lut.size() && static_cast<size_t>(col) < dtmf_lut[static_cast<size_t>(row)].size()) {
               
                QChar a = dtmf_lut[static_cast<size_t>(row)][static_cast<size_t>(col)];
                qDebug() << "Detected DTMF tone: " << dtmf_lut[static_cast<size_t>(row)][static_cast<size_t>(col)];
                if (ui->label_9) { // dtmfDisplayLabel'ýn var olduðundan emin ol
                    ui->label_9->setText(QString(a));
                }
                    ui->label_9->setMinimumSize(60, 60); // Minimum boyut (isteðe baðlý)
                    ui->label_9->setVisible(true);
				
            }
            else {
                qDebug() << "DTMF detection error: row or col index out of bounds. Row:" << row << "Col:" << col;
                qDebug() << "Received frequencies: " << result[0] << " Hz, " << result[1] << " Hz";
                if (ui->label_9) {
                    ui->label_9->setText("Err"); // Hata durumunda göster
                    ui->label_9->setVisible(true);
                }
            }
        }
        else {
            qDebug() << "DTMF tone not detected. No valid row/col pair found.";
            qDebug() << "Received frequencies: " << result[0] << " Hz, " << result[1] << " Hz";
            qDebug() << "Found row index: " << row << ", Found col index: " << col;
            if (ui->label_9) {
                ui->label_9->clear(); // Tespit yoksa temizle
                ui->label_9->setVisible(false);
            }
        }
    }
}

void MainWindow::pushbutton5() {
    if (!fftDatafiltered.empty()) {
        QVector<float> spectrumData;
        spectrumData.reserve(fftDatafiltered.size());
        for (const float& val : fftDatafiltered) {
            spectrumData.append(val);
        }

        QMetaObject::invokeMethod(fftWorker, "processFFT", Qt::QueuedConnection,
            Q_ARG(QVector<float>, spectrumData),
            Q_ARG(int, 1)); // 1 for IFFT
    }
    else {
        if (!fftWorker) {
            qDebug() << "Inverse FFT could not be started: fftWorker not initialized.";
        }
        if (fftDatafiltered.empty()) {
            qDebug() << "Inverse FFT could not be started: Forward FFT must be done first (fftDatafiltered is empty).";
            QMessageBox::warning(this, "IFFT Error", "Please calculate FFT for an audio file first.");
        }
    }
}

void MainWindow::onMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && ui->custom->axisRect()->rect().contains(event->pos()))
    {
        double clicked_x_coord = ui->custom->xAxis->pixelToCoord(event->pos().x());

        if (ui->radioButton_4->isChecked()) // Band-pass
        {
            if (!flag)
            {
                mouse_x = clicked_x_coord;
                flag = true;
                ui->radioButton_4->setText("First freq: " + QString::number(mouse_x, 'f', 2) + " - Select second point");
                qDebug() << "Band-pass: First click at" << mouse_x << ". Waiting for second click.";
            }
            else
            {
                mouse_x1 = clicked_x_coord;
                if (mouse_x > mouse_x1) { // Ensure mouse_x is lower frequency
                    std::swap(mouse_x, mouse_x1);
                }
                ui->radioButton_4->setText("Band-pass range " + QString::number(mouse_x, 'f', 2) + "-" + QString::number(mouse_x1, 'f', 2));
                flag = false;
                qDebug() << "Band-pass: Second click at" << mouse_x1 << ". Range set to" << mouse_x << "-" << mouse_x1;
            }
        }
        else if (ui->radioButton_5->isChecked()) { // Band-stop

            if (!flag)
            {
                mouse_x = clicked_x_coord;
                flag = true;
                ui->radioButton_5->setText("First freq: " + QString::number(mouse_x, 'f', 2) + " - Select second point");
                qDebug() << "Band-stop: First click at" << mouse_x << ". Waiting for second click.";
            }
            else
            {
                mouse_x1 = clicked_x_coord;
                if (mouse_x > mouse_x1) { // Ensure mouse_x is lower frequency
                    std::swap(mouse_x, mouse_x1);
                }
                ui->radioButton_5->setText("Band-stop range " + QString::number(mouse_x, 'f', 2) + "-" + QString::number(mouse_x1, 'f', 2));
                flag = false;
                qDebug() << "Band-stop: Second click at" << mouse_x1 << ". Range set to" << mouse_x << "-" << mouse_x1;
            }
        }
        else 
        {
            mouse_x = clicked_x_coord;
            mouse_x1 = -1.0; 
            flag = false; 

            if (ui->radioButton_2->isChecked()) // Low-pass
            {
                ui->radioButton_2->setText("Low-pass freq " + QString::number(mouse_x, 'f', 2));
                qDebug() << "Low-pass frequency set to" << mouse_x;
            }
            else if (ui->radioButton_3->isChecked()) // High-pass
            {
                ui->radioButton_3->setText("High-pass freq " + QString::number(mouse_x, 'f', 2));
                qDebug() << "High-pass frequency set to" << mouse_x;
            }
            else
            {
                qDebug() << "No filter type selected for single click.";
            }
        }
    }
    else if (event->type() == QEvent::MouseMove && ui->custom->axisRect()->rect().contains(event->pos())) {
        double x_coord = ui->custom->xAxis->pixelToCoord(event->pos().x());
        double y_coord = ui->custom->yAxis->pixelToCoord(event->pos().y());

        if (m_coordText) {
            m_coordText->setText(QString("X: %1\nY: %2").arg(x_coord, 0, 'f', 2).arg(y_coord, 0, 'f', 2));
            m_coordText->position->setCoords(x_coord, y_coord);
            m_coordText->setVisible(true);
        }
        ui->custom->replot(QCustomPlot::rpQueuedReplot);
    }
    else
    {
        if (m_coordText) {
            m_coordText->setVisible(false);
        }
        ui->custom->replot(QCustomPlot::rpQueuedReplot);
    }
}

void MainWindow::pushbutton3() // Apply Filter button
{
    if (fftData.empty()) {
        QMessageBox::warning(this, "Filter Error", "Please perform FFT on an audio file first.");
        return;
    }

    float freq_resolution = 0.0f;
    if (fftSize > 0) {
        freq_resolution = static_cast<float>(metadata1.sampleRate) / fftSize;
    }
    else if (!fftData.empty() && metadata1.sampleRate > 0) {
        freq_resolution = static_cast<float>(metadata1.sampleRate) / (fftData.size() / 2.0f);
        qDebug() << "Warning: fftSize not set, calculating freq_resolution based on fftData.size(). This might be incorrect for filter2's 'ff' parameter.";
    }
    if(ui->radioButton_6->isChecked())
    {
        QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
            Q_ARG(std::vector<float>, fftData),
            Q_ARG(float, 0),
            Q_ARG(float, 1760),
            Q_ARG(int, 22),
            Q_ARG(float, freq_resolution));
	
        ui->radioButton_6->setChecked(false);
        return;
    }
    else if (ui->radioButton_7->isChecked()) {

        QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
            Q_ARG(std::vector<float>, fftData),
            Q_ARG(float, 810),
            Q_ARG(float, 6600),
            Q_ARG(int, 22),
            Q_ARG(float, freq_resolution));

        ui->radioButton_7->setChecked(false);
        return;

    }
    if (mouse_x != -1.0 && ui->radioButton_2->isChecked()) { // Low-pass
        QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
            Q_ARG(std::vector<float>, fftData),
            Q_ARG(float, static_cast<float>(mouse_x)),
            Q_ARG(float, 0.0f),
            Q_ARG(int, 11),
            Q_ARG(float, freq_resolution));
        mouse_x = -1.0;
        ui->radioButton_2->setText("low pass");
        ui->radioButton_2->setChecked(false);
    }
    else if (mouse_x != -1.0 && ui->radioButton_3->isChecked()) { // High-pass
        QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
            Q_ARG(std::vector<float>, fftData),
            Q_ARG(float, static_cast<float>(mouse_x)),
            Q_ARG(float, 0.0f),
            Q_ARG(int, 12),
            Q_ARG(float, freq_resolution));
        mouse_x = -1.0;
        ui->radioButton_3->setText("high pass");
        ui->radioButton_3->setChecked(false);
    }
    else if (mouse_x != -1.0 && mouse_x1 != -1.0 && ui->radioButton_4->isChecked()) { // Band-pass
        QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
            Q_ARG(std::vector<float>, fftData),
            Q_ARG(float, static_cast<float>(mouse_x)),
            Q_ARG(float, static_cast<float>(mouse_x1)),
            Q_ARG(int, 21),
            Q_ARG(float, freq_resolution));
        mouse_x = -1.0;
        mouse_x1 = -1.0;
        ui->radioButton_4->setText("band pass");
        ui->radioButton_4->setChecked(false);
    }
    else if (mouse_x != -1.0 && mouse_x1 != -1.0 && ui->radioButton_5->isChecked()) { // Band-stop
        QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
            Q_ARG(std::vector<float>, fftData),
            Q_ARG(float, static_cast<float>(mouse_x)),
            Q_ARG(float, static_cast<float>(mouse_x1)),
            Q_ARG(int, 22),
            Q_ARG(float, freq_resolution));
        mouse_x = -1.0;
        mouse_x1 = -1.0;
        ui->radioButton_5->setText("band stop");
        ui->radioButton_5->setChecked(false);
    }
    else {
        QMessageBox::warning(this, "Filter Error", "Please select a filter type and click on the graph to set the frequency/frequencies.");
    }
}

void MainWindow::pushbutton4() // IFFT of original FFT data
{
    if (fftWorker && !fftData.empty()) {
        QVector<float> qFftData(fftData.begin(), fftData.end());

        QMetaObject::invokeMethod(fftWorker, "processFFT", Qt::QueuedConnection,
            Q_ARG(QVector<float>, qFftData),
            Q_ARG(int, 1)); // 1 for IFFT
    }
    else {
        if (!fftWorker) {
            qDebug() << "Inverse FFT could not be started: fftWorker not initialized.";
        }
        if (fftData.empty()) {
            qDebug() << "Inverse FFT could not be started: Forward FFT must be done first (fftData is empty).";
            QMessageBox::warning(this, "IFFT Error", "Please calculate FFT for an audio file first.");
        }
    }
}

void MainWindow::pushbutton2() // Process Audio File for FFT
{
    QComboBox* sourceComboBox = ui->comboBox;
    if (flag2) {
        if (flag3) {
            sourceComboBox = ui->comboBox_3;
        }
        else {
            sourceComboBox = ui->comboBox_2;
        }
    }

    QVector<float> audioDataVec;
    QString selectedFile = sourceComboBox->currentText();
    QString directoryPath = currentDirectory.path();
    qDebug() << "Selected file for Forward FFT:" << selectedFile << "Directory path:" << directoryPath;

    oke = false;

    if (directoryPath.isEmpty()) {
        qDebug() << "No directory selected. Cannot process file:" << selectedFile;
        QMessageBox::warning(this, "Directory Not Selected", "Please select the directory containing the audio files first.");
        return;
    }
    if (selectedFile.isEmpty()) {
        qDebug() << "No file selected.";
        QMessageBox::warning(this, "File Not Selected", "Please select an audio file to process.");
        return;
    }

    QString filePath = QDir(directoryPath).absoluteFilePath(selectedFile);
    directory = directoryPath.toStdString();

    metadata1 = {};

    if (!filePath.isEmpty()) {
        if (readAudioMetadata(filePath, metadata1)) {
            qDebug() << "File:" << selectedFile;
            qDebug() << "Sample Rate:" << metadata1.sampleRate << "hz";
            qDebug() << "Channel Count:" << metadata1.channelCount;
            qDebug() << "Duration:" << metadata1.duration << "ms";
            qDebug() << "Bit Rate:" << metadata1.bitRate << "bps";
            qDebug() << "Bit Depth:" << metadata1.bitDepth << " bit";
        }
        else {
            qDebug() << "Metadata reading failed:" << selectedFile;
            QMessageBox::warning(this, "Metadata Error", QString("Could not read metadata for '%1'.").arg(selectedFile));
            return;
        }
    }
    else {
        qDebug() << "File path is empty.";
        return;
    }

    if (filePath.toLower().endsWith(".wav")) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Could not open file:" << filePath;
            QMessageBox::critical(this, "File Error", QString("Could not open file '%1'.").arg(selectedFile));
            return;
        }

        file.seek(44);
        QByteArray rawPcmData = file.readAll();
        file.close();

        if (rawPcmData.isEmpty()) {
            qDebug() << "Could not read data from WAV file (empty after header):" << filePath;
            QMessageBox::warning(this, "Data Error", QString("Could not read audio data from file '%1'.").arg(selectedFile));
            return;
        }

        int bitDepth = metadata1.bitDepth;

        int bytesPerSample = bitDepth / 8;
        if (bytesPerSample == 0) {
            qDebug() << "Invalid bytesPerSample calculated, bit depth:" << bitDepth;
            return;
        }
        int sampleCount = rawPcmData.size() / bytesPerSample;
        audioDataVec.reserve(sampleCount);

        if (bitDepth == 8) {
            const quint8* pcmData = reinterpret_cast<const quint8*>(rawPcmData.constData());
            for (int i = 0; i < sampleCount; ++i) {
                audioDataVec.append(static_cast<float>(pcmData[i] - 128) / 128.0f);
            }
        }
        else if (bitDepth == 16) {
            const qint16* pcmData = reinterpret_cast<const qint16*>(rawPcmData.constData());
            for (int i = 0; i < sampleCount; ++i) {
                audioDataVec.append(static_cast<float>(pcmData[i]) / 32768.0f);
            }
        }
        else if (bitDepth == 32 && metadata1.isFloat) {
            const float* pcmData = reinterpret_cast<const float*>(rawPcmData.constData());
            for (int i = 0; i < sampleCount; ++i) {
                audioDataVec.append(pcmData[i]);
            }
        }
        else if (bitDepth == 32 && !metadata1.isFloat) {
            const qint32* pcmData = reinterpret_cast<const qint32*>(rawPcmData.constData());
            for (int i = 0; i < sampleCount; ++i) {
                audioDataVec.append(static_cast<float>(pcmData[i]) / 2147483648.0f);
            }
        }
        else {
            qDebug() << "Unsupported bit depth for direct conversion:" << bitDepth;
            QMessageBox::warning(this, "Data Error", QString("Unsupported bit depth: %1").arg(bitDepth));
            return;
        }

        if (audioDataVec.isEmpty()) {
            qDebug() << "Audio data is empty after conversion.";
            QMessageBox::warning(this, "Data Error", "Audio data could not be converted to float.");
            return;
        }

        oke = true;

        if (flag2 && !flag3) {
            QCustomPlot* customPlot1 = ui->custom2;
            customPlot1->clearGraphs();
            customPlot1->addGraph();
            customPlot1->graph(0)->setPen(QPen(Qt::red));
            customPlot1->graph(0)->setName("Amplitude");

            QVector<double> xData(audioDataVec.size());
            QVector<double> yData(audioDataVec.size());
            for (int i = 0; i < audioDataVec.size(); ++i) {
                xData[i] = static_cast<double>(i) / metadata1.sampleRate;
                yData[i] = static_cast<double>(audioDataVec[i]);
            }

            customPlot1->graph(0)->setData(xData, yData);
            customPlot1->xAxis->setLabel("Time (s)");
            customPlot1->yAxis->setLabel("Amplitude");
            customPlot1->rescaleAxes();
            customPlot1->replot();
        }

        if (!flag2) {
            if (progressDialog) {
                progressDialog->close();
                delete progressDialog;
                progressDialog = nullptr;
            }
            progressDialog = new QProgressDialog("Calculating FFT...", "Cancel", 0, 0, this);
            progressDialog->setWindowModality(Qt::WindowModal);
            progressDialog->setMinimumDuration(0);
            progressDialog->setValue(0);
            progressDialog->show();
        }

        QMetaObject::invokeMethod(fftWorker, "processFFT", Qt::QueuedConnection,
            Q_ARG(QVector<float>, audioDataVec),
            Q_ARG(int, -1));
    }
    else {
        QMessageBox::warning(this, "File Error", "Selected file is not a .wav file.");
        return;
    }
}

void MainWindow::onFFTFinished(const std::vector<float>& workerResult, int inverse, unsigned int fftSizeUsed)
{
    if (progressDialog) {
        progressDialog->close();
        delete progressDialog;
        progressDialog = nullptr;
    }
    this->fftSize = fftSizeUsed;

    QCustomPlot* plotSpectrum = ui->custom;
    QCustomPlot* plotTimeDomainIFFT = ui->custom;
    QCustomPlot* plotFftForFlag2 = ui->custom3;

    plotSpectrum->clearGraphs();
    if (plotSpectrum != plotTimeDomainIFFT) plotTimeDomainIFFT->clearGraphs();
    if (plotFftForFlag2) plotFftForFlag2->clearGraphs();

    if (workerResult.empty()) {
        QString operationType = (inverse == -1) ? "FFT" : (inverse == 1 ? "IFFT" : "Filter");
        qDebug() << operationType << " result is empty or worker could not be started. Received FFT Size:" << fftSizeUsed;
        QMessageBox::warning(this, operationType + " Error", operationType + " results are empty or the operation failed.");

        if (m_triggerFilterProcessing_Button7) {
            m_triggerFilterProcessing_Button7 = false;
            m_filterTypeForButton7Sequence = 0;
            oke = false;
        }
        return;
    }

    if (inverse == -1) {
        qDebug() << "FFT completed. Result (complex) size:" << workerResult.size() << ". FFT Size Used:" << fftSizeUsed;

        fftData.assign(workerResult.begin(), workerResult.end());

        size_t numMagnitudePoints = (fftSizeUsed / 2) + 1;
        std::vector<double> magnitudes;
        magnitudes.reserve(numMagnitudePoints);

        fftData2.clear();
        fftData2.reserve(numMagnitudePoints);

        for (size_t k = 0; k < numMagnitudePoints; k++) {
            if ((2 * k + 1) < workerResult.size()) {
                float re = workerResult[2 * k];
                float im = workerResult[2 * k + 1];
                double mag = std::sqrt(std::pow(static_cast<double>(re), 2) + std::pow(static_cast<double>(im), 2));
                magnitudes.push_back(mag);
                fftData2.push_back(static_cast<float>(mag));
            }
            else if (2 * k < workerResult.size() && k == numMagnitudePoints - 1 && fftSizeUsed % 2 != 0) {
                float re = workerResult[2 * k];
                magnitudes.push_back(std::abs(static_cast<double>(re)));
                fftData2.push_back(std::abs(re));
            }
            else {
                qDebug() << "Warning: workerResult seems shorter than expected for magnitudes. k =" << k << "numMagPoints=" << numMagnitudePoints << "workerResult.size()=" << workerResult.size();
                break;
            }
        }

        QCustomPlot* currentPlotForFFT = plotSpectrum;
        if (flag2 && !flag3) {
            currentPlotForFFT = plotFftForFlag2;
        }
        currentPlotForFFT->clearGraphs();
        currentPlotForFFT->setWindowTitle("FFT Amplitude Spectrum");
        currentPlotForFFT->addGraph();
        currentPlotForFFT->graph(0)->setPen(QPen(Qt::blue));
        currentPlotForFFT->graph(0)->setName("FFT Amplitudes");

        QVector<double> xData(magnitudes.size());
        QVector<double> yData(magnitudes.begin(), magnitudes.end());

        double freqResolution = (fftSizeUsed > 0 && metadata1.sampleRate > 0) ? (static_cast<double>(metadata1.sampleRate) / fftSizeUsed) : 0.0;

        for (size_t i = 0; i < magnitudes.size(); ++i) {
            xData[i] = static_cast<double>(i) * freqResolution;
        }

        currentPlotForFFT->graph(0)->setData(xData, yData);
        currentPlotForFFT->xAxis->setLabel("Frequency (Hz)");
        currentPlotForFFT->yAxis->setLabel("Amplitude");
        currentPlotForFFT->rescaleAxes();
        currentPlotForFFT->replot();

        if (!flag2) {
            QMessageBox::information(this, "FFT Completed",
                QString("FFT calculation completed successfully. %1 amplitude points plotted. FFT size used: %2").arg(magnitudes.size()).arg(fftSizeUsed));
        }

        if (m_filterTypeForButton7Sequence != 0 && oke) {
            m_triggerFilterProcessing_Button7 = true;
        }

        if (m_triggerFilterProcessing_Button7) {
            bool filterDispatchAttempted = false;
            if (!fftData.empty() && metadata1.sampleRate > 0 && fftSizeUsed > 0) {
                float filter_freq_resolution = static_cast<float>(metadata1.sampleRate) / fftSizeUsed;

                if (m_filterTypeForButton7Sequence == 23) {
                    QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
                        Q_ARG(std::vector<float>, fftData),
                        Q_ARG(float, 5000.0f),
                        Q_ARG(float, 11025.0f),
                        Q_ARG(int, 23),
                        Q_ARG(float, filter_freq_resolution));
                    filterDispatchAttempted = true;
                }
                else if (m_filterTypeForButton7Sequence == 24) {
                    QMetaObject::invokeMethod(filterWorker, "filter2", Qt::QueuedConnection,
                        Q_ARG(std::vector<float>, fftData2),
                        Q_ARG(float, 0.0f),
                        Q_ARG(float, 0.0f),
                        Q_ARG(int, 24),
                        Q_ARG(float, filter_freq_resolution));
                    filterDispatchAttempted = true;
                }
            }

            if (!filterDispatchAttempted) {
                qDebug() << "Filter not applied for button7/9 sequence: Pre-conditions not met.";
                QMessageBox::warning(this, "Filter Error", "Cannot apply filter: FFT data invalid or parameters missing.");
                oke = false;
            }
            m_triggerFilterProcessing_Button7 = false;
            m_filterTypeForButton7Sequence = 0;
        }
    }
    else if (inverse == 1) {
        qDebug() << "IFFT completed. Result (time domain) size:" << workerResult.size() << ". FFT Size Used:" << fftSizeUsed;

        t1.assign(workerResult.begin(), workerResult.end());

        plotTimeDomainIFFT->clearGraphs();
        plotTimeDomainIFFT->setWindowTitle("IFFT Result (Time Domain)");
        plotTimeDomainIFFT->addGraph();
        plotTimeDomainIFFT->graph(0)->setPen(QPen(Qt::red));
        plotTimeDomainIFFT->graph(0)->setName("IFFT Signal");

        QVector<double> xData(t1.size());
        QVector<double> yData(t1.size());
        for (size_t i = 0; i < t1.size(); ++i) {
            if (metadata1.sampleRate > 0) {
                xData[i] = static_cast<double>(i) / metadata1.sampleRate;
            }
            else {
                xData[i] = static_cast<double>(i);
            }
            yData[i] = static_cast<double>(t1[i]);
        }

        plotTimeDomainIFFT->graph(0)->setData(xData, yData);
        plotTimeDomainIFFT->xAxis->setLabel("Time (s)");
        plotTimeDomainIFFT->yAxis->setLabel("Amplitude");
        plotTimeDomainIFFT->rescaleAxes();
        plotTimeDomainIFFT->replot();

        if (ui->radioButton->isChecked()) {
            struct::sampleData sd;
            sd.sampleRate = metadata1.sampleRate;
            sd.numChannels = metadata1.channelCount;
            sd.bitDepth = metadata1.bitDepth;
            sd.fileName = "output_ifft.wav";
            sd.filePath = directory + "/" + sd.fileName;

            qDebug() << "Saving IFFT result as WAV:" << QString::fromStdString(sd.filePath);
            rawdatatowavfile(t1, sd);

            QMessageBox::information(this, "IFFT Completed",
                QString("IFFT calculation completed successfully and saved as '%1'.").arg(QString::fromStdString(sd.fileName)));
        }
        else {
            QMessageBox::information(this, "IFFT Completed",
                QString("IFFT calculation completed successfully. %1 time domain samples obtained.").arg(t1.size()));
        }
    }
    else if (inverse == 31) {
        qDebug() << "Filter processing finished. Result (complex) size:" << workerResult.size() << ". FFT Size Used (context):" << fftSizeUsed;

        fftDatafiltered.assign(workerResult.begin(), workerResult.end());

        size_t numFilteredMagnitudePoints = (fftDatafiltered.size() / 2);
        std::vector<double> filteredMagnitudes;
        filteredMagnitudes.reserve(numFilteredMagnitudePoints);

        for (size_t k = 0; k < numFilteredMagnitudePoints; ++k) {
            if ((2 * k + 1) < fftDatafiltered.size()) {
                float re = fftDatafiltered[2 * k];
                float im = fftDatafiltered[2 * k + 1];
                filteredMagnitudes.push_back(std::sqrt(std::pow(static_cast<double>(re), 2) + std::pow(static_cast<double>(im), 2)));
            }
            else {
                break;
            }
        }

        plotSpectrum->clearGraphs();
        plotSpectrum->setWindowTitle("Filtered Signal Spectrum");
        plotSpectrum->addGraph();
        plotSpectrum->graph(0)->setPen(QPen(Qt::green));
        plotSpectrum->graph(0)->setName("Filtered Spectrum");

        QVector<double> xData(filteredMagnitudes.size());
        QVector<double> yData(filteredMagnitudes.begin(), filteredMagnitudes.end());

        double freqResolution = (this->fftSize > 0 && metadata1.sampleRate > 0) ? (static_cast<double>(metadata1.sampleRate) / this->fftSize) : 0.0;

        for (size_t i = 0; i < filteredMagnitudes.size(); ++i) {
            xData[i] = static_cast<double>(i) * freqResolution;
        }

        plotSpectrum->graph(0)->setData(xData, yData);
        plotSpectrum->xAxis->setLabel("Frequency (Hz)");
        plotSpectrum->yAxis->setLabel("Amplitude");
        plotSpectrum->rescaleAxes();
        plotSpectrum->replot();
        QMessageBox::information(this, "Filter Applied", "Filter applied successfully. Displaying filtered spectrum.");
    }
}

void MainWindow::pushbutton() // Select Directory
{
    QLabel* targetLabel = ui->label;
    QComboBox* targetComboBox = ui->comboBox;

    if (flag2) {
        if (flag3) {
            targetLabel = ui->label_5;
            targetComboBox = ui->comboBox_3;
        }
        else {
            targetLabel = ui->label_2;
            targetComboBox = ui->comboBox_2;
        }
    }

    QString dirPath = QFileDialog::getExistingDirectory(this,
        tr("Please select the directory containing audio files"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dirPath.isEmpty()) {
        currentDirectory.setPath(dirPath);
        targetLabel->setText("Selected directory: " + dirPath);
        QDir selectedDir(dirPath);
        directory = dirPath.toStdString();

        QStringList nameFilters;
        nameFilters << "*.wav";
        selectedDir.setNameFilters(nameFilters);
        QStringList fileList = selectedDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        targetComboBox->clear();

        if (fileList.isEmpty()) {
            qDebug() << "No .wav files found in the selected directory:" << dirPath;
            QMessageBox::information(this, "No Files Found", "No .wav audio files found in the selected directory.");
            targetLabel->setText("No .wav files in: " + dirPath.left(30) + "...");
        }
        else {
            qDebug() << fileList.size() << ".wav files found in" << dirPath;
            for (const QString& fileName : fileList) {
                targetComboBox->addItem(fileName);
            }
            if (!fileList.isEmpty()) {
                targetLabel->setText("Selected file: " + targetComboBox->currentText());
            }
        }
    }
    else {
        targetLabel->setText("No directory selected");
        directory.clear();
        targetComboBox->clear();
    }

    if (flag2 && flag3) {
        flag3 = false;
    }
    if (flag2) {
        flag2 = false;
    }
}