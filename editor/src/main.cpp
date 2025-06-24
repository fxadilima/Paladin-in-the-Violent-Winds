#include "common.h"

// Forward declarations for callback functions
static void on_activate(GtkApplication *app, gpointer user_data);
static void on_open_clicked(GtkButton *button, gpointer user_data);
static void on_save_clicked(GtkButton *button, gpointer user_data);
static void open_file_from_dialog(GtkFileChooserNative *native, gint response, gpointer user_data);
static void save_file_to_dialog(GtkFileChooserNative *native, gint response, gpointer user_data);
static void show_message_dialog(GtkWindow *parent, GtkMessageType type, const char *primary_text, const char *secondary_text);

/**
 * @brief Logic for opening a file.
 * @param editor_state Pointer to the EditorState structure.

static void do_open_file_logic(EditorState *editor_state) {
    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open File",
                                                               editor_state->main_window,
                                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                                               "_Open",
                                                               "_Cancel");
    g_signal_connect(native, "response", G_CALLBACK(open_file_from_dialog), editor_state);
    // Removed: gtk_file_chooser_set_modal(GTK_FILE_CHOOSER(native), TRUE);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}
*/
static void open_file_dialog_response_cb(GObject *source_object, GAsyncResult *res, gpointer user_data);

static void do_open_file_logic(EditorState *editor_state) {
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Open File");

    // Asynchronously open the file dialog
    gtk_file_dialog_open(dialog,
                         editor_state->main_window,
                         NULL, // Cancellable
                         open_file_dialog_response_cb,
                         editor_state); // User data
    g_object_unref(dialog); // Unref dialog after initiating open, it will be kept alive by the async operation
}

/**
 * @brief Callback for GtkFileDialog open operation.
 * @param source_object The GtkFileDialog object.
 * @param res The GAsyncResult.
 * @param user_data Pointer to the EditorState structure.
 */
static void open_file_dialog_response_cb(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file = NULL;
    char *path = NULL;
    GError *error = NULL;

    goto do_it;

cleanup:
    if (file) {
        g_object_unref(file);
    }
    if (path) {
        g_free(path);
    }
    // dialog object is unreffed in do_open_file_logic
    return;

do_it:

    file = gtk_file_dialog_open_finish(dialog, res, &error);

    if (error) {
        // Handle error (e.g., user cancelled, or actual error)
        if (error->code != G_IO_ERROR_CANCELLED) { // Don't show error if user cancelled
            std::cerr << "Error opening file dialog: " << error->message << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "File Dialog Error", error->message);
        }
        g_error_free(error);
        goto cleanup;
    }

    if (!file) {
        // User cancelled without selecting a file (no error occurred)
        goto cleanup;
    }

    path = g_file_get_path(file);
    if (!path) {
        std::cerr << "Error: No file path obtained for opening." << std::endl;
        show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "File Error", "Could not obtain file path.");
        goto cleanup;
    }

    std::ifstream input_file(path);
    if (!input_file.is_open()) {
        std::cerr << "Error: Could not open file " << path << std::endl;
        show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "File Error", ("Could not open file: " + std::string(path)).c_str());
        goto cleanup;
    }

    std::string content((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(editor_state->source_buffer), content.c_str(), -1); // -1 for null-terminated

    // Update current file path
    if (editor_state->current_filepath) {
        g_free(editor_state->current_filepath);
    }
    editor_state->current_filepath = g_strdup(path);

    // Set syntax highlighting language based on file extension
    GtkSourceLanguageManager *lang_manager = gtk_source_language_manager_get_default();
    GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_manager, "markdown"); // NULL for content type
    if (lang) {
        gtk_source_buffer_set_language(editor_state->source_buffer, lang);
    } else {
        gtk_source_buffer_set_language(editor_state->source_buffer, nullptr); // No specific language
    }

    GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
    GtkSourceStyleScheme* ss = gtk_source_style_scheme_manager_get_scheme(ssm, "Adwaita-dark");
    gtk_source_buffer_set_style_scheme(editor_state->source_buffer, ss);

    editor_state->set_window_title();
    std::cout << "Opened file: " << path << std::endl;
    goto cleanup;

}

