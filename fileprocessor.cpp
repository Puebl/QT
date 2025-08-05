#include "fileprocessor.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include <QRegExp>
#include <QDebug>
#include <QMutexLocker>

FileProcessor::FileProcessor(QObject *parent)
    : QObject(parent)
    , m_deleteInput(false)
    , m_fileConflictMode(0)
    , m_stopRequested(false)
{
}

FileProcessor::~FileProcessor()
{
    stopProcessing();
}

void FileProcessor::setInputMask(const QString &mask)
{
    m_inputMask = mask;
}

void FileProcessor::setOutputPath(const QString &path)
{
    m_outputPath = path;
}

void FileProcessor::setDeleteInput(bool deleteInput)
{
    m_deleteInput = deleteInput;
}

void FileProcessor::setFileConflictMode(int mode)
{
    m_fileConflictMode = mode;
}

void FileProcessor::setXorValue(const QByteArray &value)
{
    m_xorValue = value;
}

void FileProcessor::setInputPath(const QString &path)
{
    m_inputPath = path;
}

void FileProcessor::startProcessing()
{
    m_stopRequested = false;
    
    emit statusChanged("Поиск файлов...");
    
    QStringList files = findFiles();
    if (files.isEmpty()) {
        emit statusChanged("Файлы не найдены");
        emit processingFinished();
        return;
    }
    
    emit statusChanged(QString("Найдено файлов: %1").arg(files.size()));
    
    int processedCount = 0;
    for (const QString &inputFile : files) {
        if (m_stopRequested) {
            emit statusChanged("Обработка остановлена");
            break;
        }
        
        QFileInfo fileInfo(inputFile);
        QString outputFile = generateOutputFileName(inputFile);
        
        emit statusChanged(QString("Обработка: %1").arg(fileInfo.fileName()));
        
        if (processFile(inputFile, outputFile)) {
            emit fileProcessed(fileInfo.fileName());
            
            if (m_deleteInput) {
                QFile::remove(inputFile);
            }
        }
        
        processedCount++;
        int progress = (processedCount * 100) / files.size();
        emit progressChanged(progress);
    }
    
    emit statusChanged("Обработка завершена");
    emit processingFinished();
}

void FileProcessor::stopProcessing()
{
    QMutexLocker locker(&m_mutex);
    m_stopRequested = true;
    m_condition.wakeAll();
}

QStringList FileProcessor::findFiles()
{
    QStringList files;
    QDir dir(m_inputPath.isEmpty() ? QDir::currentPath() : m_inputPath);
    
    if (!dir.exists()) {
        emit processingError("Указанная папка не существует");
        return files;
    }
    
    // Convert mask to regex pattern
    QString pattern = m_inputMask;
    pattern.replace(".", "\\.");
    pattern.replace("*", ".*");
    pattern.replace("?", ".");
    
    QRegExp regex(pattern, Qt::CaseInsensitive);
    
    QDirIterator it(dir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fileInfo(filePath);
        
        if (regex.exactMatch(fileInfo.fileName())) {
            files.append(filePath);
        }
    }
    
    return files;
}

QString FileProcessor::generateOutputFileName(const QString &inputFile)
{
    QFileInfo inputFileInfo(inputFile);
    QString baseName = inputFileInfo.baseName();
    QString extension = inputFileInfo.suffix();
    QString outputFile = QString("%1/%2.%3").arg(m_outputPath).arg(baseName).arg(extension);
    
    if (m_fileConflictMode == 1) { // Add counter
        int counter = 1;
        QString originalOutputFile = outputFile;
        while (QFile::exists(outputFile)) {
            outputFile = QString("%1/%2_%3.%4").arg(m_outputPath).arg(baseName).arg(counter).arg(extension);
            counter++;
        }
    }
    
    return outputFile;
}

bool FileProcessor::processFile(const QString &inputFile, const QString &outputFile)
{
    QFile input(inputFile);
    if (!input.open(QIODevice::ReadOnly)) {
        emit processingError(QString("Не удалось открыть файл: %1").arg(inputFile));
        return false;
    }
    
    QFile output(outputFile);
    if (!output.open(QIODevice::WriteOnly)) {
        emit processingError(QString("Не удалось создать файл: %1").arg(outputFile));
        input.close();
        return false;
    }
    
    const int bufferSize = 8192; // 8KB buffer
    QByteArray buffer;
    
    while (!input.atEnd() && !m_stopRequested) {
        buffer = input.read(bufferSize);
        if (buffer.isEmpty()) {
            break;
        }
        
        QByteArray processedData = xorData(buffer);
        if (output.write(processedData) == -1) {
            emit processingError(QString("Ошибка записи в файл: %1").arg(outputFile));
            input.close();
            output.close();
            return false;
        }
    }
    
    input.close();
    output.close();
    
    return !m_stopRequested;
}

QByteArray FileProcessor::xorData(const QByteArray &data)
{
    QByteArray result = data;
    
    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ m_xorValue[i % m_xorValue.size()];
    }
    
    return result;
}

bool FileProcessor::isValidXorValue(const QString &value)
{
    if (value.length() != 16) return false;
    return QRegExp("^[0-9A-Fa-f]{16}$").exactMatch(value);
}

QByteArray FileProcessor::parseXorValue(const QString &value)
{
    return QByteArray::fromHex(value.toUtf8());
} 