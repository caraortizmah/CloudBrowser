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
                cfg.startPath = obj["folder_path"].toString();
	    }
            
            if (obj.contains("intercept_folders") && obj["intercept_folders"].isArray()) {
                QJsonArray arr = obj["intercept_folders"].toArray();
                for (const auto& val : arr) {
                    cfg.interceptedFolders << val.toString();
                }
            }
            
            if (obj.contains("custom_list_file")) {
                cfg.customListFilePath = obj["custom_list_file"].toString();
            }
        }
    }
    
    return cfg;
                }

QStringList readCustomFolderList(const QString& filePath) {
    QStringList folders;
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith("#")) {
                folders << line;
            }
        }
        file.close();
    }
    
    return folders;
}

int main(int argc, char *argv[]) {
    // Create the Qt Application object
    QApplication app(argc, argv);

    Config cfg = readConfig();
    
    // Create the main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Folder Broser for proton - " + startPath);
    mainWindow.resize(800, 600);

    // Real file system model (for normal browsing)
    QFileSystemModel *fsModel = new QFileSystemModel();
    fsModel->setRootPath(cfg.startPath);
    fsModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);

    // String list model (for custom folder lists)
    QStringListModel *customModel = new QStringListModel();
    
    // List view widget
    QListView *view = new QListView();
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
    
    QPushButton *goButton = new QPushButton("Go");
    QPushButton *upButton = new QPushButton("Up");
    QLabel *statusLabel = new QLabel("Ready");
    
    // Track current path and whether we're in "custom mode"
    QString currentPath = cfg.startPath;
    bool isCustomMode = false;
    QStringList customFolders;
    
    // Function to check if a folder should be intercepted
    bool shouldIntercept(const QString& folderPath) {
        QString folderName = QDir(folderPath).dirName();
        return cfg.interceptedFolders.contains(folderName);
    }
    
    // Function to switch to custom view
    void enterCustomMode(const QString& path) {
        customFolders = readCustomFolderList(cfg.customListFilePath);
        QStringListModel *model = qobject_cast<QStringListModel*>(customModel);
        if (model) {
            model->setStringList(customFolders);
        }
        view->setModel(customModel);
        isCustomMode = true;
        currentPath = path;
        pathBar->setText(path + " [CUSTOM VIEW]");
        statusLabel->setText(QString("Custom view: showing %1 folders from list").arg(customFolders.size()));
    }
    
    // Function to switch to normal filesystem view
    void enterNormalMode(const QString& path) {
        QModelIndex idx = fsModel->index(path);
        if (idx.isValid()) {
            view->setModel(fsModel);
            view->setRootIndex(idx);
            isCustomMode = false;
            currentPath = path;
            pathBar->setText(path);
            statusLabel->setText("Browsing: " + path);
        } else {
            statusLabel->setText("Cannot access: " + path);
        }
    }
    
    // Function to change directory with interception logic
    void changeDirectory(const QString& path) {
        QDir targetDir(path);
        QString canonicalPath = targetDir.canonicalPath();
        
        // Check if we should intercept this folder
        if (shouldIntercept(canonicalPath)) {
            enterCustomMode(canonicalPath);
        } else {
            enterNormalMode(canonicalPath);
        }
    }
    
    // Connect signals
    QObject::connect(view, &QListView::doubleClicked, [&](const QModelIndex &index) {
        if (isCustomMode) {
            // In custom mode: open the selected folder path directly
            QString selectedPath = customFolders.value(index.row());
            if (!selectedPath.isEmpty() && QDir(selectedPath).exists()) {
                // Check if the new path itself should be intercepted
                if (shouldIntercept(selectedPath)) {
                    enterCustomMode(selectedPath);
                } else {
                    enterNormalMode(selectedPath);
                }
            }
        } else {
            // Normal mode: get path from filesystem model
            QString path = fsModel->filePath(index);
            if (fsModel->isDir(index)) {
            changeDirectory(path);
        }
        }
    });
    
    // Connect navigation buttons
    QObject::connect(goButton, &QPushButton::clicked, [&]() {
        changeDirectory(pathBar->text());
    });
    
    QObject::connect(upButton, &QPushButton::clicked, [&]() {
        if (isCustomMode) {
            // Up from custom view goes to parent directory
            QDir parent(currentPath);
            parent.cdUp();
            changeDirectory(parent.absolutePath());
        } else {
            QDir current(currentPath);
        current.cdUp();
        changeDirectory(current.absolutePath());
        }
    });
    
    QObject::connect(pathBar, &QLineEdit::returnPressed, [&]() {
        changeDirectory(pathBar->text());
    });
    
    // Layout
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *navLayout = new QHBoxLayout();
    
    navLayout->addWidget(upButton);
    navLayout->addWidget(pathBar);
    navLayout->addWidget(goButton);
    
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(view);
    mainLayout->addWidget(statusLabel);
    
    mainWindow.setCentralWidget(centralWidget);

    // Start at initial path
    changeDirectory(cfg.startPath);
    
    // Show the window and start the event loop
    mainWindow.show();
    return app.exec();
}
