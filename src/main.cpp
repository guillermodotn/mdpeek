#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include "MarkdownViewer.h"

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

    if (path == "--help" || path == "-h") {
        usage(argv[0]);
        return 0;
    }

    // Resolve to absolute path so the file watcher works reliably
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        std::fprintf(stderr, "Error: '%s' is not a file or does not exist.\n",
                     qPrintable(path));
        return 1;
    }
    QString absPath = fi.absoluteFilePath();

    QApplication app(argc, argv);
    app.setApplicationName("mdpeek");
    app.setApplicationVersion("0.1.0");

    MarkdownViewer viewer(absPath);
    viewer.show();

    return app.exec();
}
