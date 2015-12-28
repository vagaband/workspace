#include<QApplication>// application abstruct class
#include<QWidget> //window class

int main(int argc, char *argv[]){
    QApplication app(argc, argv);
    QWidget w;
    w.setWindowTitle("hello world!");
    w.show();
    return app.exec();
}