/**
 * @brief Logic for saving a file.
 * @param editor_state Pointer to the EditorState structure.
 */
static void do_save_file_logic(EditorState *editor_state) {
    if (editor_state->current_filepath && strlen(editor_state->current_filepath) > 0) {
        // If file is already open, save directly to it
        std::ofstream output_file(editor_state->current_filepath);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not save file " << editor_state->current_filepath << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "Save Error", ("Could not save file: " + std::string(editor_state->current_filepath)).c_str());
            return;
        }

        GtkTextIter start, end;
        GtkTextBuffer* tb = GTK_TEXT_BUFFER(editor_state->source_buffer);
        gtk_text_buffer_get_start_iter(tb, &start);
        gtk_text_buffer_get_end_iter(tb, &end);
        gchar *content = gtk_text_buffer_get_text(tb, &start, &end, FALSE);

        output_file << content;
        output_file.close();

        g_free(content);
        std::cout << "Saved file: " << editor_state->current_filepath << std::endl;
        show_message_dialog(editor_state->main_window, GTK_MESSAGE_INFO, "Success", ("File saved successfully to: " + std::string(editor_state->current_filepath)).c_str());
    } else {
        // If no file is currently open, show a "Save As" dialog
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Save File",
                                                                   editor_state->main_window,
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                   "_Save",
                                                                   "_Cancel");
        //gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(native), TRUE);
        g_object_set(G_OBJECT(native), "overwrite-confirmation", TRUE, NULL);
        
        g_signal_connect(native, "response", G_CALLBACK(save_file_to_dialog), editor_state);
        //gtk_file_chooser_set_modal(GTK_FILE_CHOOSER(native), TRUE);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
    }
}


/**
 * @brief Displays a generic message dialog.
 * @param parent The parent window for the dialog.
 * @param type The type of message (e.g., GTK_MESSAGE_ERROR, GTK_MESSAGE_INFO).
 * @param primary_text The main message.
 * @param secondary_text Additional explanatory text.
 */
static void show_message_dialog(GtkWindow *parent, GtkMessageType type, const char *primary_text, const char *secondary_text) {
    GtkWidget *dialog = gtk_message_dialog_new(parent,
                                               GTK_DIALOG_MODAL,
                                               type,
                                               GTK_BUTTONS_OK,
                                               "%s", primary_text);
    if (secondary_text) {
        // Corrected: Use gtk_message_dialog_format_secondary_text
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary_text);
    }
    // In GTK4, gtk_dialog_run is removed. Dialogs are shown with gtk_window_present
    // and their lifecycle (e.g., destruction on response) is managed via signals.
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), nullptr); // Destroy dialog on response
    gtk_window_present(GTK_WINDOW(dialog)); // Show the dialog
}

/**
 * @brief Helper function to set the window title with the current file.
 * @param editor_state Pointer to the EditorState structure.
 */
static void set_window_title(EditorState *editor_state) {
    if (editor_state->current_filepath) {
        const char *filename_start = strrchr(editor_state->current_filepath, G_DIR_SEPARATOR);
        const char *display_filename = filename_start ? (filename_start + 1) : editor_state->current_filepath;
        gchar *title = g_strdup_printf("GTK4 Text Editor - %s", display_filename);
        gtk_window_set_title(editor_state->main_window, title);
        g_free(title);
    } else {
        gtk_window_set_title(editor_state->main_window, "GTK4 Text Editor - [Untitled]");
    }
}

/**
 * @brief Callback function for the "Open File" dialog's response.
 * Reads the selected file and loads its content into the GtkSourceBuffer.
 * @param native The GtkFileChooserNative instance.
 * @param response The response received from the dialog (GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL).
 * @param user_data Pointer to the EditorState structure.
 */
