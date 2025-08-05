#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QMutex>
#include <QWaitCondition>

class FileProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FileProcessor(QObject *parent = nullptr);
    ~FileProcessor();

    void setInputMask(const QString &mask);
    void setOutputPath(const QString &path);
    void setDeleteInput(bool deleteInput);
    void setFileConflictMode(int mode);
    void setXorValue(const QByteArray &value);
    void setInputPath(const QString &path);

public slots:
    void startProcessing();
    void stopProcessing();

signals:
    void progressChanged(int progress);
    void processingFinished();
    void processingError(const QString &error);
    void fileProcessed(const QString &filename);
    void statusChanged(const QString &status);

private:
    QString m_inputMask;
    QString m_outputPath;
    QString m_inputPath;
    bool m_deleteInput;
    int m_fileConflictMode;
    QByteArray m_xorValue;
    bool m_stopRequested;
    QMutex m_mutex;
    QWaitCondition m_condition;
    
    QStringList findFiles();
    QString generateOutputFileName(const QString &inputFile);
    bool processFile(const QString &inputFile, const QString &outputFile);
    QByteArray xorData(const QByteArray &data);
    bool isValidXorValue(const QString &value);
    QByteArray parseXorValue(const QString &value);
};

#endif // FILEPROCESSOR_H 