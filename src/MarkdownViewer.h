#ifndef MARKDOWNVIEWER_H
#define MARKDOWNVIEWER_H

#include <QMainWindow>
#include <QFileSystemWatcher>
#include <QTextBrowser>
#include <QTimer>
#include <QString>

class MarkdownViewer : public QMainWindow {
    Q_OBJECT

public:
    explicit MarkdownViewer(const QString &filePath, QWidget *parent = nullptr);

private slots:
    void onFileChanged(const QString &path);
    void reloadFile();

private:
    void loadFile();
    QString renderMarkdown(const QByteArray &markdown);
    QString wrapHtml(const QString &body);

    QString m_filePath;
    QTextBrowser *m_browser;
    QFileSystemWatcher *m_watcher;
    QTimer *m_debounceTimer;
};

#endif // MARKDOWNVIEWER_H
