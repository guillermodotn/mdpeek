#include "viewer.h"

#include <cmark-gfm.h>
#include <cmark-gfm-core-extensions.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Forward declarations ───────────────────────────────────────────── */

static char *render_markdown(const char *markdown, size_t len);
static char *transform_alerts(const char *html);
static char *wrap_html(const char *body);
static void  on_file_changed(GFileMonitor *monitor, GFile *file,
                              GFile *other, GFileMonitorEvent event,
                              gpointer user_data);

/* ── HTML template ──────────────────────────────────────────────────── */

static const char *HTML_TEMPLATE =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<meta charset=\"utf-8\">\n"
"<style>\n"
/* === github-markdown-css (light) === */
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
".markdown-body a {\n"
"  background-color: transparent;\n"
"  color: #0969da;\n"
"  text-decoration: none;\n"
"}\n"
".markdown-body a:hover { text-decoration: underline; }\n"
".markdown-body b,\n"
".markdown-body strong { font-weight: 600; }\n"
".markdown-body dfn { font-style: italic; }\n"
".markdown-body h1 {\n"
"  margin: .67em 0;\n"
"  font-weight: 600;\n"
"  padding-bottom: .3em;\n"
"  font-size: 2em;\n"
"  border-bottom: 1px solid #d1d9e0b3;\n"
"}\n"
".markdown-body mark { background-color: #fff8c5; color: #1f2328; }\n"
".markdown-body small { font-size: 90%; }\n"
".markdown-body sub,\n"
".markdown-body sup {\n"
"  font-size: 75%;\n"
"  line-height: 0;\n"
"  position: relative;\n"
"  vertical-align: baseline;\n"
"}\n"
".markdown-body sub { bottom: -0.25em; }\n"
".markdown-body sup { top: -0.5em; }\n"
".markdown-body img {\n"
"  border-style: none;\n"
"  max-width: 100%%;\n"
"  box-sizing: content-box;\n"
"}\n"
".markdown-body code,\n"
".markdown-body kbd,\n"
".markdown-body pre,\n"
".markdown-body samp {\n"
"  font-family: monospace;\n"
"  font-size: 1em;\n"
"}\n"
".markdown-body hr {\n"
"  box-sizing: content-box;\n"
"  overflow: hidden;\n"
"  background: transparent;\n"
"  border-bottom: 1px solid #d1d9e0b3;\n"
"  height: .25em;\n"
"  padding: 0;\n"
"  margin: 1.5rem 0;\n"
"  background-color: #d1d9e0;\n"
"  border: 0;\n"
"}\n"
".markdown-body h1,\n"
".markdown-body h2,\n"
".markdown-body h3,\n"
".markdown-body h4,\n"
".markdown-body h5,\n"
".markdown-body h6 {\n"
"  margin-top: 1.5rem;\n"
"  margin-bottom: 1rem;\n"
"  font-weight: 600;\n"
"  line-height: 1.25;\n"
"}\n"
".markdown-body h2 {\n"
"  font-weight: 600;\n"
"  padding-bottom: .3em;\n"
"  font-size: 1.5em;\n"
"  border-bottom: 1px solid #d1d9e0b3;\n"
"}\n"
".markdown-body h3 { font-weight: 600; font-size: 1.25em; }\n"
".markdown-body h4 { font-weight: 600; font-size: 1em; }\n"
".markdown-body h5 { font-weight: 600; font-size: .875em; }\n"
".markdown-body h6 { font-weight: 600; font-size: .85em; color: #59636e; }\n"
".markdown-body p { margin-top: 0; margin-bottom: 10px; }\n"
".markdown-body blockquote {\n"
"  margin: 0;\n"
"  padding: 0 1em;\n"
"  color: #59636e;\n"
"  border-left: .25em solid #d1d9e0;\n"
"}\n"
".markdown-body ul,\n"
".markdown-body ol {\n"
"  margin-top: 0;\n"
"  margin-bottom: 0;\n"
"  padding-left: 2em;\n"
"}\n"
".markdown-body ol ol,\n"
".markdown-body ul ol { list-style-type: lower-roman; }\n"
".markdown-body ul ul ol,\n"
".markdown-body ul ol ol,\n"
".markdown-body ol ul ol,\n"
".markdown-body ol ol ol { list-style-type: lower-alpha; }\n"
".markdown-body dd { margin-left: 0; }\n"
".markdown-body tt,\n"
".markdown-body code,\n"
".markdown-body samp {\n"
"  font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas,\n"
"               Liberation Mono, monospace;\n"
"  font-size: 12px;\n"
"}\n"
".markdown-body pre {\n"
"  margin-top: 0;\n"
"  margin-bottom: 0;\n"
"  font-family: ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas,\n"
"               Liberation Mono, monospace;\n"
"  font-size: 12px;\n"
"  word-wrap: normal;\n"
"}\n"
".markdown-body>*:first-child { margin-top: 0 !important; }\n"
".markdown-body>*:last-child { margin-bottom: 0 !important; }\n"
".markdown-body a:not([href]) { color: inherit; text-decoration: none; }\n"
".markdown-body p,\n"
".markdown-body blockquote,\n"
".markdown-body ul,\n"
".markdown-body ol,\n"
".markdown-body dl,\n"
".markdown-body table,\n"
".markdown-body pre,\n"
".markdown-body details {\n"
"  margin-top: 0;\n"
"  margin-bottom: 1rem;\n"
"}\n"
".markdown-body blockquote>:first-child { margin-top: 0; }\n"
".markdown-body blockquote>:last-child { margin-bottom: 0; }\n"
".markdown-body h1 tt, .markdown-body h1 code,\n"
".markdown-body h2 tt, .markdown-body h2 code,\n"
".markdown-body h3 tt, .markdown-body h3 code,\n"
".markdown-body h4 tt, .markdown-body h4 code,\n"
".markdown-body h5 tt, .markdown-body h5 code,\n"
".markdown-body h6 tt, .markdown-body h6 code {\n"
"  padding: 0 .2em;\n"
"  font-size: inherit;\n"
"}\n"
".markdown-body ul.no-list,\n"
".markdown-body ol.no-list { padding: 0; list-style-type: none; }\n"
".markdown-body ul ul,\n"
".markdown-body ul ol,\n"
".markdown-body ol ol,\n"
".markdown-body ol ul { margin-top: 0; margin-bottom: 0; }\n"
".markdown-body li>p { margin-top: 1rem; }\n"
".markdown-body li+li { margin-top: .25em; }\n"
".markdown-body dl { padding: 0; }\n"
".markdown-body dl dt {\n"
"  padding: 0; margin-top: 1rem;\n"
"  font-size: 1em; font-style: italic; font-weight: 600;\n"
"}\n"
".markdown-body dl dd { padding: 0 1rem; margin-bottom: 1rem; }\n"
/* Tables */
".markdown-body table {\n"
"  border-spacing: 0;\n"
"  border-collapse: collapse;\n"
"  display: block;\n"
"  width: max-content;\n"
"  max-width: 100%%;\n"
"  overflow: auto;\n"
"  font-variant: tabular-nums;\n"
"}\n"
".markdown-body table th { font-weight: 600; }\n"
".markdown-body table th,\n"
".markdown-body table td {\n"
"  padding: 6px 13px;\n"
"  border: 1px solid #d1d9e0;\n"
"}\n"
".markdown-body table td>:last-child { margin-bottom: 0; }\n"
".markdown-body table tr {\n"
"  background-color: #ffffff;\n"
"  border-top: 1px solid #d1d9e0b3;\n"
"}\n"
".markdown-body table tr:nth-child(2n) { background-color: #f6f8fa; }\n"
/* Code */
".markdown-body code,\n"
".markdown-body tt {\n"
"  padding: .2em .4em;\n"
"  margin: 0;\n"
"  font-size: 85%%;\n"
"  white-space: break-spaces;\n"
"  background-color: #818b981f;\n"
"  border-radius: 6px;\n"
"}\n"
".markdown-body code br,\n"
".markdown-body tt br { display: none; }\n"
".markdown-body del code { text-decoration: inherit; }\n"
".markdown-body samp { font-size: 85%%; }\n"
".markdown-body pre code { font-size: 100%%; }\n"
".markdown-body pre>code {\n"
"  padding: 0; margin: 0;\n"
"  word-break: normal; white-space: pre;\n"
"  background: transparent; border: 0;\n"
"}\n"
".markdown-body .highlight { margin-bottom: 1rem; }\n"
".markdown-body .highlight pre { margin-bottom: 0; word-break: normal; }\n"
".markdown-body .highlight pre,\n"
".markdown-body pre {\n"
"  padding: 1rem;\n"
"  overflow: auto;\n"
"  font-size: 85%%;\n"
"  line-height: 1.45;\n"
"  color: #1f2328;\n"
"  background-color: #f6f8fa;\n"
"  border-radius: 6px;\n"
"}\n"
".markdown-body pre code,\n"
".markdown-body pre tt {\n"
"  display: inline; padding: 0; margin: 0;\n"
"  overflow: visible; line-height: inherit;\n"
"  word-wrap: normal; background-color: transparent; border: 0;\n"
"}\n"
/* Task lists */
".markdown-body .task-list-item { list-style-type: none; }\n"
".markdown-body .task-list-item label { font-weight: 400; }\n"
".markdown-body .task-list-item+.task-list-item { margin-top: 0.25rem; }\n"
".markdown-body .task-list-item-checkbox {\n"
"  margin: 0 .2em .25em -1.4em;\n"
"  vertical-align: middle;\n"
"}\n"
/* Footnotes */
".markdown-body .footnotes {\n"
"  font-size: 12px; color: #59636e;\n"
"  border-top: 1px solid #d1d9e0;\n"
"}\n"
".markdown-body .footnotes ol { padding-left: 1rem; }\n"
/* KBD */
".markdown-body kbd {\n"
"  display: inline-block; padding: 0.25rem;\n"
"  font: 11px ui-monospace, SFMono-Regular, SF Mono, Menlo, Consolas,\n"
"       Liberation Mono, monospace;\n"
"  line-height: 10px; color: #1f2328;\n"
"  vertical-align: middle; background-color: #f6f8fa;\n"
"  border: solid 1px #d1d9e0;\n"
"  border-bottom-color: #d1d9e0;\n"
"  border-radius: 6px;\n"
"  box-shadow: inset 0 -1px 0 #d1d9e0;\n"
"}\n"
/* GitHub Alerts */
".markdown-body .markdown-alert {\n"
"  padding: 0.5rem 1rem; margin-bottom: 1rem;\n"
"  color: inherit; border-left: .25em solid #d1d9e0;\n"
"}\n"
".markdown-body .markdown-alert>:first-child { margin-top: 0; }\n"
".markdown-body .markdown-alert>:last-child { margin-bottom: 0; }\n"
".markdown-body .markdown-alert .markdown-alert-title {\n"
"  display: flex; font-weight: 500;\n"
"  align-items: center; line-height: 1;\n"
"}\n"
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
/* Syntax highlighting (prettylights) */
".markdown-body .pl-c { color: #59636e; }\n"
".markdown-body .pl-c1, .markdown-body .pl-s .pl-v { color: #0550ae; }\n"
".markdown-body .pl-e, .markdown-body .pl-en { color: #6639ba; }\n"
".markdown-body .pl-smi, .markdown-body .pl-s .pl-s1 { color: #1f2328; }\n"
".markdown-body .pl-ent { color: #0550ae; }\n"
".markdown-body .pl-k { color: #cf222e; }\n"
".markdown-body .pl-s, .markdown-body .pl-pds,\n"
".markdown-body .pl-s .pl-pse .pl-s1,\n"
".markdown-body .pl-sr, .markdown-body .pl-sr .pl-cce,\n"
".markdown-body .pl-sr .pl-sre, .markdown-body .pl-sr .pl-sra { color: #0a3069; }\n"
".markdown-body .pl-v, .markdown-body .pl-smw { color: #953800; }\n"
".markdown-body .pl-bu { color: #82071e; }\n"
".markdown-body .pl-sr .pl-cce { font-weight: bold; color: #116329; }\n"
".markdown-body .pl-ml { color: #3b2300; }\n"
".markdown-body .pl-mh, .markdown-body .pl-mh .pl-en,\n"
".markdown-body .pl-ms { font-weight: bold; color: #0550ae; }\n"
".markdown-body .pl-mi { font-style: italic; color: #1f2328; }\n"
".markdown-body .pl-mb { font-weight: bold; color: #1f2328; }\n"
".markdown-body .pl-md { color: #82071e; background-color: #ffebe9; }\n"
".markdown-body .pl-mi1 { color: #116329; background-color: #dafbe1; }\n"
".markdown-body .pl-mc { color: #953800; background-color: #ffd8b5; }\n"
".markdown-body .pl-mdr { font-weight: bold; color: #8250df; }\n"
".markdown-body .pl-ba { color: #59636e; }\n"
/* Page layout */
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
"</body>\n"
"</html>\n";

