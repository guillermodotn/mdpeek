#include "viewer.h"

#include <stdio.h>
#include <string.h>

static char *g_file_path = NULL;

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <file.md>\n", prog);
    fprintf(stderr, "\nPreview a Markdown file with live reload.\n");
}

static void activate(AdwApplication *app, gpointer user_data)
{
    (void)user_data;
    viewer_new(app, g_file_path);
}

int main(int argc, char *argv[])
{
    /* Parse args before GTK consumes them */
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

    AdwApplication *app = adw_application_new("com.github.gleiro.mdpeek",
                                               G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    /* GTK expects its own argc/argv — pass only argv[0] so it doesn't
     * try to interpret the markdown file path as a GTK option. */
    int gtk_argc = 1;
    int status = g_application_run(G_APPLICATION(app), gtk_argc, argv);

    g_object_unref(app);
    free(g_file_path);
    return status;
}
