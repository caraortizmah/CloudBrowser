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

// Global pointers for access in callbacks (needed for lambda captures)
QFileSystemModel *g_fsModel = nullptr;
QStringListModel *g_customModel = nullptr;
QListView *g_view = nullptr;
QLineEdit *g_pathBar = nullptr;
QLabel *g_statusLabel = nullptr;
Config g_cfg;
QString g_currentPath;
bool g_isCustomMode = false;
QStringList g_customFolders;

// Function declarations
bool shouldIntercept(const QString& folderPath);
void enterCustomMode(const QString& path);
void enterNormalMode(const QString& path);
void changeDirectory(const QString& path);

// Function definitions
bool shouldIntercept(const QString& folderPath) {
    QString folderName = QDir(folderPath).dirName();
    return g_cfg.interceptedFolders.contains(folderName);
}

void enterCustomMode(const QString& path) {
    g_customFolders = readCustomFolderList(g_cfg.customListFilePath);
    g_customModel->setStringList(g_customFolders);
    g_view->setModel(g_customModel);
    g_isCustomMode = true;
    g_currentPath = path;
    g_pathBar->setText(path + " [CUSTOM VIEW]");
    g_statusLabel->setText(QString("Custom view: showing %1 folders from list").arg(g_customFolders.size()));
}

void enterNormalMode(const QString& path) {
    QModelIndex idx = g_fsModel->index(path);
    if (idx.isValid()) {
        g_view->setModel(g_fsModel);
        g_view->setRootIndex(idx);
        g_isCustomMode = false;
        g_currentPath = path;
        g_pathBar->setText(path);
        g_statusLabel->setText("Browsing: " + path);
    } else {
        g_statusLabel->setText("Cannot access: " + path);
    }
}

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

int main(int argc, char *argv[]) {
    // Create the Qt Application object
    QApplication app(argc, argv);

    g_cfg = readConfig();
    
    // Create the main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Folder Browser for Proton");
    mainWindow.resize(800, 600);

    // Real file system model (for normal browsing)
    g_fsModel = new QFileSystemModel();
    g_fsModel->setRootPath(g_cfg.startPath);
    g_fsModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    
    // String list model (for custom folder lists)
    g_customModel = new QStringListModel();
    
    // List view widget
    g_view = new QListView();
    // Set the view to show icons like a file manager
    g_view->setViewMode(QListView::IconMode);
    // Adjust layout when window is resized
    g_view->setResizeMode(QListView::Adjust);
    g_view->setGridSize(QSize(100, 100));
    g_view->setIconSize(QSize(64, 64));

    // Navigation widgets
    g_pathBar = new QLineEdit();
    
    QPushButton *goButton = new QPushButton("Go");
    QPushButton *upButton = new QPushButton("Up");
    g_statusLabel = new QLabel("Ready");
    
    // Connect signals
    QObject::connect(g_view, &QListView::doubleClicked, [&](const QModelIndex &index) {
        if (g_isCustomMode) {
            // In custom mode: open the selected folder path directly
            QString selectedPath = g_customFolders.value(index.row());
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
            QString path = g_fsModel->filePath(index);
            if (g_fsModel->isDir(index)) {
                changeDirectory(path);
            }
        }
    });
    
    // Connect navigation buttons
    QObject::connect(goButton, &QPushButton::clicked, [&]() {
        changeDirectory(g_pathBar->text());
    });
    
    QObject::connect(upButton, &QPushButton::clicked, [&]() {
        if (g_isCustomMode) {
            // Up from custom view goes to parent directory
            QDir parent(g_currentPath);
            parent.cdUp();
            changeDirectory(parent.absolutePath());
        } else {
            QDir current(g_currentPath);
            current.cdUp();
            changeDirectory(current.absolutePath());
        }
    });
    
    QObject::connect(g_pathBar, &QLineEdit::returnPressed, [&]() {
        changeDirectory(g_pathBar->text());
    });
    
    // Layout
    QWidget *centralWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *navLayout = new QHBoxLayout();
    
    navLayout->addWidget(upButton);
    navLayout->addWidget(g_pathBar);
    navLayout->addWidget(goButton);
    
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(g_view);
    mainLayout->addWidget(g_statusLabel);
    
    mainWindow.setCentralWidget(centralWidget);

    // Start at initial path
    changeDirectory(g_cfg.startPath);
    
    // Show the window and start the event loop
    mainWindow.show();
    return app.exec();
}