/* ── Alert definitions ──────────────────────────────────────────────── */

typedef struct {
    const char *tag;
    const char *label;
    const char *css_class;
    const char *svg;
} AlertType;

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
#define N_ALERTS (sizeof(ALERTS) / sizeof(ALERTS[0]))

/* ── Viewer lifecycle ───────────────────────────────────────────────── */

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

MdpeekViewer *viewer_new(GtkApplication *app, const char *file_path)
{
    MdpeekViewer *v = g_new0(MdpeekViewer, 1);
    v->file_path = g_strdup(file_path);

    /* Window */
    char *basename = g_path_get_basename(file_path);
    char *title = g_strdup_printf("%s \xe2\x80\x94 mdpeek", basename);
    g_free(basename);

    v->window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(v->window, title);
    gtk_window_set_default_size(v->window, 900, 700);
    g_free(title);

    /* WebView */
    v->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_window_set_child(v->window, GTK_WIDGET(v->webview));

    /* File monitor */
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

    /* Initial load */
    viewer_load_file(v);

    gtk_window_present(v->window);
    return v;
}

/* ── File change handling ───────────────────────────────────────────── */

static gboolean reload_timeout(gpointer user_data)
{
    MdpeekViewer *v = (MdpeekViewer *)user_data;
    v->reload_source_id = 0;

    /* Save scroll position, reload, restore — via JavaScript */
    webkit_web_view_evaluate_javascript(
        v->webview,
        "window.scrollY", -1, NULL, NULL, NULL, NULL, NULL);

    viewer_load_file(v);
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

    /* Debounce — reset the timer on each event */
    if (v->reload_source_id)
        g_source_remove(v->reload_source_id);
    v->reload_source_id = g_timeout_add(150, reload_timeout, v);
}

