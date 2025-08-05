#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_timer(new QTimer(this))
    , m_processor(new FileProcessor())
    , m_processorThread(new QThread())
{
    ui->setupUi(this);
    setupUI();
    connectSignals();
    loadSettings();
    
    setWindowTitle("File Modifier - Модификатор файлов");
    resize(800, 600);
}

MainWindow::~MainWindow()
{
    saveSettings();
    if (m_processorThread->isRunning()) {
        m_processor->stopProcessing();
        m_processorThread->quit();
        m_processorThread->wait();
    }
    delete ui;
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Input Settings Group
    QGroupBox *inputGroup = new QGroupBox("Настройки входных файлов", centralWidget);
    QGridLayout *inputLayout = new QGridLayout(inputGroup);
    
    inputLayout->addWidget(new QLabel("Маска файлов:"), 0, 0);
    m_inputMaskEdit = new QLineEdit("*.txt", inputGroup);
    inputLayout->addWidget(m_inputMaskEdit, 0, 1);
    
    inputLayout->addWidget(new QLabel("Путь к файлам:"), 1, 0);
    QHBoxLayout *inputPathLayout = new QHBoxLayout();
    m_inputPathEdit = new QLineEdit(inputGroup);
    m_inputPathEdit->setReadOnly(true);
    m_inputPathEdit->setPlaceholderText("Выберите папку с файлами");
    m_browseInputButton = new QPushButton("Обзор", inputGroup);
    inputPathLayout->addWidget(m_inputPathEdit);
    inputPathLayout->addWidget(m_browseInputButton);
    inputLayout->addLayout(inputPathLayout, 1, 1);
    
    m_deleteInputCheckBox = new QCheckBox("Удалять входные файлы", inputGroup);
    inputLayout->addWidget(m_deleteInputCheckBox, 2, 0, 1, 2);
    
    mainLayout->addWidget(inputGroup);
    
    // Output Settings Group
    QGroupBox *outputGroup = new QGroupBox("Настройки выходных файлов", centralWidget);
    QGridLayout *outputLayout = new QGridLayout(outputGroup);
    
    outputLayout->addWidget(new QLabel("Путь сохранения:"), 0, 0);
    QHBoxLayout *outputPathLayout = new QHBoxLayout();
    m_outputPathEdit = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), outputGroup);
    m_browseOutputButton = new QPushButton("Обзор", outputGroup);
    outputPathLayout->addWidget(m_outputPathEdit);
    outputPathLayout->addWidget(m_browseOutputButton);
    outputLayout->addLayout(outputPathLayout, 0, 1);
    
    outputLayout->addWidget(new QLabel("При конфликте имен:"), 1, 0);
    m_fileConflictComboBox = new QComboBox(outputGroup);
    m_fileConflictComboBox->addItem("Перезаписать", 0);
    m_fileConflictComboBox->addItem("Добавить счетчик", 1);
    outputLayout->addWidget(m_fileConflictComboBox, 1, 1);
    
    mainLayout->addWidget(outputGroup);
    
    // Processing Settings Group
    QGroupBox *processingGroup = new QGroupBox("Настройки обработки", centralWidget);
    QGridLayout *processingLayout = new QGridLayout(processingGroup);
    
    processingLayout->addWidget(new QLabel("XOR значение (8 байт):"), 0, 0);
    m_xorValueEdit = new QLineEdit("0123456789ABCDEF", processingGroup);
    processingLayout->addWidget(m_xorValueEdit, 0, 1);
    
    m_timerModeCheckBox = new QCheckBox("Режим таймера", processingGroup);
    processingLayout->addWidget(m_timerModeCheckBox, 1, 0);
    
    processingLayout->addWidget(new QLabel("Интервал (мс):"), 2, 0);
    m_timerIntervalSpinBox = new QSpinBox(processingGroup);
    m_timerIntervalSpinBox->setRange(1000, 60000);
    m_timerIntervalSpinBox->setValue(5000);
    m_timerIntervalSpinBox->setSuffix(" мс");
    processingLayout->addWidget(m_timerIntervalSpinBox, 2, 1);
    
    mainLayout->addWidget(processingGroup);
    
    // Control Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton("Старт", centralWidget);
    m_stopButton = new QPushButton("Стоп", centralWidget);
    m_stopButton->setEnabled(false);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // Progress and Status
    m_statusLabel = new QLabel("Готов к работе", centralWidget);
    mainLayout->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar(centralWidget);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Log
    QGroupBox *logGroup = new QGroupBox("Лог операций", centralWidget);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logTextEdit = new QTextEdit(logGroup);
    m_logTextEdit->setMaximumHeight(150);
    logLayout->addWidget(m_logTextEdit);
    mainLayout->addWidget(logGroup);
}

void MainWindow::connectSignals()
{
    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(m_browseInputButton, &QPushButton::clicked, this, &MainWindow::onBrowseInputPathClicked);
    connect(m_browseOutputButton, &QPushButton::clicked, this, &MainWindow::onBrowseOutputPathClicked);
    
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    
    // File processor signals
    connect(m_processor, &FileProcessor::progressChanged, this, &MainWindow::onProcessingProgress);
    connect(m_processor, &FileProcessor::processingFinished, this, &MainWindow::onProcessingFinished);
    connect(m_processor, &FileProcessor::processingError, this, &MainWindow::onProcessingError);
    connect(m_processor, &FileProcessor::fileProcessed, this, &MainWindow::onFileProcessed);
    connect(m_processor, &FileProcessor::statusChanged, m_statusLabel, &QLabel::setText);
    
    // Move processor to separate thread
    m_processor->moveToThread(m_processorThread);
    connect(m_processorThread, &QThread::started, m_processor, &FileProcessor::startProcessing);
    connect(m_processor, &FileProcessor::processingFinished, m_processorThread, &QThread::quit);
}

