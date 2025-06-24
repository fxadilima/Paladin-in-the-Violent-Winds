#include "common.h"

/**
 * @brief Helper function to set the window title with the current file.
 * @param editor_state Pointer to the EditorState structure.
 */
void EditorState::set_window_title() {
    if (current_filepath) {
        const char *filename_start = strrchr(current_filepath, G_DIR_SEPARATOR);
        const char *display_filename = filename_start ? (filename_start + 1) : current_filepath;
        gchar *title = g_strdup_printf("GTK4 Text Editor - %s", display_filename);
        gtk_window_set_title(main_window, title);
        g_free(title);
    } else {
        gtk_window_set_title(main_window, "GTK4 Text Editor - [Untitled]");
    }
}

gchar* EditorState::getCurrentText() {
    if (!source_buffer) return nullptr;
    GtkTextIter start, end;
    GtkTextBuffer* tb = GTK_TEXT_BUFFER(source_buffer);
    gtk_text_buffer_get_start_iter(tb, &start);
    gtk_text_buffer_get_end_iter(tb, &end);
    gchar *content = gtk_text_buffer_get_text(tb, &start, &end, FALSE);
    return content;
}

