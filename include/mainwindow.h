#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>

#include <QProgressDialog>
#include <qcustomplot.h>
#include <QMouseEvent>
#include <readfile.h>
#include <Filter1.h>
#include "FFTWorker.h"  // Dosya adý büyük harfle eþleþmeli
#include <QFileDialog>
#include <struct.h>
#include <realtime.h>

namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void pushbutton();
    void onMousePress(QMouseEvent* event);
    void pushbutton2();
    void spectrum(const std::vector<float>& result, int inverse, unsigned int fftSizeUsed);
    void onFFTFinished(const std::vector<float>& result, int inverse, unsigned int fftSizeUsed);
	void realtimeFinished(const std::vector<double>& result, const std::vector<double>& time,const std::vector<double>& firres, unsigned int fftSizeUsed);
    void pushbutton4();
    void pushbutton3();
    void pushbutton6();
    void pushbutton5();
    void pushbutton8();
    void pushbutton7();
    void pushbutton9();
    void pushbutton10();
    void pushbutton11();

  
    
private:
    std::vector<float> fftData;
    std::vector<float> fftData2;
    bool flag2;
    bool flag3;
    bool oke;
    QVector<QString> csvData;
    QVector<qfloat16> csvData2;
    bool oke2;
    std::vector<float> fftDatafiltered;
    Ui::MainWindow *ui;
    QDir currentDirectory;
    FFTWorker* fftWorker;
    Filter1* filterWorker;
    float mouse_x, mouse_x1;
    QThread* workerThread;
    bool flag;
    QProgressDialog* progressDialog;
    std::vector<float> t1;
    std::string directory;
    QCPItemTracer* mPositionMarker;
   
	realtime* realtimeWorker;

	QCustomPlot* realtimePlot;
    QCustomPlot* realtimePlot2;
    double freq_resolution12;

	QVector<double> averageData;
	int averageCounter;
	QVector<double> realaverag;
    AudioMetadata metadata1;
    unsigned int fftSize;
    bool m_triggerFilterProcessing_Button7;
    int m_filterTypeForButton7Sequence;
    QCPItemText* m_coordText;
};
#endif // MAINWINDOW_H
