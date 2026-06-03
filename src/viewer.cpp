#ifdef MDPEEK_USE_QT
#include "viewer.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QShortcut>
#include <QKeySequence>
#include <QUrl>
#include <QWebEngineSettings>

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#else /* GTK */
#include "viewer.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif /* MDPEEK_USE_QT */


/* ── Forward declarations (GTK only — Qt uses class methods) ────────── */

#ifndef MDPEEK_USE_QT
static char    *render_markdown(const char *markdown, size_t len);
static char    *transform_alerts(const char *html);
static char    *wrap_html(const char *body);
static void     on_file_changed(GFileMonitor *monitor, GFile *file,
                                 GFile *other, GFileMonitorEvent event,
                                 gpointer user_data);
static void     on_close_action(GSimpleAction *action, GVariant *param,
                                 gpointer user_data);
static gboolean on_decide_policy(WebKitWebView *web_view,
                                  WebKitPolicyDecision *decision,
                                  WebKitPolicyDecisionType type,
                                  gpointer user_data);
static void     scroll_save_and_reload(MdpeekViewer *v);
#endif /* MDPEEK_USE_QT */


/* ── HTML template ──────────────────────────────────────────────────── */

#ifdef MDPEEK_USE_QT
static const QString HTML_TEMPLATE = QStringLiteral(R"(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
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
.markdown-body a { background-color: transparent; color: #0969da; text-decoration: none; }
.markdown-body a:hover { text-decoration: underline; }
.markdown-body b, .markdown-body strong { font-weight: 600; }
.markdown-body dfn { font-style: italic; }
.markdown-body h1 { margin: .67em 0; font-weight: 600; padding-bottom: .3em; font-size: 2em; border-bottom: 1px solid #d1d9e0b3; }
.markdown-body mark { background-color: #fff8c5; color: #1f2328; }
.markdown-body small { font-size: 90%; }
.markdown-body sub, .markdown-body sup { font-size: 75%; line-height: 0; position: relative; vertical-align: baseline; }
.markdown-body sub { bottom: -0.25em; }
.markdown-body sup { top: -0.5em; }
.markdown-body img { border-style: none; max-width: 100%; box-sizing: content-box; }
.markdown-body code, .markdown-body kbd, .markdown-body pre, .markdown-body samp { font-family: monospace; font-size: 1em; }
.markdown-body hr { box-sizing: content-box; overflow: hidden; background: transparent; border-bottom: 1px solid #d1d9e0b3; height: .25em; padding: 0; margin: 1.5rem 0; background-color: #d1d9e0; border: 0; }
.markdown-body h1, .markdown-body h2, .markdown-body h3, .markdown-body h4, .markdown-body h5, .markdown-body h6 { margin-top: 1.5rem; margin-bottom: 1rem; font-weight: 600; line-height: 1.25; }
.markdown-body h2 { font-weight: 600; padding-bottom: .3em; font-size: 1.5em; border-bottom: 1px solid #d1d9e0b3; }
.markdown-body h3 { font-weight: 600; font-size: 1.25em; }
.markdown-body h4 { font-weight: 600; font-size: 1em; }
.markdown-body h5 { font-weight: 600; font-size: .875em; }
.markdown-body h6 { font-weight: 600; font-size: .85em; color: #59636e; }
.markdown-body p { margin-top: 0; margin-bottom: 10px; }
.markdown-body blockquote { margin: 0; padding: 0 1em; color: #59636e; border-left: .25em solid #d1d9e0; }
.markdown-body ul, .markdown-body ol { margin-top: 0; margin-bottom: 0; padding-left: 2em; }
.markdown-body ol ol, .markdown-body ul ol { list-style-type: lower-roman; }
.markdown-body ul ul ol, .markdown-body ul ol ol, .markdown-body ol ul ol, .markdown-body ol ol ol { list-style-type: lower-alpha; }
.markdown-body dd { margin-left: 0; }
.markdown-body tt, .markdown-body code, .markdown-body samp { font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, Liberation Mono, monospace; font-size: 12px; }
.markdown-body pre { margin-top: 0; margin-bottom: 0; font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, Liberation Mono, monospace; font-size: 12px; word-wrap: normal; }
.markdown-body>*:first-child { margin-top: 0 !important; }
.markdown-body>*:last-child { margin-bottom: 0 !important; }
.markdown-body a:not([href]) { color: inherit; text-decoration: none; }
.markdown-body p, .markdown-body blockquote, .markdown-body ul, .markdown-body ol, .markdown-body dl, .markdown-body table, .markdown-body pre, .markdown-body details { margin-top: 0; margin-bottom: 1rem; }
.markdown-body blockquote>:first-child { margin-top: 0; }
.markdown-body blockquote>:last-child { margin-bottom: 0; }
.markdown-body h1 tt, .markdown-body h1 code, .markdown-body h2 tt, .markdown-body h2 code, .markdown-body h3 tt, .markdown-body h3 code, .markdown-body h4 tt, .markdown-body h4 code, .markdown-body h5 tt, .markdown-body h5 code, .markdown-body h6 tt, .markdown-body h6 code { padding: 0 .2em; font-size: inherit; }
.markdown-body ul.no-list, .markdown-body ol.no-list { padding: 0; list-style-type: none; }
.markdown-body ul ul, .markdown-body ul ol, .markdown-body ol ol, .markdown-body ol ul { margin-top: 0; margin-bottom: 0; }
.markdown-body li>p { margin-top: 1rem; }
.markdown-body li+li { margin-top: .25em; }
.markdown-body dl { padding: 0; }
.markdown-body dl dt { padding: 0; margin-top: 1rem; font-size: 1em; font-style: italic; font-weight: 600; }
.markdown-body dl dd { padding: 0 1rem; margin-bottom: 1rem; }
.markdown-body table { border-spacing: 0; border-collapse: collapse; display: block; width: max-content; max-width: 100%; overflow: auto; font-variant: tabular-nums; }
.markdown-body table th { font-weight: 600; }
.markdown-body table th, .markdown-body table td { padding: 6px 13px; border: 1px solid #d1d9e0; }
.markdown-body table td>:last-child { margin-bottom: 0; }
.markdown-body table tr { background-color: #ffffff; border-top: 1px solid #d1d9e0b3; }
.markdown-body table tr:nth-child(2n) { background-color: #f6f8fa; }
.markdown-body code, .markdown-body tt { padding: .2em .4em; margin: 0; font-size: 85%; white-space: break-spaces; background-color: #818b981f; border-radius: 6px; }
.markdown-body code br, .markdown-body tt br { display: none; }
.markdown-body del code { text-decoration: inherit; }
.markdown-body samp { font-size: 85%; }
.markdown-body pre code { font-size: 100%; }
.markdown-body pre>code { padding: 0; margin: 0; word-break: normal; white-space: pre; background: transparent; border: 0; }
.markdown-body .highlight { margin-bottom: 1rem; }
.markdown-body .highlight pre { margin-bottom: 0; word-break: normal; }
.markdown-body .highlight pre, .markdown-body pre { padding: 1rem; overflow: auto; font-size: 85%; line-height: 1.45; color: #1f2328; background-color: #f6f8fa; border-radius: 6px; }
.markdown-body pre code, .markdown-body pre tt { display: inline; padding: 0; margin: 0; overflow: visible; line-height: inherit; word-wrap: normal; background-color: transparent; border: 0; }
.markdown-body .task-list-item { list-style-type: none; }
.markdown-body .task-list-item label { font-weight: 400; }
.markdown-body .task-list-item+.task-list-item { margin-top: 0.25rem; }
.markdown-body .task-list-item-checkbox { margin: 0 .2em .25em -1.4em; vertical-align: middle; }
.markdown-body .footnotes { font-size: 12px; color: #59636e; border-top: 1px solid #d1d9e0; }
.markdown-body .footnotes ol { padding-left: 1rem; }
.markdown-body kbd { display: inline-block; padding: 0.25rem; font: 11px ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, Liberation Mono, monospace; line-height: 10px; color: #1f2328; vertical-align: middle; background-color: #f6f8fa; border: solid 1px #d1d9e0; border-bottom-color: #d1d9e0; border-radius: 6px; box-shadow: inset 0 -1px 0 #d1d9e0; }
.markdown-body .markdown-alert { padding: 0.5rem 1rem; margin-bottom: 1rem; color: inherit; border-left: .25em solid #d1d9e0; }
.markdown-body .markdown-alert>:first-child { margin-top: 0; }
.markdown-body .markdown-alert>:last-child { margin-bottom: 0; }
.markdown-body .markdown-alert .markdown-alert-title { display: flex; font-weight: 500; align-items: center; line-height: 1; }
.markdown-body .markdown-alert.markdown-alert-note { border-left-color: #0969da; }
.markdown-body .markdown-alert.markdown-alert-note .markdown-alert-title { color: #0969da; }
.markdown-body .markdown-alert.markdown-alert-tip { border-left-color: #1a7f37; }
.markdown-body .markdown-alert.markdown-alert-tip .markdown-alert-title { color: #1a7f37; }
.markdown-body .markdown-alert.markdown-alert-important { border-left-color: #8250df; }
.markdown-body .markdown-alert.markdown-alert-important .markdown-alert-title { color: #8250df; }
.markdown-body .markdown-alert.markdown-alert-warning { border-left-color: #9a6700; }
.markdown-body .markdown-alert.markdown-alert-warning .markdown-alert-title { color: #9a6700; }
.markdown-body .markdown-alert.markdown-alert-caution { border-left-color: #cf222e; }
.markdown-body .markdown-alert.markdown-alert-caution .markdown-alert-title { color: #d1242f; }
.markdown-body .pl-c { color: #59636e; }
.markdown-body .pl-c1, .markdown-body .pl-s .pl-v { color: #0550ae; }
.markdown-body .pl-e, .markdown-body .pl-en { color: #6639ba; }
.markdown-body .pl-smi, .markdown-body .pl-s .pl-s1 { color: #1f2328; }
.markdown-body .pl-ent { color: #0550ae; }
.markdown-body .pl-k { color: #cf222e; }
.markdown-body .pl-s, .markdown-body .pl-pds, .markdown-body .pl-s .pl-pse .pl-s1, .markdown-body .pl-sr, .markdown-body .pl-sr .pl-cce, .markdown-body .pl-sr .pl-sre, .markdown-body .pl-sr .pl-sra { color: #0a3069; }
.markdown-body .pl-v, .markdown-body .pl-smw { color: #953800; }
.markdown-body .pl-bu { color: #82071e; }
.markdown-body .pl-sr .pl-cce { font-weight: bold; color: #116329; }
.markdown-body .pl-ml { color: #3b2300; }
.markdown-body .pl-mh, .markdown-body .pl-mh .pl-en, .markdown-body .pl-ms { font-weight: bold; color: #0550ae; }
.markdown-body .pl-mi { font-style: italic; color: #1f2328; }
.markdown-body .pl-mb { font-weight: bold; color: #1f2328; }
.markdown-body .pl-md { color: #82071e; background-color: #ffebe9; }
.markdown-body .pl-mi1 { color: #116329; background-color: #dafbe1; }
.markdown-body .pl-mc { color: #953800; background-color: #ffd8b5; }
.markdown-body .pl-mdr { font-weight: bold; color: #8250df; }
.markdown-body .pl-ba { color: #59636e; }
body { background-color: #ffffff; margin: 0; padding: 0; }
.container { max-width: 1012px; margin: 0 auto; padding: 32px 32px; }
</style>
</head>
<body>
<div class="container">
<article class="markdown-body">
%1
</article>
</div>
<script src="https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.min.js"></script>
<script>
document.querySelectorAll('code.language-mermaid').forEach(function(code) {
  var pre = code.parentElement;
  var div = document.createElement('div');
  div.className = 'mermaid';
  div.textContent = code.textContent;
  pre.parentElement.replaceChild(div, pre);
});
mermaid.initialize({ startOnLoad: true });
</script>
</body>
</html>
)");

#else /* GTK */

static const char *HTML_TEMPLATE =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset=\"utf-8\">\n"
"<style>\n"
".markdown-body {\n"
"  -ms-text-size-adjust: 100%;\n"
"  -webkit-text-size-adjust: 100%;\n"
"  margin: 0;\n"
"  font-weight: 400;\n"
"  color: #1f2328;\n"
"  background-color: #ffffff;\n"
"  font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", \"Noto Sans\",\n"
"               Helvetica, Arial, sans-serif, \"Apple Color Emoji\", \"Segoe UI Emoji\";\n"
"  font-size: 16px;\n"
"  line-height: 1.5;\n"
"  word-wrap: break-word;\n"
"}\n"
".markdown-body a { background-color: transparent; color: #0969da; text-decoration: none; }\n"
".markdown-body a:hover { text-decoration: underline; }\n"
".markdown-body b, .markdown-body strong { font-weight: 600; }\n"
".markdown-body dfn { font-style: italic; }\n"
".markdown-body h1 { margin: .67em 0; font-weight: 600; padding-bottom: .3em; font-size: 2em; border-bottom: 1px solid #d1d9e0b3; }\n"
".markdown-body mark { background-color: #fff8c5; color: #1f2328; }\n"
".markdown-body small { font-size: 90%; }\n"
".markdown-body sub, .markdown-body sup { font-size: 75%; line-height: 0; position: relative; vertical-align: baseline; }\n"
".markdown-body sub { bottom: -0.25em; }\n"
".markdown-body sup { top: -0.5em; }\n"
".markdown-body img { border-style: none; max-width: 100%%; box-sizing: content-box; }\n"
".markdown-body code, .markdown-body kbd, .markdown-body pre, .markdown-body samp { font-family: monospace; font-size: 1em; }\n"
".markdown-body hr { box-sizing: content-box; overflow: hidden; background: transparent; border-bottom: 1px solid #d1d9e0b3; height: .25em; padding: 0; margin: 1.5rem 0; background-color: #d1d9e0; border: 0; }\n"
".markdown-body h1, .markdown-body h2, .markdown-body h3, .markdown-body h4, .markdown-body h5, .markdown-body h6 { margin-top: 1.5rem; margin-bottom: 1rem; font-weight: 600; line-height: 1.25; }\n"
".markdown-body h2 { font-weight: 600; padding-bottom: .3em; font-size: 1.5em; border-bottom: 1px solid #d1d9e0b3; }\n"
".markdown-body h3 { font-weight: 600; font-size: 1.25em; }\n"
".markdown-body h4 { font-weight: 600; font-size: 1em; }\n"
".markdown-body h5 { font-weight: 600; font-size: .875em; }\n"
".markdown-body h6 { font-weight: 600; font-size: .85em; color: #59636e; }\n"
".markdown-body p { margin-top: 0; margin-bottom: 10px; }\n"
".markdown-body blockquote { margin: 0; padding: 0 1em; color: #59636e; border-left: .25em solid #d1d9e0; }\n"
".markdown-body ul, .markdown-body ol { margin-top: 0; margin-bottom: 0; padding-left: 2em; }\n"
".markdown-body ol ol, .markdown-body ul ol { list-style-type: lower-roman; }\n"
".markdown-body ul ul ol, .markdown-body ul ol ol, .markdown-body ol ul ol, .markdown-body ol ol ol { list-style-type: lower-alpha; }\n"
".markdown-body dd { margin-left: 0; }\n"
".markdown-body tt, .markdown-body code, .markdown-body samp { font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, Liberation Mono, monospace; font-size: 12px; }\n"
".markdown-body pre { margin-top: 0; margin-bottom: 0; font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, Liberation Mono, monospace; font-size: 12px; word-wrap: normal; }\n"
".markdown-body>*:first-child { margin-top: 0 !important; }\n"
".markdown-body>*:last-child { margin-bottom: 0 !important; }\n"
".markdown-body a:not([href]) { color: inherit; text-decoration: none; }\n"
".markdown-body p, .markdown-body blockquote, .markdown-body ul, .markdown-body ol, .markdown-body dl, .markdown-body table, .markdown-body pre, .markdown-body details { margin-top: 0; margin-bottom: 1rem; }\n"
".markdown-body blockquote>:first-child { margin-top: 0; }\n"
".markdown-body blockquote>:last-child { margin-bottom: 0; }\n"
".markdown-body h1 tt, .markdown-body h1 code, .markdown-body h2 tt, .markdown-body h2 code, .markdown-body h3 tt, .markdown-body h3 code, .markdown-body h4 tt, .markdown-body h4 code, .markdown-body h5 tt, .markdown-body h5 code, .markdown-body h6 tt, .markdown-body h6 code { padding: 0 .2em; font-size: inherit; }\n"
".markdown-body ul.no-list, .markdown-body ol.no-list { padding: 0; list-style-type: none; }\n"
".markdown-body ul ul, .markdown-body ul ol, .markdown-body ol ol, .markdown-body ol ul { margin-top: 0; margin-bottom: 0; }\n"
".markdown-body li>p { margin-top: 1rem; }\n"
".markdown-body li+li { margin-top: .25em; }\n"
".markdown-body dl { padding: 0; }\n"
".markdown-body dl dt { padding: 0; margin-top: 1rem; font-size: 1em; font-style: italic; font-weight: 600; }\n"
".markdown-body dl dd { padding: 0 1rem; margin-bottom: 1rem; }\n"
".markdown-body table { border-spacing: 0; border-collapse: collapse; display: block; width: max-content; max-width: 100%%; overflow: auto; font-variant: tabular-nums; }\n"
".markdown-body table th { font-weight: 600; }\n"
".markdown-body table th, .markdown-body table td { padding: 6px 13px; border: 1px solid #d1d9e0; }\n"
".markdown-body table td>:last-child { margin-bottom: 0; }\n"
".markdown-body table tr { background-color: #ffffff; border-top: 1px solid #d1d9e0b3; }\n"
".markdown-body table tr:nth-child(2n) { background-color: #f6f8fa; }\n"
".markdown-body code, .markdown-body tt { padding: .2em .4em; margin: 0; font-size: 85%%; white-space: break-spaces; background-color: #818b981f; border-radius: 6px; }\n"
".markdown-body code br, .markdown-body tt br { display: none; }\n"
".markdown-body del code { text-decoration: inherit; }\n"
".markdown-body samp { font-size: 85%%; }\n"
".markdown-body pre code { font-size: 100%%; }\n"
".markdown-body pre>code { padding: 0; margin: 0; word-break: normal; white-space: pre; background: transparent; border: 0; }\n"
".markdown-body .highlight { margin-bottom: 1rem; }\n"
".markdown-body .highlight pre { margin-bottom: 0; word-break: normal; }\n"
".markdown-body .highlight pre, .markdown-body pre { padding: 1rem; overflow: auto; font-size: 85%%; line-height: 1.45; color: #1f2328; background-color: #f6f8fa; border-radius: 6px; }\n"
".markdown-body pre code, .markdown-body pre tt { display: inline; padding: 0; margin: 0; overflow: visible; line-height: inherit; word-wrap: normal; background-color: transparent; border: 0; }\n"
".markdown-body .task-list-item { list-style-type: none; }\n"
".markdown-body .task-list-item label { font-weight: 400; }\n"
".markdown-body .task-list-item+.task-list-item { margin-top: 0.25rem; }\n"
".markdown-body .task-list-item-checkbox { margin: 0 .2em .25em -1.4em; vertical-align: middle; }\n"
".markdown-body .footnotes { font-size: 12px; color: #59636e; border-top: 1px solid #d1d9e0; }\n"
".markdown-body .footnotes ol { padding-left: 1rem; }\n"
".markdown-body kbd { display: inline-block; padding: 0.25rem; font: 11px ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas, Liberation Mono, monospace; line-height: 10px; color: #1f2328; vertical-align: middle; background-color: #f6f8fa; border: solid 1px #d1d9e0; border-bottom-color: #d1d9e0; border-radius: 6px; box-shadow: inset 0 -1px 0 #d1d9e0; }\n"
".markdown-body .markdown-alert { padding: 0.5rem 1rem; margin-bottom: 1rem; color: inherit; border-left: .25em solid #d1d9e0; }\n"
".markdown-body .markdown-alert>:first-child { margin-top: 0; }\n"
".markdown-body .markdown-alert>:last-child { margin-bottom: 0; }\n"
".markdown-body .markdown-alert .markdown-alert-title { display: flex; font-weight: 500; align-items: center; line-height: 1; }\n"
".markdown-body .markdown-alert.markdown-alert-note { border-left-color: #0969da; }\n"
".markdown-body .markdown-alert.markdown-alert-note .markdown-alert-title { color: #0969da; }\n"
".markdown-body .markdown-alert.markdown-alert-tip { border-left-color: #1a7f37; }\n"
".markdown-body .markdown-alert.markdown-alert-tip .markdown-alert-title { color: #1a7f37; }\n"
".markdown-body .markdown-alert.markdown-alert-important { border-left-color: #8250df; }\n"
".markdown-body .markdown-alert.markdown-alert-important .markdown-alert-title { color: #8250df; }\n"
".markdown-body .markdown-alert.markdown-alert-warning { border-left-color: #9a6700; }\n"
".markdown-body .markdown-alert.markdown-alert-warning .markdown-alert-title { color: #9a6700; }\n"
".markdown-body .markdown-alert.markdown-alert-caution { border-left-color: #cf222e; }\n"
".markdown-body .markdown-alert.markdown-alert-caution .markdown-alert-title { color: #d1242f; }\n"
".markdown-body .pl-c { color: #59636e; }\n"
".markdown-body .pl-c1, .markdown-body .pl-s .pl-v { color: #0550ae; }\n"
".markdown-body .pl-e, .markdown-body .pl-en { color: #6639ba; }\n"
".markdown-body .pl-smi, .markdown-body .pl-s .pl-s1 { color: #1f2328; }\n"
".markdown-body .pl-ent { color: #0550ae; }\n"
".markdown-body .pl-k { color: #cf222e; }\n"
".markdown-body .pl-s, .markdown-body .pl-pds, .markdown-body .pl-s .pl-pse .pl-s1, .markdown-body .pl-sr, .markdown-body .pl-sr .pl-cce, .markdown-body .pl-sr .pl-sre, .markdown-body .pl-sr .pl-sra { color: #0a3069; }\n"
".markdown-body .pl-v, .markdown-body .pl-smw { color: #953800; }\n"
".markdown-body .pl-bu { color: #82071e; }\n"
".markdown-body .pl-sr .pl-cce { font-weight: bold; color: #116329; }\n"
".markdown-body .pl-ml { color: #3b2300; }\n"
".markdown-body .pl-mh, .markdown-body .pl-mh .pl-en, .markdown-body .pl-ms { font-weight: bold; color: #0550ae; }\n"
".markdown-body .pl-mi { font-style: italic; color: #1f2328; }\n"
".markdown-body .pl-mb { font-weight: bold; color: #1f2328; }\n"
".markdown-body .pl-md { color: #82071e; background-color: #ffebe9; }\n"
".markdown-body .pl-mi1 { color: #116329; background-color: #dafbe1; }\n"
".markdown-body .pl-mc { color: #953800; background-color: #ffd8b5; }\n"
".markdown-body .pl-mdr { font-weight: bold; color: #8250df; }\n"
".markdown-body .pl-ba { color: #59636e; }\n"
"body { background-color: #ffffff; margin: 0; padding: 0; }\n"
".container { max-width: 1012px; margin: 0 auto; padding: 32px 32px; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"container\">\n"
"<article class=\"markdown-body\">\n"
"%s\n"
"</article>\n"
"</div>\n"
"<script src=\"https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.min.js\"></script>\n"
"<script>\n"
"document.querySelectorAll('code.language-mermaid').forEach(function(code) {\n"
"  var pre = code.parentElement;\n"
"  var div = document.createElement('div');\n"
"  div.className = 'mermaid';\n"
"  div.textContent = code.textContent;\n"
"  pre.parentElement.replaceChild(div, pre);\n"
"});\n"
"mermaid.initialize({ startOnLoad: true });\n"
"</script>\n"
"</body>\n"
"</html>\n";

#endif /* MDPEEK_USE_QT */

/* ── Alert definitions (shared) ─────────────────────────────────────── */

struct AlertType {
    const char *tag;
    const char *label;
    const char *css_class;
    const char *svg;
};

static const AlertType ALERTS[] = {
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
static const size_t N_ALERTS = sizeof(ALERTS) / sizeof(ALERTS[0]);

/* ── Shared rendering (cmark-gfm) ───────────────────────────────────── */

/* Returns a heap-allocated C string; caller must free() it. */
static char *render_markdown(const char *markdown, size_t len)
{
    cmark_gfm_core_extensions_ensure_registered();

    int options = CMARK_OPT_UNSAFE | CMARK_OPT_SMART;
    cmark_parser *parser = cmark_parser_new(options);

    static const char *ext_names[] = {
        "table", "strikethrough", "autolink", "tasklist", "tagfilter"
    };
    for (size_t i = 0; i < sizeof(ext_names) / sizeof(ext_names[0]); i++) {
        cmark_syntax_extension *ext = cmark_find_syntax_extension(ext_names[i]);
        if (ext)
            cmark_parser_attach_syntax_extension(parser, ext);
    }

    cmark_parser_feed(parser, markdown, len);
    cmark_node *doc = cmark_parser_finish(parser);

    char *html = cmark_render_html(doc, options,
                                   cmark_parser_get_syntax_extensions(parser));
    cmark_node_free(doc);
    cmark_parser_free(parser);
    return html; /* caller must free() */
}

/* ── Alert transformation (shared via #ifdef on string ops) ─────────── */

#ifdef MDPEEK_USE_QT

static QString transform_alerts(const QString &html)
{
    QString result = html;
    for (size_t i = 0; i < N_ALERTS; i++) {
        const AlertType &a = ALERTS[i];
        QString pattern = QStringLiteral("<blockquote>\\s*<p>\\[!%1\\]\\s*(?:<br\\s*/?>)?\\s*")
                          .arg(QString::fromUtf8(a.tag));
        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(result);
        while (match.hasMatch()) {
            int startPos    = match.capturedStart();
            int contentStart = match.capturedEnd();
            int closeTag = result.indexOf(QStringLiteral("</blockquote>"), contentStart);
            if (closeTag < 0) break;
            QString inner = result.mid(contentStart, closeTag - contentStart);
            QString replacement = QStringLiteral(
                "<div class='markdown-alert markdown-alert-%1'>"
                "<p class='markdown-alert-title'>%2%3</p>%4</div>")
                .arg(QString::fromUtf8(a.css_class),
                     QString::fromUtf8(a.svg),
                     QString::fromUtf8(a.label),
                     inner);
            result.replace(startPos, closeTag + 13 - startPos, replacement);
            match = re.match(result, startPos + replacement.size());
        }
    }
    return result;
}

static QString wrap_html(const QString &body)
{
    return HTML_TEMPLATE.arg(body);
}

#else /* GTK */

static char *transform_alerts(const char *html)
{
    char *result = g_strdup(html);
    for (size_t i = 0; i < N_ALERTS; i++) {
        const AlertType *a = &ALERTS[i];
        char *pattern = g_strdup_printf(
            "<blockquote>\\s*<p>\\[!%s\\]\\s*(?:<br\\s*/?>)?\\s*", a->tag);
        GRegex *re = g_regex_new(pattern, G_REGEX_CASELESS, (GRegexMatchFlags)0, NULL);
        g_free(pattern);
        if (!re) continue;
        for (;;) {
            GMatchInfo *match_info = NULL;
            if (!g_regex_match(re, result, (GRegexMatchFlags)0, &match_info)) {
                g_match_info_free(match_info);
                break;
            }
            int start_pos, content_start;
            g_match_info_fetch_pos(match_info, 0, &start_pos, &content_start);
            g_match_info_free(match_info);
            const char *close = strstr(result + content_start, "</blockquote>");
            if (!close) break;
            int close_pos = (int)(close - result);
            int close_len = 13;
            char *inner = g_strndup(result + content_start, close_pos - content_start);
            char *replacement = g_strdup_printf(
                "<div class='markdown-alert markdown-alert-%s'>"
                "<p class='markdown-alert-title'>%s%s</p>%s</div>",
                a->css_class, a->svg, a->label, inner);
            g_free(inner);
            int old_len    = close_pos + close_len - start_pos;
            int new_len    = (int)strlen(replacement);
            int result_len = (int)strlen(result);
            int final_len  = result_len - old_len + new_len;
            char *new_result = (char *)g_malloc(final_len + 1);
            memcpy(new_result, result, start_pos);
            memcpy(new_result + start_pos, replacement, new_len);
            memcpy(new_result + start_pos + new_len,
                   result + close_pos + close_len,
                   result_len - close_pos - close_len);
            new_result[final_len] = '\0';
            g_free(replacement);
            g_free(result);
            result = new_result;
        }
        g_regex_unref(re);
    }
    return result;
}

static char *wrap_html(const char *body)
{
    return g_strdup_printf(HTML_TEMPLATE, body);
}

#endif /* MDPEEK_USE_QT */

/* ── Qt viewer implementation ───────────────────────────────────────── */

#ifdef MDPEEK_USE_QT

MdpeekPage::MdpeekPage(QObject *parent)
    : QWebEnginePage(parent)
{
}

bool MdpeekPage::acceptNavigationRequest(const QUrl &url,
                                          NavigationType type,
                                          bool isMainFrame)
{
    Q_UNUSED(type); Q_UNUSED(isMainFrame);

    /* Allow internal / blank loads */
    if (url.isEmpty() || url.scheme() == QStringLiteral("about") ||
        url.scheme() == QStringLiteral("data") ||
        url.scheme() == QStringLiteral("blob"))
        return true;

    /* Intercept reload: re-render markdown instead of loading raw file */
    if (url.scheme() == QStringLiteral("file")) {
        if (url.toLocalFile() == m_filePath) {
            emit reloadRequested();
            return false;
        }
        return true; /* allow other local file links */
    }

    /* Open external links in the default browser */
    QDesktopServices::openUrl(url);
    return false;
}

MdpeekViewer::MdpeekViewer(const QString &filePath, QWidget *parent)
    : QMainWindow(parent)
    , m_filePath(filePath)
    , m_scrollY(0.0)
{
    QFileInfo fi(m_filePath);
    setWindowTitle(fi.fileName() + QString::fromUtf8(" \xe2\x80\x94 mdpeek"));
    resize(900, 700);

    m_page = new MdpeekPage(this);
    m_page->setFilePath(m_filePath);

    /* Allow file:// pages to load https:// CDN scripts (e.g. Mermaid.js) */
    m_page->settings()->setAttribute(
        QWebEngineSettings::AllowRunningInsecureContent, true);
    m_page->settings()->setAttribute(
        QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    m_view = new QWebEngineView(this);
    m_view->setPage(m_page);
    setCentralWidget(m_view);

    connect(m_page, &MdpeekPage::reloadRequested,
            this,   &MdpeekViewer::reloadFile);

    /* Escape to close */
    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(close()));

    /* File watcher */
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(m_filePath);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this,      &MdpeekViewer::onFileChanged);

    /* Debounce timer */
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(150);
    connect(m_debounceTimer, &QTimer::timeout,
            this,            &MdpeekViewer::reloadFile);

    loadFile();
}

void MdpeekViewer::onFileChanged(const QString &path)
{
    Q_UNUSED(path);
    /* Re-add watch — editors that do atomic saves remove the path */
    if (!m_watcher->files().contains(m_filePath)) {
        QTimer::singleShot(50, this, [this]() {
            if (QFile::exists(m_filePath))
                m_watcher->addPath(m_filePath);
        });
    }
    m_debounceTimer->start();
}

void MdpeekViewer::reloadFile()
{
    m_page->runJavaScript(QStringLiteral("window.scrollY"),
        [this](const QVariant &result) {
            m_scrollY = result.toDouble();
            loadFile();
        });
}

void MdpeekViewer::loadFile()
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

    char *body_c = render_markdown(raw.constData(), (size_t)raw.size());
    QString body = QString::fromUtf8(body_c);
    free(body_c);

    body = transform_alerts(body);

    QString full_html = wrap_html(body);

    /* Inject scroll-restore script when reloading */
    if (m_scrollY > 0.0) {
        full_html += QStringLiteral(
            "<script>window.addEventListener('load',function(){"
            "window.scrollTo(0,%1)},{once:true});</script>")
            .arg(m_scrollY);
        m_scrollY = 0.0;
    }

    /* Use the file's directory as base URL so relative images resolve */
    QUrl baseUrl = QUrl::fromLocalFile(QFileInfo(m_filePath).absolutePath() + '/');
    m_view->setContent(full_html.toUtf8(), QStringLiteral("text/html"), baseUrl);
}

#else /* GTK */

/* ── GTK viewer lifecycle ───────────────────────────────────────────── */

static void viewer_free(MdpeekViewer *v)
{
    if (v->monitor) {
        g_file_monitor_cancel(v->monitor);
        g_object_unref(v->monitor);
    }
    if (v->reload_source_id)
        g_source_remove(v->reload_source_id);
    g_free(v->file_path);
    g_free(v);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data)
{
    (void)widget;
    viewer_free((MdpeekViewer *)user_data);
}

static void on_close_action(GSimpleAction *action, GVariant *param,
                             gpointer user_data)
{
    (void)action; (void)param;
    MdpeekViewer *v = (MdpeekViewer *)user_data;
    gtk_window_close(GTK_WINDOW(v->window));
}

static void on_scroll_saved(GObject *source, GAsyncResult *result,
                             gpointer user_data)
{
    MdpeekViewer *v = (MdpeekViewer *)user_data;
    JSCValue *val = webkit_web_view_evaluate_javascript_finish(
        WEBKIT_WEB_VIEW(source), result, NULL);
    if (val) {
        v->scroll_y = jsc_value_to_double(val);
        g_object_unref(val);
    }
    viewer_load_file(v);
}

static void scroll_save_and_reload(MdpeekViewer *v)
{
    webkit_web_view_evaluate_javascript(v->webview,
        "window.scrollY", -1, NULL, NULL, NULL,
        on_scroll_saved, v);
}

static gboolean reload_idle(gpointer user_data)
{
    scroll_save_and_reload((MdpeekViewer *)user_data);
    return G_SOURCE_REMOVE;
}

static gboolean on_decide_policy(WebKitWebView *web_view,
                                  WebKitPolicyDecision *decision,
                                  WebKitPolicyDecisionType type,
                                  gpointer user_data)
{
    (void)web_view;
    MdpeekViewer *v = (MdpeekViewer *)user_data;

    if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
        return FALSE;

    WebKitNavigationPolicyDecision *nav =
        WEBKIT_NAVIGATION_POLICY_DECISION(decision);
    WebKitNavigationAction *action =
        webkit_navigation_policy_decision_get_navigation_action(nav);
    WebKitURIRequest *request =
        webkit_navigation_action_get_request(action);
    const char *uri = webkit_uri_request_get_uri(request);

    if (uri == NULL ||
        g_str_has_prefix(uri, "about:") ||
        g_str_has_prefix(uri, "data:"))
        return FALSE;

    if (g_str_has_prefix(uri, "file:")) {
        char *content_uri = g_filename_to_uri(v->file_path, NULL, NULL);
        gboolean is_self = (g_strcmp0(uri, content_uri) == 0);
        g_free(content_uri);
        if (is_self) {
            webkit_policy_decision_ignore(decision);
            g_idle_add(reload_idle, v);
            return TRUE;
        }
        return FALSE;
    }

    webkit_policy_decision_ignore(decision);
    g_app_info_launch_default_for_uri(uri, NULL, NULL);
    return TRUE;
}

MdpeekViewer *viewer_new(AdwApplication *app, const char *file_path)
{
    MdpeekViewer *v = g_new0(MdpeekViewer, 1);
    v->file_path = g_strdup(file_path);

    char *basename = g_path_get_basename(file_path);
    char *title = g_strdup_printf("%s \xe2\x80\x94 mdpeek", basename);
    g_free(basename);

    v->window = ADW_APPLICATION_WINDOW(
        adw_application_window_new(GTK_APPLICATION(app)));
    gtk_window_set_title(GTK_WINDOW(v->window), title);
    gtk_window_set_default_size(GTK_WINDOW(v->window), 900, 700);
    g_free(title);

    WebKitSettings *wk_settings = webkit_settings_new();
    webkit_settings_set_allow_file_access_from_file_urls(wk_settings, TRUE);
    v->webview = WEBKIT_WEB_VIEW(
        g_object_new(WEBKIT_TYPE_WEB_VIEW, "settings", wk_settings, NULL));
    g_object_unref(wk_settings);

    g_signal_connect(v->webview, "decide-policy",
                     G_CALLBACK(on_decide_policy), v);
    adw_application_window_set_content(v->window, GTK_WIDGET(v->webview));

    GSimpleAction *close_action = g_simple_action_new("close", NULL);
    g_signal_connect(close_action, "activate",
                     G_CALLBACK(on_close_action), v);
    g_action_map_add_action(G_ACTION_MAP(v->window),
                            G_ACTION(close_action));
    g_object_unref(close_action);

    const char * const close_accels[] = {"Escape", NULL};
    gtk_application_set_accels_for_action(GTK_APPLICATION(app),
                                          "win.close", close_accels);

    GFile *gfile = g_file_new_for_path(file_path);
    GError *err = NULL;
    v->monitor = g_file_monitor_file(gfile, G_FILE_MONITOR_NONE, NULL, &err);
    g_object_unref(gfile);

    if (v->monitor) {
        g_file_monitor_set_rate_limit(v->monitor, 150);
        g_signal_connect(v->monitor, "changed",
                         G_CALLBACK(on_file_changed), v);
    } else {
        fprintf(stderr, "Warning: could not watch file: %s\n",
                err ? err->message : "unknown error");
        g_clear_error(&err);
    }

    g_signal_connect(v->window, "destroy",
                     G_CALLBACK(on_window_destroy), v);

    viewer_load_file(v);

    printf("Press Escape to quit.\n");
    gtk_window_present(GTK_WINDOW(v->window));
    return v;
}

/* ── GTK file change handling ───────────────────────────────────────── */

static gboolean reload_timeout(gpointer user_data)
{
    MdpeekViewer *v = (MdpeekViewer *)user_data;
    v->reload_source_id = 0;
    scroll_save_and_reload(v);
    return G_SOURCE_REMOVE;
}

static void on_file_changed(GFileMonitor *monitor, GFile *file,
                              GFile *other, GFileMonitorEvent event,
                              gpointer user_data)
{
    (void)monitor; (void)file; (void)other;
    MdpeekViewer *v = (MdpeekViewer *)user_data;

    if (event != G_FILE_MONITOR_EVENT_CHANGED &&
        event != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT &&
        event != G_FILE_MONITOR_EVENT_CREATED)
        return;

    if (v->reload_source_id)
        g_source_remove(v->reload_source_id);
    v->reload_source_id = g_timeout_add(150, reload_timeout, v);
}

/* ── GTK load and render ────────────────────────────────────────────── */

void viewer_load_file(MdpeekViewer *v)
{
    char *contents = NULL;
    gsize length = 0;
    GError *err = NULL;

    if (!g_file_get_contents(v->file_path, &contents, &length, &err)) {
        char *error_html = g_strdup_printf(
            "<html><body><p style='color:red;'>Failed to open: %s<br>%s</p></body></html>",
            v->file_path, err ? err->message : "");
        webkit_web_view_load_html(v->webview, error_html, NULL);
        g_free(error_html);
        g_clear_error(&err);
        return;
    }

    char *body = render_markdown(contents, length);
    g_free(contents);

    char *transformed = transform_alerts(body);
    free(body);

    char *base_html = wrap_html(transformed);
    g_free(transformed);

    char *full_html;
    if (v->scroll_y > 0.0) {
        full_html = g_strdup_printf(
            "%s<script>window.addEventListener('load',function(){"
            "window.scrollTo(0,%f)},{once:true});</script>",
            base_html, v->scroll_y);
        v->scroll_y = 0.0;
    } else {
        full_html = base_html;
        base_html = NULL;
    }
    g_free(base_html);

    char *dir = g_path_get_dirname(v->file_path);
    char *dir_uri = g_filename_to_uri(dir, NULL, NULL);
    g_free(dir);
    char *base_uri = g_strdup_printf("%s/", dir_uri);
    g_free(dir_uri);

    char *content_uri = g_filename_to_uri(v->file_path, NULL, NULL);

    webkit_web_view_load_alternate_html(v->webview, full_html,
                                        content_uri, base_uri);
    g_free(content_uri);
    g_free(base_uri);
    g_free(full_html);
}

#endif /* MDPEEK_USE_QT */