/* ── Load and render ────────────────────────────────────────────────── */

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
    g_free(body);

    char *full_html = wrap_html(transformed);
    g_free(transformed);

    webkit_web_view_load_html(v->webview, full_html, NULL);
    g_free(full_html);
}

/* ── cmark-gfm rendering ───────────────────────────────────────────── */

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

/* ── Alert block transformation ─────────────────────────────────────── */

static char *transform_alerts(const char *html)
{
    char *result = g_strdup(html);

    for (size_t i = 0; i < N_ALERTS; i++) {
        const AlertType *a = &ALERTS[i];

        char *pattern = g_strdup_printf(
            "<blockquote>\\s*<p>\\[!%s\\]\\s*(?:<br\\s*/?>)?\\s*", a->tag);

        GRegex *re = g_regex_new(pattern, G_REGEX_CASELESS, 0, NULL);
        g_free(pattern);
        if (!re) continue;

        /* Process all matches of this alert type */
        for (;;) {
            GMatchInfo *match_info = NULL;
            if (!g_regex_match(re, result, 0, &match_info)) {
                g_match_info_free(match_info);
                break;
            }

            int start_pos, content_start;
            g_match_info_fetch_pos(match_info, 0, &start_pos, &content_start);
            g_match_info_free(match_info);

            /* Find closing </blockquote> */
            const char *close = strstr(result + content_start, "</blockquote>");
            if (!close) break;

            int close_pos = (int)(close - result);
            int close_len = 13; /* strlen("</blockquote>") */

            /* Extract inner content */
            char *inner = g_strndup(result + content_start,
                                    close_pos - content_start);

            /* Build replacement */
            char *replacement = g_strdup_printf(
                "<div class='markdown-alert markdown-alert-%s'>"
                "<p class='markdown-alert-title'>%s%s</p>"
                "%s"
                "</div>",
                a->css_class, a->svg, a->label, inner);
            g_free(inner);

            /* Splice: result[0..start_pos) + replacement + result[close_pos+close_len..] */
            int old_len = close_pos + close_len - start_pos;
            int new_len = (int)strlen(replacement);
            int result_len = (int)strlen(result);
            int final_len = result_len - old_len + new_len;

            char *new_result = g_malloc(final_len + 1);
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

/* ── HTML wrapping ──────────────────────────────────────────────────── */

static char *wrap_html(const char *body)
{
    return g_strdup_printf(HTML_TEMPLATE, body);
}
