#include <QApplication>
#include <QMainWindow>
#include <QFileSystemModel>
#include <QListView>
#include <QVBoxLayout>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>

QString readConfigPath() {
    // Look for config file in several locations
    QStringList configPaths = {
        QDir::homePath() + "/.config/folderbrowser/config.json",
        QDir::homePath() + "/folderbrowser_config.json",
        QCoreApplication::applicationDirPath() + "/config.json"
    };
    
    for (const QString& configPath : configPaths) {
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            
            if (obj.contains("folder_path")) {
                QString path = obj["folder_path"].toString();
                if (QDir(path).exists()) {
                    qDebug() << "Loaded config from:" << configPath;
                    qDebug() << "Folder path:" << path;
                    return path;
                }
            }
        }
    }
    
    // Fallback to home directory if config not found
    qDebug() << "Config not found, using home directory";
    return QDir::homePath();
}

int main(int argc, char *argv[]) {
    // 1. Create the Qt Application object
    QApplication app(argc, argv);

    // 2. Create the main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Simple Folder Browser");

    // 3. Create the file system model
    QFileSystemModel *model = new QFileSystemModel();
    // Set the root path to watch (user's home directory)
    model->setRootPath(QDir::homePath());

    // 4. Create the list view and set the model
    QListView *view = new QListView();
    view->setModel(model);
    // Set the view to show icons like a file manager
    view->setViewMode(QListView::IconMode); // [citation:4][citation:9]
    // Adjust layout when window is resized
    view->setResizeMode(QListView::Adjust); // [citation:9]

    // 5. Set the root index of the view to the user's home directory
    //    This ensures the view starts inside the home folder, not the entire system root.
    QModelIndex rootIndex = model->index(QDir::homePath());
    view->setRootIndex(rootIndex); // [citation:5]

    // 6. Place the view inside the main window
    //    A QMainWindow requires a central widget.
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(view);
    mainWindow.setCentralWidget(centralWidget); // [citation:6]

    // 7. Show the window and start the event loop
    mainWindow.show();
    return app.exec();
}
