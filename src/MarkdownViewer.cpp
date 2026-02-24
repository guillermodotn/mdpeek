#include "MarkdownViewer.h"

#include <QFile>
#include <QFileInfo>
#include <QScrollBar>
#include <QRegularExpression>

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
    html = transformAlerts(html);
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

QString MarkdownViewer::transformAlerts(const QString &html)
{
    // GitHub alert/admonition syntax: > [!TYPE]\n> content
    // cmark-gfm renders these as normal blockquotes with a <p> starting
    // with "[!TYPE]". We detect that pattern and replace the blockquote
    // with a styled div.
    //
    // cmark-gfm output looks like:
    //   <blockquote>\n<p>[!TIP]\nSome text...</p>\n</blockquote>

    struct AlertType {
        const char *tag;
        const char *label;
        const char *borderColor;
        const char *titleColor;
        const char *icon;  // Unicode icon similar to GitHub's octicons
    };

    static const AlertType alerts[] = {
        {"NOTE",      "Note",      "#0969da", "#0969da", "\xe2\x84\xb9\xef\xb8\x8f"},   // info
        {"TIP",       "Tip",       "#1a7f37", "#1a7f37", "\xf0\x9f\x92\xa1"},             // bulb
        {"IMPORTANT", "Important", "#8250df", "#8250df", "\xe2\x9d\x97"},                 // exclamation
        {"WARNING",   "Warning",   "#9a6700", "#9a6700", "\xe2\x9a\xa0\xef\xb8\x8f"},    // warning
        {"CAUTION",   "Caution",   "#cf222e", "#d1242f", "\xf0\x9f\x94\xb4"},             // red circle
    };

    QString result = html;

    for (const auto &alert : alerts) {
        // Match: <blockquote>\n<p>[!TYPE]\n or <blockquote>\n<p>[!TYPE]<br> or similar
        // The pattern needs to be flexible since cmark-gfm may use \n or <br /> between
        // the [!TYPE] marker and the content.
        QString pattern = QStringLiteral(
            "<blockquote>\\s*<p>\\[!%1\\]\\s*(?:<br\\s*/?>)?\\s*"
        ).arg(QString::fromUtf8(alert.tag));

        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(result);

        while (match.hasMatch()) {
            // Find the closing </blockquote> for this match
            int startPos = match.capturedStart();
            int contentStart = match.capturedEnd();

            // Find the matching </blockquote>
            int closeTag = result.indexOf(QStringLiteral("</blockquote>"), contentStart);
            if (closeTag < 0) break;

            // Extract the inner content (everything between our match end and </blockquote>)
            // We need to also strip the closing </p> that wraps the first paragraph
            QString inner = result.mid(contentStart, closeTag - contentStart);

            // Build the alert box HTML using inline styles (since QTextBrowser
            // has limited class/selector support, inline is most reliable)
            QString replacement = QStringLiteral(
                "<div style='border-left: 4px solid %1; padding: 8px 16px; "
                "margin: 0 0 16px 0; color: #1f2328;'>"
                "<p style='font-weight: 600; color: %2; margin-bottom: 8px;'>"
                "%3 %4</p>"
                "%5"
                "</div>"
            ).arg(
                QString::fromUtf8(alert.borderColor),
                QString::fromUtf8(alert.titleColor),
                QString::fromUtf8(alert.icon),
                QString::fromUtf8(alert.label),
                inner
            );

            result.replace(startPos, closeTag + 13 /* len("</blockquote>") */ - startPos, replacement);

            // Search again in case there are more of the same type
            match = re.match(result, startPos + replacement.size());
        }
    }

    return result;
}

QString MarkdownViewer::wrapHtml(const QString &body)
{
    // GitHub-style stylesheet using only QTextBrowser-compatible CSS.
    // Color values are resolved from github-markdown-css (alpha hex
    // blended against #ffffff where needed).
    static const QString tmpl = QStringLiteral(R"(
<!DOCTYPE html>
<html>
<head>
<style>
body {
    color: #1f2328;
    background-color: #ffffff;
    font-family: "Segoe UI", "Noto Sans", Helvetica, Arial, sans-serif;
    font-size: 16px;
    font-weight: 400;
    line-height: 1.5;
    margin: 32px;
}

/* Headings */
h1, h2, h3, h4, h5, h6 {
    margin-top: 24px;
    margin-bottom: 16px;
    font-weight: 600;
    line-height: 1.25;
}
h1 {
    font-size: 32px;
    padding-bottom: 10px;
    border-bottom: 1px solid #d8dfe5;
}
h2 {
    font-size: 24px;
    padding-bottom: 7px;
    border-bottom: 1px solid #d8dfe5;
}
h3 { font-size: 20px; }
h4 { font-size: 16px; }
h5 { font-size: 14px; }
h6 { font-size: 14px; color: #59636e; }

/* Paragraphs */
p {
    margin-top: 0;
    margin-bottom: 16px;
}

/* Links */
a {
    color: #0969da;
    text-decoration: underline;
}

/* Bold */
b, strong { font-weight: 600; }

/* Inline code */
code {
    font-family: Consolas, "Liberation Mono", Menlo, monospace;
    font-size: 13px;
    padding: 3px 6px;
    margin: 0;
    background-color: #edeef0;
}

/* Code blocks */
pre {
    font-family: Consolas, "Liberation Mono", Menlo, monospace;
    font-size: 13px;
    line-height: 1.45;
    color: #1f2328;
    background-color: #f6f8fa;
    padding: 16px;
    margin-top: 0;
    margin-bottom: 16px;
    border: 1px solid #d1d9e0;
}
pre code {
    padding: 0;
    margin: 0;
    background-color: transparent;
    border: 0;
    font-size: 13px;
}

/* Blockquote */
blockquote {
    margin: 0 0 16px 0;
    padding: 0 16px;
    color: #59636e;
    border-left: 4px solid #d1d9e0;
}

/* Tables */
table {
    border-collapse: collapse;
    margin-top: 0;
    margin-bottom: 16px;
}
th, td {
    padding: 6px 13px;
    border: 1px solid #d1d9e0;
}
th {
    font-weight: 600;
    background-color: #f6f8fa;
}
tr {
    background-color: #ffffff;
}

/* Lists */
ul, ol {
    margin-top: 0;
    margin-bottom: 16px;
    padding-left: 32px;
}
li {
    margin-top: 4px;
}

/* Horizontal rule — GitHub renders this as a solid 4px bar */
hr {
    height: 4px;
    padding: 0;
    margin: 24px 0;
    background-color: #d1d9e0;
    border: 0;
}

/* Images */
img {
    border: 0;
}

/* Strikethrough */
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
