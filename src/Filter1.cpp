#include "filter1.h"
#include <iostream> // std::cerr ve std::cout için
#include <vector>
#include <algorithm> // std::max_element için eklendi
#include <iterator>  // std::distance için eklendi
#include <cmath>     // std::abs için zaten vardý
#include <QDebug>   // qDebug() için eklendi
// #include <QThread> // QThread bu dosyada doðrudan kullanýlmýyorsa kaldýrýlabilir

// Yapýcý: QObject üst sýnýfýnýn yapýcýsýný çaðýrýr
Filter1::Filter1(QObject* parent) : QObject(parent) {

}


Filter1::~Filter1() {

}


void Filter1::filter2(const std::vector<float>& data, float f1, float f2, int state, float ff) {
    if (data.empty()) {
        std::cerr << "Input data is empty!" << std::endl;
        emit finished({}, 31, 0); 
        return;
    }
    qDebug() << "Filter2 fonksiyonu çaðrýldý. Durum: " << data.size() << ff;
    std::vector<float> filteredData(data.size(), 0.0f);

    // case 24 için vektörler
    std::vector<float> main_amps(3,0.0f);
    std::vector<float> main_freqs(3,0.0f);
    std::vector<float> sub_amps(4,0.0f);
    std::vector<float> sub_freqs(4,0.0f);
    std::vector<float> detected_dtmf_freqs(2,0.0f); 
    std::vector<float>::iterator max_main_amp_iter;
    std::vector<float>::iterator max_sub_amp_iter;
    std::ptrdiff_t distance_main = 0;
    std::ptrdiff_t distance_sub = 0;
    float tolerance = ff / 2.0f;
    switch (state) {
    case 11: // Low-pass filter
        for (size_t i = 0; i < filteredData.size(); ++i) {
            if (static_cast<float>(i) < (f1 * 2.0f + 1.0f) / ff) {
                filteredData[i] = data[i];
            }
        }
        break;
    case 12: // High-pass filter
        for (size_t i = 0; i < filteredData.size(); ++i) {
            if (static_cast<float>(i) > (f1 * 2.0f + 1.0f) / ff) {
                filteredData[i] = data[i];
            }
        }
        break;
    case 21: // Band-pass filter
        for (size_t i = 0; i < filteredData.size(); ++i) {
            if (static_cast<float>(i) > (f1 * 2.0f + 1.0f) / ff && static_cast<float>(i) < (f2 * 2.0f + 1.0f) / ff) {
                filteredData[i] = data[i];
            }
        }
        break;
    case 22: // Band-stop filter
        for (size_t i = 0; i < filteredData.size(); ++i) {
            if (static_cast<float>(i) < (f1 * 2.0f + 1.0f) / ff || static_cast<float>(i) > (f2 * 2.0f + 1.0f) / ff) {
                filteredData[i] = data[i];
            }
        }
        break;
    case 23:
        for (size_t i = 0; i < filteredData.size(); ++i) {
            if (static_cast<float>(i) > (f1 * 2.0f + 1.0f) / ff && static_cast<float>(i) < (f2 * 2.0f + 1.0f) / ff) {
                filteredData[i] = data[i]; 
                
            }
        }
        emit spectrum(filteredData, 31, filteredData.size());
        return; 

    case 24:
        main_amps.clear();
        main_freqs.clear();
        sub_amps.clear();
        sub_freqs.clear();
        
        
        

        for (size_t j_idx = 0; j_idx < data.size() ; ++j_idx) { 
            float current_freq = static_cast<float>(j_idx) * ff;
            
            if (std::abs(current_freq - 1209.0f) <= tolerance) {
                main_amps.push_back(data[j_idx]);
                main_freqs.push_back(1209.0f);
            } else if (std::abs(current_freq - 1336.0f) <= tolerance) {
                main_amps.push_back(data[j_idx]);
                main_freqs.push_back(1336.0f);
            } else if (std::abs(current_freq - 1477.0f) <= tolerance) {
                main_amps.push_back(data[j_idx]);
                main_freqs.push_back(1477.0f);
            }

            
            if (std::abs(current_freq - 697.0f) <= tolerance) {
                sub_amps.push_back(data[j_idx]);
                sub_freqs.push_back(697.0f);
            } else if (std::abs(current_freq - 770.0f) <= tolerance) {
                sub_amps.push_back(data[j_idx]);
                sub_freqs.push_back(770.0f);
            } else if (std::abs(current_freq - 852.0f) <= tolerance) {
                sub_amps.push_back(data[j_idx]);
                sub_freqs.push_back(852.0f);
            } else if (std::abs(current_freq - 941.0f) <= tolerance) {
                sub_amps.push_back(data[j_idx]);
                sub_freqs.push_back(941.0f);
            }
        }

        if (main_amps.empty()) {
            main_amps.push_back(0.0f); 
            main_freqs.push_back(0.0f);
        }
        if (sub_amps.empty()) {
            sub_amps.push_back(0.0f);  
            sub_freqs.push_back(0.0f); 
        }
        
	    max_main_amp_iter = std::max_element(main_amps.begin(), main_amps.end());
	    max_sub_amp_iter = std::max_element(sub_amps.begin(), sub_amps.end());
        
        distance_main = std::distance(main_amps.begin(), max_main_amp_iter);
	    distance_sub = std::distance(sub_amps.begin(), max_sub_amp_iter);

        
        if (distance_main < static_cast<std::ptrdiff_t>(main_freqs.size())) { 
            detected_dtmf_freqs[0] = main_freqs[distance_main];
        } else {
            detected_dtmf_freqs[0] = 0.0f; 
        }

        if (distance_sub < static_cast<std::ptrdiff_t>(sub_freqs.size())) { 
		    detected_dtmf_freqs[1] = sub_freqs[distance_sub];
        } else {
            detected_dtmf_freqs[1] = 0.0f; 
        }

        emit spectrum(detected_dtmf_freqs, -2, detected_dtmf_freqs.size()); 
        return; 

    default:
        std::cerr << "Geçersiz filtre durumu: " << state << std::endl;
        emit finished(data, 31, 0); 
        return;
    }

    
    emit finished(filteredData, 31, filteredData.size());
}