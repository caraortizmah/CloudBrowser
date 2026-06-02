#include <QApplication>
#include <QMainWindow>
#include <QFileSystemModel>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStringListModel>
#include <QListView>

// Configuration structure
struct Config {
    QString startPath;
    QStringList interceptedFolders;  // Folder names that trigger custom view
    QString customListFilePath;       // File containing custom folder list
};
Config readConfig() {
    // Look for config file in several locations
    Config cfg;
    
    // Default values
    cfg.startPath = QDir::homePath();
    cfg.customListFilePath = QDir::homePath() + "/.config/folderbrowser/folders.txt";
    
    QStringList configPaths = {
        QDir::homePath() + "/.config/folderbrowser/config.json",
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
    // Create the Qt Application object
    QApplication app(argc, argv);

    QString startPath = readConfigPath();
    
    // Create the main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Folder Broser for proton - " + startPath);
    mainWindow.resize(800, 600);

    // Create the file system model
    QFileSystemModel *model = new QFileSystemModel();

    // Set the root path to watch (user's home directory)
    model->setRootPath(startPath);
    model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    // Create the list view and set the model
    QListView *view = new QListView();
    view->setModel(model);
    // Set the view to show icons like a file manager
    view->setViewMode(QListView::IconMode);
    // Adjust layout when window is resized
    view->setResizeMode(QListView::Adjust);
    view->setGridSize(QSize(100, 100));
    view->setIconSize(QSize(64, 64));

    // Set the root index of the view to the user's home directory
    //    This ensures the view starts inside the home folder, not the entire system root.
    QModelIndex rootIndex = model->index(startPath);
    
    if (!rootIndex.isValid()) {
        QMessageBox::warning(&mainWindow, "Error", 
            "Cannot access folder:\n" + startPath + 
            "\n\nFalling back to home directory.");
        startPath = QDir::homePath();
        rootIndex = model->index(startPath);
        mainWindow.setWindowTitle("Folder Browser - " + startPath);
    }
    
    view->setRootIndex(rootIndex);

    // Place the view inside the main window
    // Navigation widgets
    QLineEdit *pathBar = new QLineEdit();
    pathBar->setText(startPath);
    
    QPushButton *goButton = new QPushButton("Go");
    QPushButton *upButton = new QPushButton("Up");
    QLabel *statusLabel = new QLabel("Ready");
    //    A QMainWindow requires a central widget.
    //  Using Layout
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *navLayout = new QHBoxLayout();
    
    navLayout->addWidget(upButton);
    navLayout->addWidget(pathBar);
    navLayout->addWidget(goButton);
    
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(view);
    mainLayout->addWidget(statusLabel);
    
    // Function to change directory
    auto changeDirectory = [&](const QString &path) {
        QModelIndex idx = model->index(path);
        if (idx.isValid()) {
            view->setRootIndex(idx);
            pathBar->setText(path);
            statusLabel->setText("Browsing: " + path);
        } else {
            statusLabel->setText("Cannot access: " + path);
        }
    };
    
    // Set initial directory
    changeDirectory(startPath);
    
    // Connect signals
    QObject::connect(view, &QListView::doubleClicked, [&](const QModelIndex &index) {
        QString path = model->filePath(index);
        if (model->isDir(index)) {
            changeDirectory(path);
        }
    });
    
    QObject::connect(goButton, &QPushButton::clicked, [&]() {
        changeDirectory(pathBar->text());
    });
    
    QObject::connect(upButton, &QPushButton::clicked, [&]() {
        QDir current(pathBar->text());
        current.cdUp();
        changeDirectory(current.absolutePath());
    });
    
    QObject::connect(pathBar, &QLineEdit::returnPressed, [&]() {
        changeDirectory(pathBar->text());
    });
    
    mainWindow.setCentralWidget(centralWidget);

    // Show the window and start the event loop
    mainWindow.show();
    return app.exec();
}
