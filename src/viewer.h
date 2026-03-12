#ifndef VIEWER_H
#define VIEWER_H

#include <adwaita.h>
#include <webkit/webkit.h>

typedef struct {
    AdwApplicationWindow *window;
    WebKitWebView        *webview;
    GFileMonitor         *monitor;
    char                 *file_path;
    guint                 reload_source_id;
} MdpeekViewer;

/* Create a viewer for the given file path. The viewer is owned by the
 * AdwApplication and will be destroyed when the window closes. */
MdpeekViewer *viewer_new(AdwApplication *app, const char *file_path);

/* Load (or reload) the markdown file and render it. */
void viewer_load_file(MdpeekViewer *viewer);

#endif /* VIEWER_H */
