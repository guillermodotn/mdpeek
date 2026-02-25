#include "MarkdownViewer.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QWebEnginePage>

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

MarkdownViewer::MarkdownViewer(const QString &filePath, QWidget *parent)
    : QMainWindow(parent)
    , m_filePath(filePath)
{
    // Window setup
    QFileInfo fi(m_filePath);
    setWindowTitle(fi.fileName() + QString::fromUtf8(" \xe2\x80\x94 mdpeek"));
    resize(900, 700);

    // WebEngine view
    m_view = new QWebEngineView(this);
    setCentralWidget(m_view);

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
    // Preserve scroll position across reloads.
    // QWebEngineView runs async — we need to grab the scroll position via JS,
    // then reload, then restore after the page finishes loading.
    m_view->page()->runJavaScript(
        QStringLiteral("window.scrollY"),
        [this](const QVariant &result) {
            int scrollY = result.toInt();
            loadFile();
            // Restore scroll after the new content loads
            connect(m_view, &QWebEngineView::loadFinished, this,
                [this, scrollY](bool ok) {
                    Q_UNUSED(ok);
                    m_view->page()->runJavaScript(
                        QStringLiteral("window.scrollTo(0, %1)").arg(scrollY));
                    // Disconnect so this lambda only runs once
                    disconnect(m_view, &QWebEngineView::loadFinished, this, nullptr);
                });
        });
}

void MarkdownViewer::loadFile()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_view->setHtml(
            QStringLiteral("<html><body><p style='color:red;'>Failed to open: %1</p></body></html>")
                .arg(m_filePath));
        return;
    }

    QByteArray raw = file.readAll();
    file.close();

    QString html = renderMarkdown(raw);
    html = transformAlerts(html);
    QString fullHtml = wrapHtml(html);

    // setHtml has a 2MB limit due to data: URL encoding.
    // For most markdown files this is fine, but use setContent for safety.
    m_view->setContent(fullHtml.toUtf8(), QStringLiteral("text/html"));
}

