
#include <QTWidgets/QApplication>
#include <MainWindow.h>

int main(int argc, char* argv[])
{ 
    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    MainWindow window;
    
    window.setWindowTitle("digicom");

    window.resize(800, 600);
	window.setMinimumSize(800, 600);
    


    window.show();

    return app.exec();
   
}
