#include "MarkdownViewer.h"

#include <QFile>
#include <QFileInfo>
#include <QScrollBar>

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

MarkdownViewer::MarkdownViewer(const QString &filePath, QWidget *parent)
    : QMainWindow(parent)
    , m_filePath(filePath)
{
    // Window setup
    QFileInfo fi(m_filePath);
    setWindowTitle(fi.fileName() + QString::fromUtf8(" \xe2\x80\x94 mdpeek"));
    resize(800, 600);

    // Browser widget
    m_browser = new QTextBrowser(this);
    m_browser->setOpenExternalLinks(true);
    setCentralWidget(m_browser);

    // File watcher
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(m_filePath);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &MarkdownViewer::onFileChanged);

    // Debounce timer — editors often do atomic saves (write tmp + rename)
    // which can fire multiple signals in rapid succession
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(150);
    connect(m_debounceTimer, &QTimer::timeout,
            this, &MarkdownViewer::reloadFile);

    // Initial load
    loadFile();
}

void MarkdownViewer::onFileChanged(const QString &path)
{
    Q_UNUSED(path);

    // Re-add the watch — many editors delete + recreate the file (atomic save),
    // which causes QFileSystemWatcher to drop the path.
    if (!m_watcher->files().contains(m_filePath)) {
        // Small delay to let the editor finish writing the new file
        QTimer::singleShot(50, this, [this]() {
            if (QFile::exists(m_filePath)) {
                m_watcher->addPath(m_filePath);
            }
        });
    }

    m_debounceTimer->start();
}

void MarkdownViewer::reloadFile()
{
    // Preserve scroll position across reloads
    int scrollPos = m_browser->verticalScrollBar()->value();
    loadFile();
    m_browser->verticalScrollBar()->setValue(scrollPos);
}

void MarkdownViewer::loadFile()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_browser->setHtml(
            QStringLiteral("<html><body><p style='color:red;'>Failed to open: %1</p></body></html>")
                .arg(m_filePath));
        return;
    }

    QByteArray raw = file.readAll();
    file.close();

    QString html = renderMarkdown(raw);
    m_browser->setHtml(wrapHtml(html));
}

QString MarkdownViewer::renderMarkdown(const QByteArray &markdown)
{
    // Register GFM extensions (idempotent — safe to call multiple times)
    cmark_gfm_core_extensions_ensure_registered();

    int options = CMARK_OPT_UNSAFE | CMARK_OPT_SMART;

    cmark_parser *parser = cmark_parser_new(options);

    // Attach GFM extensions
    static const char *extNames[] = {
        "table", "strikethrough", "autolink", "tasklist", "tagfilter"
    };
    for (const char *name : extNames) {
        cmark_syntax_extension *ext = cmark_find_syntax_extension(name);
        if (ext) {
            cmark_parser_attach_syntax_extension(parser, ext);
        }
    }

    cmark_parser_feed(parser, markdown.constData(), markdown.size());
    cmark_node *doc = cmark_parser_finish(parser);

    char *html = cmark_render_html(doc, options,
                                   cmark_parser_get_syntax_extensions(parser));
    QString result = QString::fromUtf8(html);

    free(html);
    cmark_node_free(doc);
    cmark_parser_free(parser);

    return result;
}

QString MarkdownViewer::wrapHtml(const QString &body)
{
    // Minimal stylesheet that works within QTextBrowser's CSS subset.
    // QTextBrowser supports basic CSS: font, color, margin, padding,
    // border, background, text-align — but no flexbox, grid, etc.
    static const QString tmpl = QStringLiteral(R"(
<!DOCTYPE html>
<html>
<head>
<style>
body {
    font-family: -apple-system, "Segoe UI", Helvetica, Arial, sans-serif;
    font-size: 15px;
    line-height: 1.6;
    color: #1f2328;
    margin: 20px;
}
h1, h2, h3, h4, h5, h6 {
    margin-top: 24px;
    margin-bottom: 16px;
    font-weight: 600;
    line-height: 1.25;
}
h1 { font-size: 2em; border-bottom: 1px solid #d1d9e0; padding-bottom: 0.3em; }
h2 { font-size: 1.5em; border-bottom: 1px solid #d1d9e0; padding-bottom: 0.3em; }
h3 { font-size: 1.25em; }
code {
    font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, monospace;
    font-size: 85%;
    background-color: #eff1f3;
    padding: 0.2em 0.4em;
    border-radius: 3px;
}
pre {
    background-color: #f6f8fa;
    border: 1px solid #d1d9e0;
    border-radius: 6px;
    padding: 16px;
    overflow: auto;
    font-size: 85%;
    line-height: 1.45;
}
pre code {
    background: transparent;
    padding: 0;
}
blockquote {
    margin: 0;
    padding: 0 16px;
    color: #656d76;
    border-left: 4px solid #d1d9e0;
}
table {
    border-collapse: collapse;
    margin: 16px 0;
}
th, td {
    border: 1px solid #d1d9e0;
    padding: 6px 13px;
}
th {
    font-weight: 600;
    background-color: #f6f8fa;
}
a {
    color: #0969da;
    text-decoration: none;
}
hr {
    border: none;
    border-top: 1px solid #d1d9e0;
    margin: 24px 0;
}
img {
    max-width: 100%;
}
ul, ol {
    padding-left: 2em;
}
li {
    margin: 0.25em 0;
}
del {
    text-decoration: line-through;
}
</style>
</head>
<body>
%1
</body>
</html>
)");

    return tmpl.arg(body);
}