QString MarkdownViewer::renderMarkdown(const QByteArray &markdown)
{
    cmark_gfm_core_extensions_ensure_registered();

    int options = CMARK_OPT_UNSAFE | CMARK_OPT_SMART;

    cmark_parser *parser = cmark_parser_new(options);

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
    // GitHub alert syntax: > [!TYPE]\n> content
    // cmark-gfm renders these as normal blockquotes with [!TYPE] as text.
    // We detect and replace with proper GitHub alert markup.

    struct AlertType {
        const char *tag;
        const char *label;
        const char *cssClass;
        // SVG icons matching GitHub's octicons
        const char *svg;
    };

    static const AlertType alerts[] = {
        {"NOTE", "Note", "note",
         "<svg viewBox='0 0 16 16' width='16' height='16' style='display:inline-block;fill:currentColor;vertical-align:text-bottom;margin-right:8px'>"
         "<path d='M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8Zm8-6.5a6.5 6.5 0 1 0 0 13 6.5 6.5 0 0 0 0-13ZM6.5 7.75A.75.75 0 0 1 7.25 7h1a.75.75 0 0 1 .75.75v2.75h.25a.75.75 0 0 1 0 1.5h-2a.75.75 0 0 1 0-1.5h.25v-2h-.25a.75.75 0 0 1-.75-.75ZM8 6a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z'></path></svg>"},
        {"TIP", "Tip", "tip",
         "<svg viewBox='0 0 16 16' width='16' height='16' style='display:inline-block;fill:currentColor;vertical-align:text-bottom;margin-right:8px'>"
         "<path d='M8 1.5c-2.363 0-4 1.69-4 3.75 0 .984.424 1.625.984 2.304l.214.253c.223.264.47.556.673.848.284.411.537.896.621 1.49a.75.75 0 0 1-1.484.211c-.04-.282-.163-.547-.37-.847a8.456 8.456 0 0 0-.542-.68c-.084-.1-.173-.205-.268-.32C3.201 7.75 2.5 6.766 2.5 5.25 2.5 2.31 4.863 0 8 0s5.5 2.31 5.5 5.25c0 1.516-.701 2.5-1.328 3.259-.095.115-.184.22-.268.319-.207.245-.383.453-.541.681-.208.3-.33.565-.37.847a.751.751 0 0 1-1.485-.212c.084-.593.337-1.078.621-1.489.203-.292.45-.584.673-.848.075-.088.147-.173.213-.253.561-.679.985-1.32.985-2.304 0-2.06-1.637-3.75-4-3.75ZM5.75 12h4.5a.75.75 0 0 1 0 1.5h-4.5a.75.75 0 0 1 0-1.5ZM6 15.25a.75.75 0 0 1 .75-.75h2.5a.75.75 0 0 1 0 1.5h-2.5a.75.75 0 0 1-.75-.75Z'></path></svg>"},
        {"IMPORTANT", "Important", "important",
         "<svg viewBox='0 0 16 16' width='16' height='16' style='display:inline-block;fill:currentColor;vertical-align:text-bottom;margin-right:8px'>"
         "<path d='M0 1.75C0 .784.784 0 1.75 0h12.5C15.216 0 16 .784 16 1.75v9.5A1.75 1.75 0 0 1 14.25 13H8.06l-2.573 2.573A1.458 1.458 0 0 1 3 14.543V13H1.75A1.75 1.75 0 0 1 0 11.25Zm1.75-.25a.25.25 0 0 0-.25.25v9.5c0 .138.112.25.25.25h2a.75.75 0 0 1 .75.75v2.19l2.72-2.72a.749.749 0 0 1 .53-.22h6.5a.25.25 0 0 0 .25-.25v-9.5a.25.25 0 0 0-.25-.25Zm7 2.25v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 9a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z'></path></svg>"},
        {"WARNING", "Warning", "warning",
         "<svg viewBox='0 0 16 16' width='16' height='16' style='display:inline-block;fill:currentColor;vertical-align:text-bottom;margin-right:8px'>"
         "<path d='M6.457 1.047c.659-1.234 2.427-1.234 3.086 0l6.082 11.378A1.75 1.75 0 0 1 14.082 15H1.918a1.75 1.75 0 0 1-1.543-2.575Zm1.763.707a.25.25 0 0 0-.44 0L1.698 13.132a.25.25 0 0 0 .22.368h12.164a.25.25 0 0 0 .22-.368Zm.53 3.996v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 11a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z'></path></svg>"},
        {"CAUTION", "Caution", "caution",
         "<svg viewBox='0 0 16 16' width='16' height='16' style='display:inline-block;fill:currentColor;vertical-align:text-bottom;margin-right:8px'>"
         "<path d='M4.47.22A.749.749 0 0 1 5 0h6c.199 0 .389.079.53.22l4.25 4.25c.141.14.22.331.22.53v6a.749.749 0 0 1-.22.53l-4.25 4.25A.749.749 0 0 1 11 16H5a.749.749 0 0 1-.53-.22L.22 11.53A.749.749 0 0 1 0 11V5c0-.199.079-.389.22-.53Zm.84 1.28L1.5 5.31v5.38l3.81 3.81h5.38l3.81-3.81V5.31L10.69 1.5ZM8 4a.75.75 0 0 1 .75.75v3.5a.75.75 0 0 1-1.5 0v-3.5A.75.75 0 0 1 8 4Zm0 8a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z'></path></svg>"},
    };

    QString result = html;

    for (const auto &alert : alerts) {
        QString pattern = QStringLiteral(
            "<blockquote>\\s*<p>\\[!%1\\]\\s*(?:<br\\s*/?>)?\\s*"
        ).arg(QString::fromUtf8(alert.tag));

        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(result);

        while (match.hasMatch()) {
            int startPos = match.capturedStart();
            int contentStart = match.capturedEnd();

            int closeTag = result.indexOf(QStringLiteral("</blockquote>"), contentStart);
            if (closeTag < 0) break;

            QString inner = result.mid(contentStart, closeTag - contentStart);

            QString replacement = QStringLiteral(
                "<div class='markdown-alert markdown-alert-%1'>"
                "<p class='markdown-alert-title'>%2%3</p>"
                "%4"
                "</div>"
            ).arg(
                QString::fromUtf8(alert.cssClass),
                QString::fromUtf8(alert.svg),
                QString::fromUtf8(alert.label),
                inner
            );

            result.replace(startPos, closeTag + 13 - startPos, replacement);
            match = re.match(result, startPos + replacement.size());
        }
    }

    return result;
}

