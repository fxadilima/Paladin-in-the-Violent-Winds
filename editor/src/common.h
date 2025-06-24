#pragma once

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <iostream> // For std::cerr, std::ifstream, std::ofstream
#include <fstream>  // For file I/O
#include <string>   // For std::string

// Structure to hold pointers to essential widgets and data
// This replaces the class members in the gtkmm version,
// allowing us to pass editor state to C-style callbacks.
struct EditorState {
    GtkSourceBuffer *source_buffer;
    GtkWindow *main_window;
    char *current_filepath; // Using char* for C compatibility, manage with g_free
    GtkApplication *application; // Store a reference to the application for actions

    EditorState(GtkApplication* app) : application(app), 
        source_buffer(nullptr),
        main_window(nullptr),
        current_filepath(nullptr) {
    }
    void set_window_title();
    gchar *getCurrentText();
};