void MainWindow::onStartButtonClicked()
{
    if (!validateInputs()) {
        return;
    }
    
    // Configure processor
    m_processor->setInputMask(m_inputMaskEdit->text());
    m_processor->setOutputPath(m_outputPathEdit->text());
    m_processor->setDeleteInput(m_deleteInputCheckBox->isChecked());
    m_processor->setFileConflictMode(m_fileConflictComboBox->currentData().toInt());
    m_processor->setXorValue(parseXorValue(m_xorValueEdit->text()));
    // Use the selected input path or current directory
    QString inputPath = m_inputPath.isEmpty() ? QDir::currentPath() : m_inputPath;
    m_processor->setInputPath(inputPath);
    
    updateUIState(true);
    m_logTextEdit->append(QString("[%1] Начало обработки файлов...").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    
    if (m_timerModeCheckBox->isChecked()) {
        m_timer->start(m_timerIntervalSpinBox->value());
        m_logTextEdit->append(QString("[%1] Таймер запущен с интервалом %2 мс").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(m_timerIntervalSpinBox->value()));
    } else {
        m_processorThread->start();
    }
}

void MainWindow::onStopButtonClicked()
{
    m_timer->stop();
    m_processor->stopProcessing();
    updateUIState(false);
    m_logTextEdit->append(QString("[%1] Обработка остановлена").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::onBrowseInputPathClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку с файлами");
    if (!dir.isEmpty()) {
        m_inputPath = dir;
        m_inputPathEdit->setText(dir);
        m_logTextEdit->append(QString("[%1] Установлена папка: %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(dir));
    }
}

void MainWindow::onBrowseOutputPathClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку для сохранения");
    if (!dir.isEmpty()) {
        m_outputPathEdit->setText(dir);
    }
}

void MainWindow::onTimerTimeout()
{
    if (!m_processorThread->isRunning()) {
        m_processorThread->start();
    }
}

void MainWindow::onProcessingProgress(int progress)
{
    m_progressBar->setValue(progress);
}

void MainWindow::onProcessingFinished()
{
    updateUIState(false);
    m_progressBar->setVisible(false);
    m_logTextEdit->append(QString("[%1] Обработка завершена").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void MainWindow::onProcessingError(const QString &error)
{
    m_logTextEdit->append(QString("[%1] Ошибка: %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(error));
    QMessageBox::warning(this, "Ошибка", error);
}

void MainWindow::onFileProcessed(const QString &filename)
{
    m_logTextEdit->append(QString("[%1] Обработан файл: %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss")).arg(filename));
}

void MainWindow::updateUIState(bool processing)
{
    m_startButton->setEnabled(!processing);
    m_stopButton->setEnabled(processing);
    m_progressBar->setVisible(processing);
    
    if (processing) {
        m_progressBar->setValue(0);
    }
}

bool MainWindow::validateInputs()
{
    if (m_inputMaskEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите маску файлов");
        return false;
    }
    
    if (m_outputPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите путь для сохранения");
        return false;
    }
    
    if (!QDir(m_outputPathEdit->text()).exists()) {
        QMessageBox::warning(this, "Ошибка", "Папка для сохранения не существует");
        return false;
    }
    
    if (!isValidXorValue(m_xorValueEdit->text())) {
        QMessageBox::warning(this, "Ошибка", "XOR значение должно быть 16 символов (8 байт в hex)");
        return false;
    }
    
    return true;
}

QString MainWindow::getXorValue()
{
    return m_xorValueEdit->text();
}

void MainWindow::saveSettings()
{
    QSettings settings("FileModifier", "Settings");
    settings.setValue("inputMask", m_inputMaskEdit->text());
    settings.setValue("inputPath", m_inputPath);
    settings.setValue("outputPath", m_outputPathEdit->text());
    settings.setValue("deleteInput", m_deleteInputCheckBox->isChecked());
    settings.setValue("fileConflictMode", m_fileConflictComboBox->currentIndex());
    settings.setValue("timerMode", m_timerModeCheckBox->isChecked());
    settings.setValue("timerInterval", m_timerIntervalSpinBox->value());
    settings.setValue("xorValue", m_xorValueEdit->text());
}

void MainWindow::loadSettings()
{
    QSettings settings("FileModifier", "Settings");
    m_inputMaskEdit->setText(settings.value("inputMask", "*.txt").toString());
    m_inputPath = settings.value("inputPath", "").toString();
    m_inputPathEdit->setText(m_inputPath);
    m_outputPathEdit->setText(settings.value("outputPath", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    m_deleteInputCheckBox->setChecked(settings.value("deleteInput", false).toBool());
    m_fileConflictComboBox->setCurrentIndex(settings.value("fileConflictMode", 0).toInt());
    m_timerModeCheckBox->setChecked(settings.value("timerMode", false).toBool());
    m_timerIntervalSpinBox->setValue(settings.value("timerInterval", 5000).toInt());
    m_xorValueEdit->setText(settings.value("xorValue", "0123456789ABCDEF").toString());
}

bool MainWindow::isValidXorValue(const QString &value)
{
    if (value.length() != 16) return false;
    return QRegExp("^[0-9A-Fa-f]{16}$").exactMatch(value);
}

QByteArray MainWindow::parseXorValue(const QString &value)
{
    return QByteArray::fromHex(value.toUtf8());
}

bool MainWindow::isValidXorValue(const QString &value)
{
    if (value.length() != 16) return false;
    return QRegExp("^[0-9A-Fa-f]{16}$").exactMatch(value);
} 