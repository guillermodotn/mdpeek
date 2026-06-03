#ifndef VIEWER_H
#define VIEWER_H

#ifdef MDPEEK_USE_QT

#include <QMainWindow>
#include <QWebEngineView>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QString>

/* Custom QWebEnginePage to intercept navigation (open links in browser,
 * re-render on reload instead of showing raw markdown). */
class MdpeekPage : public QWebEnginePage {
    Q_OBJECT
public:
    explicit MdpeekPage(QObject *parent = nullptr);
    void setFilePath(const QString &path) { m_filePath = path; }

signals:
    void reloadRequested();

protected:
    bool acceptNavigationRequest(const QUrl &url,
                                 NavigationType type,
                                 bool isMainFrame) override;
private:
    QString m_filePath;
};

class MdpeekViewer : public QMainWindow {
    Q_OBJECT
public:
    explicit MdpeekViewer(const QString &filePath, QWidget *parent = nullptr);
    void loadFile();

private slots:
    void onFileChanged(const QString &path);
    void reloadFile();

private:
    QString              m_filePath;
    QWebEngineView      *m_view;
    MdpeekPage          *m_page;
    QFileSystemWatcher  *m_watcher;
    QTimer              *m_debounceTimer;
    double               m_scrollY;
};

#else /* GTK */

#include <adwaita.h>
#include <webkit/webkit.h>

typedef struct {
    AdwApplicationWindow *window;
    WebKitWebView        *webview;
    GFileMonitor         *monitor;
    char                 *file_path;
    guint                 reload_source_id;
    double                scroll_y;
} MdpeekViewer;

MdpeekViewer *viewer_new(AdwApplication *app, const char *file_path);
void          viewer_load_file(MdpeekViewer *viewer);

#endif /* MDPEEK_USE_QT */

#endif /* VIEWER_H */
