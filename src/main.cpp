#ifdef MDPEEK_USE_QT

#include "viewer.h"

#include <QApplication>
#include <QFileInfo>
#include <QFile>

#include <cstdio>

static void usage(const char *prog)
{
    std::fprintf(stderr, "Usage: %s <file.md>\n", prog);
    std::fprintf(stderr, "\nPreview a Markdown file with live reload.\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    QString path = QString::fromLocal8Bit(argv[1]);

    if (path == QStringLiteral("--help") || path == QStringLiteral("-h")) {
        usage(argv[0]);
        return 0;
    }

    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        std::fprintf(stderr, "Error: '%s' is not a file or does not exist.\n",
                     qPrintable(path));
        return 1;
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("mdpeek"));
    app.setApplicationVersion(QStringLiteral(MDPEEK_VERSION));

    MdpeekViewer viewer(fi.absoluteFilePath());
    viewer.show();

    return app.exec();
}

#else /* GTK */

#include "viewer.h"

#include <adwaita.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *g_file_path = NULL;

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <file.md>\n", prog);
    fprintf(stderr, "\nPreview a Markdown file with live reload.\n");
}

static void on_activate(AdwApplication *app, gpointer user_data)
{
    (void)user_data;
    viewer_new(app, g_file_path);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        usage(argv[0]);
        return 0;
    }

    /* Resolve to absolute path */
    char *abs_path = realpath(argv[1], NULL);
    if (!abs_path) {
        fprintf(stderr, "Error: '%s' does not exist.\n", argv[1]);
        return 1;
    }

    if (!g_file_test(abs_path, G_FILE_TEST_IS_REGULAR)) {
        fprintf(stderr, "Error: '%s' is not a regular file.\n", abs_path);
        free(abs_path);
        return 1;
    }

    g_file_path = abs_path;

    /* Silence noisy GTK/WebKit/Mesa warnings on stderr */
    freopen("/dev/null", "w", stderr);

    AdwApplication *app = adw_application_new(
        "io.github.guillermodotn.mdpeek",
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    /* Pass only argv[0] so GTK does not interpret the file path as an option */
    int gtk_argc = 1;
    int status = g_application_run(G_APPLICATION(app), gtk_argc, argv);
    g_object_unref(app);
    free(g_file_path);
    return status;
}

#endif /* MDPEEK_USE_QT */
