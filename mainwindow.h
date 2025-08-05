#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QProgressBar>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include "fileprocessor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onBrowseInputPathClicked();
    void onBrowseOutputPathClicked();
    void onTimerTimeout();
    void onProcessingProgress(int progress);
    void onProcessingFinished();
    void onProcessingError(const QString &error);
    void onFileProcessed(const QString &filename);
    void saveSettings();
    void loadSettings();

private:
    Ui::MainWindow *ui;
    QTimer *m_timer;
    FileProcessor *m_processor;
    QThread *m_processorThread;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QTextEdit *m_logTextEdit;
    
    // UI Elements
    QLineEdit *m_inputMaskEdit;
    QLineEdit *m_inputPathEdit;
    QCheckBox *m_deleteInputCheckBox;
    QLineEdit *m_outputPathEdit;
    QComboBox *m_fileConflictComboBox;
    QCheckBox *m_timerModeCheckBox;
    QSpinBox *m_timerIntervalSpinBox;
    QLineEdit *m_xorValueEdit;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QPushButton *m_browseInputButton;
    QPushButton *m_browseOutputButton;
    QString m_inputPath;
    
    void setupUI();
    void connectSignals();
    void updateUIState(bool processing);
    bool validateInputs();
    QString getXorValue();
    bool isValidXorValue(const QString &value);
    QByteArray parseXorValue(const QString &value);
};

#endif // MAINWINDOW_H 