static void open_file_from_dialog(GtkFileChooserNative *native, gint response, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;
    GFile *file = nullptr;
    char *path = nullptr;

    if (response == GTK_RESPONSE_ACCEPT) {
        file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native));
        path = g_file_get_path(file);

        if (!path) {
            std::cerr << "Error: No file path obtained for opening." << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "File Error", "Could not obtain file path.");
            goto cleanup;
        }

        std::ifstream input_file(path);
        if (!input_file.is_open()) {
            std::cerr << "Error: Could not open file " << path << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "File Error", ("Could not open file: " + std::string(path)).c_str());
            goto cleanup;
        }

        std::string content((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(editor_state->source_buffer), content.c_str(), -1); // -1 for nullptr-terminated

        // Update current file path
        if (editor_state->current_filepath) {
            g_free(editor_state->current_filepath);
        }
        editor_state->current_filepath = g_strdup(path);

        // Set syntax highlighting language based on file extension
        GtkSourceLanguageManager *lang_manager = gtk_source_language_manager_get_default();
        GtkSourceLanguage *lang = gtk_source_language_manager_get_language(lang_manager, "markdown"); // nullptr for content type
        if (lang) {
            gtk_source_buffer_set_language(editor_state->source_buffer, lang);
        } else {
            gtk_source_buffer_set_language(editor_state->source_buffer, nullptr); // No specific language
        }

        set_window_title(editor_state);
        std::cout << "Opened file: " << path << std::endl;
    }

cleanup:
    if (file) {
        g_object_unref(file);
    }
    if (path) {
        g_free(path);
    }
    g_object_unref(native); // Dialog is done, unref it
}

/**
 * @brief Callback function for the "Save" button.
 * Initiates a file save operation, either to the current file or via a "Save As" dialog.
 * @param button The GtkButton that was clicked.
 * @param user_data Pointer to the EditorState structure.
 */
static void on_open_clicked(GtkButton *button, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;

    GtkFileChooserNative *native = gtk_file_chooser_native_new("Open File",
                                                               editor_state->main_window,
                                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                                               "_Open",
                                                               "_Cancel");
    g_signal_connect(native, "response", G_CALLBACK(open_file_from_dialog), editor_state);
    //gtk_file_chooser_set_modal(GTK_FILE_CHOOSER(native), TRUE);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
}

/**
 * @brief Callback function for the "Save File" dialog's response.
 * Writes the GtkSourceBuffer content to the selected file.
 * @param native The GtkFileChooserNative instance.
 * @param response The response received from the dialog (GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL).
 * @param user_data Pointer to the EditorState structure.
 */
static void save_file_to_dialog(GtkFileChooserNative *native, gint response, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;
    GFile *file = nullptr;
    char *path = nullptr;

    if (response == GTK_RESPONSE_ACCEPT) {
        file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(native));
        path = g_file_get_path(file);

        if (!path) {
            std::cerr << "Error: No file path obtained for saving." << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "File Error", "Could not obtain file path.");
            goto cleanup;
        }

        std::ofstream output_file(path);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not save file " << path << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "Save Error", ("Could not save file: " + std::string(path)).c_str());
            goto cleanup;
        }

        gchar *content = editor_state->getCurrentText();

        output_file << content;
        output_file.close();

        g_free(content);

        // Update current file path if it was a "Save As" operation or new file
        if (editor_state->current_filepath) {
            g_free(editor_state->current_filepath);
        }
        editor_state->current_filepath = g_strdup(path);

        set_window_title(editor_state);
        std::cout << "Saved file: " << path << std::endl;
        show_message_dialog(editor_state->main_window, GTK_MESSAGE_INFO, "Success", ("File saved successfully to: " + std::string(path)).c_str());
    }

cleanup:
    if (file) {
        g_object_unref(file);
    }
    if (path) {
        g_free(path);
    }
    g_object_unref(native); // Dialog is done, unref it
}