QString MarkdownViewer::wrapHtml(const QString &body)
{
    // Full github-markdown-css (light theme) from
    // https://github.com/sindresorhus/github-markdown-css
    // plus GitHub's alert/admonition styles.
    static const QString tmpl = QStringLiteral(R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
/* === github-markdown-css (light) === */
.markdown-body {
  -ms-text-size-adjust: 100%;
  -webkit-text-size-adjust: 100%;
  margin: 0;
  font-weight: 400;
  color: #1f2328;
  background-color: #ffffff;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Noto Sans",
               Helvetica, Arial, sans-serif, "Apple Color Emoji", "Segoe UI Emoji";
  font-size: 16px;
  line-height: 1.5;
  word-wrap: break-word;
}
.markdown-body a {
  background-color: transparent;
  color: #0969da;
  text-decoration: none;
}
.markdown-body a:hover {
  text-decoration: underline;
}
.markdown-body b,
.markdown-body strong {
  font-weight: 600;
}
.markdown-body dfn {
  font-style: italic;
}
.markdown-body h1 {
  margin: .67em 0;
  font-weight: 600;
  padding-bottom: .3em;
  font-size: 2em;
  border-bottom: 1px solid #d1d9e0b3;
}
.markdown-body mark {
  background-color: #fff8c5;
  color: #1f2328;
}
.markdown-body small {
  font-size: 90%;
}
.markdown-body sub,
.markdown-body sup {
  font-size: 75%;
  line-height: 0;
  position: relative;
  vertical-align: baseline;
}
.markdown-body sub { bottom: -0.25em; }
.markdown-body sup { top: -0.5em; }
.markdown-body img {
  border-style: none;
  max-width: 100%;
  box-sizing: content-box;
}
.markdown-body code,
.markdown-body kbd,
.markdown-body pre,
.markdown-body samp {
  font-family: monospace;
  font-size: 1em;
}
.markdown-body hr {
  box-sizing: content-box;
  overflow: hidden;
  background: transparent;
  border-bottom: 1px solid #d1d9e0b3;
  height: .25em;
  padding: 0;
  margin: 1.5rem 0;
  background-color: #d1d9e0;
  border: 0;
}
.markdown-body h1,
.markdown-body h2,
.markdown-body h3,
.markdown-body h4,
.markdown-body h5,
.markdown-body h6 {
  margin-top: 1.5rem;
  margin-bottom: 1rem;
  font-weight: 600;
  line-height: 1.25;
}
.markdown-body h2 {
  font-weight: 600;
  padding-bottom: .3em;
  font-size: 1.5em;
  border-bottom: 1px solid #d1d9e0b3;
}
.markdown-body h3 { font-weight: 600; font-size: 1.25em; }
.markdown-body h4 { font-weight: 600; font-size: 1em; }
.markdown-body h5 { font-weight: 600; font-size: .875em; }
.markdown-body h6 { font-weight: 600; font-size: .85em; color: #59636e; }
.markdown-body p {
  margin-top: 0;
  margin-bottom: 10px;
}
.markdown-body blockquote {
  margin: 0;
  padding: 0 1em;
  color: #59636e;
  border-left: .25em solid #d1d9e0;
}
.markdown-body ul,
.markdown-body ol {
  margin-top: 0;
  margin-bottom: 0;
  padding-left: 2em;
}
.markdown-body ol ol,
.markdown-body ul ol {
  list-style-type: lower-roman;
}
.markdown-body ul ul ol,
.markdown-body ul ol ol,
.markdown-body ol ul ol,
.markdown-body ol ol ol {
  list-style-type: lower-alpha;
}
.markdown-body dd { margin-left: 0; }
.markdown-body tt,
.markdown-body code,
.markdown-body samp {
  font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas,
               Liberation Mono, monospace;
  font-size: 12px;
}
.markdown-body pre {
  margin-top: 0;
  margin-bottom: 0;
  font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas,
               Liberation Mono, monospace;
  font-size: 12px;
  word-wrap: normal;
}
.markdown-body>*:first-child { margin-top: 0 !important; }
.markdown-body>*:last-child { margin-bottom: 0 !important; }
.markdown-body a:not([href]) {
  color: inherit;
  text-decoration: none;
}
.markdown-body p,
.markdown-body blockquote,
.markdown-body ul,
.markdown-body ol,
.markdown-body dl,
.markdown-body table,
.markdown-body pre,
.markdown-body details {
  margin-top: 0;
  margin-bottom: 1rem;
}
.markdown-body blockquote>:first-child { margin-top: 0; }
.markdown-body blockquote>:last-child { margin-bottom: 0; }
.markdown-body h1 tt, .markdown-body h1 code,
.markdown-body h2 tt, .markdown-body h2 code,
.markdown-body h3 tt, .markdown-body h3 code,
.markdown-body h4 tt, .markdown-body h4 code,
.markdown-body h5 tt, .markdown-body h5 code,
.markdown-body h6 tt, .markdown-body h6 code {
  padding: 0 .2em;
  font-size: inherit;
}
.markdown-body ul.no-list,
.markdown-body ol.no-list {
  padding: 0;
  list-style-type: none;
}
.markdown-body ul ul,
.markdown-body ul ol,
.markdown-body ol ol,
.markdown-body ol ul {
  margin-top: 0;
  margin-bottom: 0;
}
.markdown-body li>p { margin-top: 1rem; }
.markdown-body li+li { margin-top: .25em; }
.markdown-body dl { padding: 0; }
.markdown-body dl dt {
  padding: 0;
  margin-top: 1rem;
  font-size: 1em;
  font-style: italic;
  font-weight: 600;
}
.markdown-body dl dd {
  padding: 0 1rem;
  margin-bottom: 1rem;
}

/* Tables */
.markdown-body table {
  border-spacing: 0;
  border-collapse: collapse;
  display: block;
  width: max-content;
  max-width: 100%;
  overflow: auto;
  font-variant: tabular-nums;
}
.markdown-body table th { font-weight: 600; }
.markdown-body table th,
.markdown-body table td {
  padding: 6px 13px;
  border: 1px solid #d1d9e0;
}
.markdown-body table td>:last-child { margin-bottom: 0; }
.markdown-body table tr {
  background-color: #ffffff;
  border-top: 1px solid #d1d9e0b3;
}
.markdown-body table tr:nth-child(2n) {
  background-color: #f6f8fa;
}

/* Code */
.markdown-body code,
.markdown-body tt {
  padding: .2em .4em;
  margin: 0;
  font-size: 85%;
  white-space: break-spaces;
  background-color: #818b981f;
  border-radius: 6px;
}
.markdown-body code br,
.markdown-body tt br { display: none; }
.markdown-body del code { text-decoration: inherit; }
.markdown-body samp { font-size: 85%; }
.markdown-body pre code { font-size: 100%; }
.markdown-body pre>code {
  padding: 0;
  margin: 0;
  word-break: normal;
  white-space: pre;
  background: transparent;
  border: 0;
}
.markdown-body .highlight { margin-bottom: 1rem; }
.markdown-body .highlight pre { margin-bottom: 0; word-break: normal; }
.markdown-body .highlight pre,
.markdown-body pre {
  padding: 1rem;
  overflow: auto;
  font-size: 85%;
  line-height: 1.45;
  color: #1f2328;
  background-color: #f6f8fa;
  border-radius: 6px;
}
.markdown-body pre code,
.markdown-body pre tt {
  display: inline;
  padding: 0;
  margin: 0;
  overflow: visible;
  line-height: inherit;
  word-wrap: normal;
  background-color: transparent;
  border: 0;
}

/* Task lists */
.markdown-body .task-list-item {
  list-style-type: none;
}
.markdown-body .task-list-item label { font-weight: 400; }
.markdown-body .task-list-item+.task-list-item { margin-top: 0.25rem; }
.markdown-body .task-list-item-checkbox {
  margin: 0 .2em .25em -1.4em;
  vertical-align: middle;
}

/* Footnotes */
.markdown-body .footnotes {
  font-size: 12px;
  color: #59636e;
  border-top: 1px solid #d1d9e0;
}
.markdown-body .footnotes ol { padding-left: 1rem; }

/* KBD */
.markdown-body kbd {
  display: inline-block;
  padding: 0.25rem;
  font: 11px ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas,
       Liberation Mono, monospace;
  line-height: 10px;
  color: #1f2328;
  vertical-align: middle;
  background-color: #f6f8fa;
  border: solid 1px #d1d9e0;
  border-bottom-color: #d1d9e0;
  border-radius: 6px;
  box-shadow: inset 0 -1px 0 #d1d9e0;
}

/* === GitHub Alerts / Admonitions === */
.markdown-body .markdown-alert {
  padding: 0.5rem 1rem;
  margin-bottom: 1rem;
  color: inherit;
  border-left: .25em solid #d1d9e0;
}
.markdown-body .markdown-alert>:first-child { margin-top: 0; }
.markdown-body .markdown-alert>:last-child { margin-bottom: 0; }
.markdown-body .markdown-alert .markdown-alert-title {
  display: flex;
  font-weight: 500;
  align-items: center;
  line-height: 1;
}
.markdown-body .markdown-alert.markdown-alert-note {
  border-left-color: #0969da;
}
.markdown-body .markdown-alert.markdown-alert-note .markdown-alert-title {
  color: #0969da;
}
.markdown-body .markdown-alert.markdown-alert-tip {
  border-left-color: #1a7f37;
}
.markdown-body .markdown-alert.markdown-alert-tip .markdown-alert-title {
  color: #1a7f37;
}
.markdown-body .markdown-alert.markdown-alert-important {
  border-left-color: #8250df;
}
.markdown-body .markdown-alert.markdown-alert-important .markdown-alert-title {
  color: #8250df;
}
.markdown-body .markdown-alert.markdown-alert-warning {
  border-left-color: #9a6700;
}
.markdown-body .markdown-alert.markdown-alert-warning .markdown-alert-title {
  color: #9a6700;
}
.markdown-body .markdown-alert.markdown-alert-caution {
  border-left-color: #cf222e;
}
.markdown-body .markdown-alert.markdown-alert-caution .markdown-alert-title {
  color: #d1242f;
}

/* === Syntax highlighting (prettylights) === */
.markdown-body .pl-c { color: #59636e; }
.markdown-body .pl-c1,
.markdown-body .pl-s .pl-v { color: #0550ae; }
.markdown-body .pl-e,
.markdown-body .pl-en { color: #6639ba; }
.markdown-body .pl-smi,
.markdown-body .pl-s .pl-s1 { color: #1f2328; }
.markdown-body .pl-ent { color: #0550ae; }
.markdown-body .pl-k { color: #cf222e; }
.markdown-body .pl-s,
.markdown-body .pl-pds,
.markdown-body .pl-s .pl-pse .pl-s1,
.markdown-body .pl-sr,
.markdown-body .pl-sr .pl-cce,
.markdown-body .pl-sr .pl-sre,
.markdown-body .pl-sr .pl-sra { color: #0a3069; }
.markdown-body .pl-v,
.markdown-body .pl-smw { color: #953800; }
.markdown-body .pl-bu { color: #82071e; }
.markdown-body .pl-sr .pl-cce { font-weight: bold; color: #116329; }
.markdown-body .pl-ml { color: #3b2300; }
.markdown-body .pl-mh,
.markdown-body .pl-mh .pl-en,
.markdown-body .pl-ms { font-weight: bold; color: #0550ae; }
.markdown-body .pl-mi { font-style: italic; color: #1f2328; }
.markdown-body .pl-mb { font-weight: bold; color: #1f2328; }
.markdown-body .pl-md { color: #82071e; background-color: #ffebe9; }
.markdown-body .pl-mi1 { color: #116329; background-color: #dafbe1; }
.markdown-body .pl-mc { color: #953800; background-color: #ffd8b5; }
.markdown-body .pl-mdr { font-weight: bold; color: #8250df; }
.markdown-body .pl-ba { color: #59636e; }

/* === Page layout — matches GitHub's README container === */
body {
  background-color: #ffffff;
  margin: 0;
  padding: 0;
}
.container {
  max-width: 1012px;
  margin: 0 auto;
  padding: 32px 32px;
}
</style>
</head>
<body>
<div class="container">
<article class="markdown-body">
%1
</article>
</div>
</body>
</html>
)");

    return tmpl.arg(body);
}
