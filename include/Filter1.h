#pragma once
#include <iostream>
#include <vector>
#include <QObject>  
#include <QThread>  

class Filter1 : public QObject { // QObject'ten türetildi
	Q_OBJECT 
	
public:

	explicit Filter1(QObject* parent = nullptr);
	~Filter1();

public slots:
	Q_INVOKABLE void filter2(const std::vector<float>& data, float f1, float f2, int state,float ff);
	
	 
	
signals:
	void finished(const std::vector<float>& result, int inverse, unsigned int fftSizeUsed);
	void spectrum(const std::vector<float>& result, int inverse, unsigned int fftSizeUsed);

	

};