/**
 * @brief Callback function for the "Save" button.
 * Initiates a file save operation, either to the current file or via a "Save As" dialog.
 * @param button The GtkButton that was clicked.
 * @param user_data Pointer to the EditorState structure.
 */
static void on_save_clicked(GtkButton *button, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;

    if (editor_state->current_filepath && strlen(editor_state->current_filepath) > 0) {
        // If file is already open, save directly to it
        std::ofstream output_file(editor_state->current_filepath);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not save file " << editor_state->current_filepath << std::endl;
            show_message_dialog(editor_state->main_window, GTK_MESSAGE_ERROR, "Save Error", ("Could not save file: " + std::string(editor_state->current_filepath)).c_str());
            return;
        }

        gchar *content = editor_state->getCurrentText();

        output_file << content;
        output_file.close();

        g_free(content);
        std::cout << "Saved file: " << editor_state->current_filepath << std::endl;
        show_message_dialog(editor_state->main_window, GTK_MESSAGE_INFO, "Success", ("File saved successfully to: " + std::string(editor_state->current_filepath)).c_str());
    } else {
        // If no file is currently open, show a "Save As" dialog
        GtkFileChooserNative *native = gtk_file_chooser_native_new("Save File",
                                                                   editor_state->main_window,
                                                                   GTK_FILE_CHOOSER_ACTION_SAVE,
                                                                   "_Save",
                                                                   "_Cancel");
        //gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(native), TRUE);
        g_object_set(G_OBJECT(native), "overwrite-confirmation", TRUE, NULL);

        g_signal_connect(native, "response", G_CALLBACK(save_file_to_dialog), editor_state);
        //gtk_file_chooser_set_modal(GTK_FILE_CHOOSER(native), TRUE);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(native));
    }
}

/**
 * @brief Callback for "app.open" action activation.
 * @param action The GSimpleAction that was activated.
 * @param parameter The parameter passed to the action (unused here).
 * @param user_data Pointer to the EditorState structure.
 */
static void on_app_open_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;
    do_open_file_logic(editor_state);
}

/**
 * @brief Callback for "app.save" action activation.
 * @param action The GSimpleAction that was activated.
 * @param parameter The parameter passed to the action (unused here).
 * @param user_data Pointer to the EditorState structure.
 */
static void on_app_save_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
    EditorState *editor_state = (EditorState *)user_data;
    do_save_file_logic(editor_state);
}

static void on_close_app(GApplication*, EditorState* es) {
    es->application = nullptr;
    delete es;
}

/**
 * @brief The 'activate' signal handler for the GtkApplication.
 * This is where the main window and its contents are created.
 * @param app The GtkApplication instance.
 * @param user_data User-defined data, typically nullptr for activate.
 */
static void on_activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GtkWidget *header_bar;
    GtkWidget *open_button;
    GtkWidget *save_button;
    GtkWidget *scrolled_window;

    // Allocate and initialize EditorState
    EditorState *editor_state = new EditorState(app);

    // Create the main application window
    window = gtk_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    editor_state->main_window = GTK_WINDOW(window); // Store a pointer to the main window
    set_window_title(editor_state); // Set initial title

    // Create a header bar
    header_bar = gtk_header_bar_new();
    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

    // --- Header Bar Buttons ---
    // Open button
    open_button = gtk_button_new_from_icon_name("document-open-symbolic");
    gtk_widget_set_tooltip_text(open_button, "Open File (Ctrl+O)");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), open_button);
    // Connect the clicked signal to our C-style callback function
    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_clicked), editor_state);

    // Save button
    save_button = gtk_button_new_from_icon_name("document-save-symbolic");
    gtk_widget_set_tooltip_text(save_button, "Save File (Ctrl+S)");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), save_button);
    // Connect the clicked signal to our C-style callback function
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_clicked), editor_state);


    // --- Text Editor Area (GtkSourceView) ---
    // Create a GtkSourceBuffer
    editor_state->source_buffer = gtk_source_buffer_new(nullptr); // nullptr for no GTK text tag table
    gtk_source_buffer_set_highlight_syntax(editor_state->source_buffer, TRUE);
    gtk_source_buffer_set_highlight_matching_brackets(editor_state->source_buffer, TRUE);

    // Create a GtkSourceView and associate it with the buffer
    GtkWidget *source_view = gtk_source_view_new_with_buffer(editor_state->source_buffer);
    GtkSourceView* sv = GTK_SOURCE_VIEW(source_view);
    gtk_source_view_set_auto_indent(sv, TRUE);
    gtk_source_view_set_show_line_numbers(sv, TRUE);
    gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(sv), 4);
    gtk_source_view_set_insert_spaces_instead_of_tabs(sv, TRUE);

    GtkTextView* tv = GTK_TEXT_VIEW(sv);

    gtk_text_view_set_top_margin(tv, 3);
    // Wrap the text
    gtk_text_view_set_wrap_mode(tv, GTK_WRAP_WORD);

    GdkDisplay* display = gdk_display_get_default ();
    GtkCssProvider* provider = gtk_css_provider_new();

    ///////////////////////////////////////////
    // In Gtk4, the display dont't have screen.
    // We use this instead.
    //////////////////////////////////////////// 
    gtk_style_context_add_provider_for_display(display, 
        GTK_STYLE_PROVIDER(provider), 
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    const char* css_data = "textview {font: 14px \"Consolas\";}";

    ////////////////////////////////////////////////////////////////////////////////////
    // Note: This one is deprecated in Gtk 4
    //   GError* error = NULL;
    //   gtk_css_provider_load_from_file(provider, g_file_new_for_path("my.css"), &error);
    /////////////////////////////////////////////////////////////////////////////////////

    gtk_css_provider_load_from_string(provider, css_data);

    GtkSourceStyleSchemeManager* ssm = gtk_source_style_scheme_manager_get_default();
    GtkSourceStyleScheme* ss = gtk_source_style_scheme_manager_get_scheme(ssm, "Adwaita-dark");
    gtk_source_buffer_set_style_scheme(editor_state->source_buffer, ss);

    // Place the GtkSourceView inside a ScrolledWindow
    scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), source_view);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    gtk_window_set_child(GTK_WINDOW(window), scrolled_window); // Set scrolled window as main window's child

    // --- Keyboard Shortcuts using GAction ---
    GAction *open_action = G_ACTION(g_simple_action_new("open", nullptr));
    g_signal_connect(open_action, "activate", G_CALLBACK(on_app_open_activated), editor_state);
    g_action_map_add_action(G_ACTION_MAP(app), open_action);

    GAction *save_action = G_ACTION(g_simple_action_new("save", nullptr));
    g_signal_connect(save_action, "activate", G_CALLBACK(on_app_save_activated), editor_state);
    g_action_map_add_action(G_ACTION_MAP(app), save_action);

    // Set accelerators for the actions
    // Declare named arrays for accelerators to avoid "address of temporary array" error
    const char *open_accels[] = {"<Ctrl>o", nullptr};
    const char *save_accels[] = {"<Ctrl>s", nullptr};
    gtk_application_set_accels_for_action(app, "app.open", open_accels);
    gtk_application_set_accels_for_action(app, "app.save", save_accels);

    // Show all widgets
    gtk_window_present(GTK_WINDOW(window));

    // When the window is closed, free the allocated EditorState
    g_signal_connect(window, "destroy", G_CALLBACK(on_close_app), editor_state);
}

int main(int argc, char *argv[]) {
    GtkApplication *app;
    int status;

    // Create a new GtkApplication instance
    app = gtk_application_new("org.gtk.texteditor.plainc", G_APPLICATION_DEFAULT_FLAGS);

    // Connect the "activate" signal to our on_activate callback
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);

    // Run the application
    status = g_application_run(G_APPLICATION(app), argc, argv);

    // Release the application object
    g_object_unref(app);

    return status;
}


