#include <stdlib.h>
#include <gtk/gtk.h>
#include "board.h"

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 3

struct HandlerData
{
    Board* board;
    GtkWidget* grid;
    struct Position position;
};

static void set_remaining_mines_label(Board* board, GtkWidget* label)
{
    char labelText[10];
    sprintf(labelText, "%d", board->mines - board->flags);
    gtk_label_set_text(GTK_LABEL(label), labelText);
}

static void disconnect_grid_handlers(GtkWidget* grid, Board* board)
{
    GtkWidget *imageTile;
    GtkGestureClick *click;
    GtkEventControllerMotion* mouseMotion;

    for(int i = 0; i < board->rows; i++)
    {
        for(int j = 0; j < board->columns; j++)
        {
            imageTile = gtk_grid_get_child_at(GTK_GRID(grid), j, i);
            click = g_object_get_data(G_OBJECT(imageTile), "left click");
            g_signal_handlers_destroy(click);
            click = g_object_get_data(G_OBJECT(imageTile), "right click");
            g_signal_handlers_destroy(click);
            mouseMotion = g_object_get_data(G_OBJECT(imageTile), "mouse enter");
            g_signal_handlers_destroy(mouseMotion);
            mouseMotion = g_object_get_data(G_OBJECT(imageTile), "mouse leave");
            g_signal_handlers_destroy(mouseMotion);
        }
    }
}

static void set_tile_number(GtkWidget* tileImage, int neighbouringMines)
{
    switch(neighbouringMines)
    {
        case 1:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 1.png");
            break;
        case 2:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 2.png");
            break;
        case 3:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 3.png");
            break;
        case 4:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 4.png");
            break;
        case 5:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 5.png");
            break;
        case 6:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 6.png");
            break;
        case 7:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 7.png");
            break;
        case 8:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile 8.png");
            break;
        default:
            gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile blank.png");
            break;
    }
}

static void update_grid(GtkWidget* grid, Board* board, struct Position lastTileClicked)
{
    GtkWidget *tileImage;
    for(int i = 0; i < board->rows; i++)
    {
        for(int j = 0; j < board->columns; j++)
        {
            tileImage = gtk_grid_get_child_at(GTK_GRID(grid), j, i);
            if(get_tile_state(board, i, j) == Revealed)
                set_tile_number(tileImage, board->tiles[i][j].neighbouringMines);
            if(get_tile_state(board, i, j) == Covered && board->tiles[i][j].hasMine && board->gameLose)
                gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile mine.png");
            if(get_tile_state(board, i, j) == Flagged && !board->tiles[i][j].hasMine && board->gameLose)
                gtk_image_set_from_file(GTK_IMAGE(tileImage), "images/tile X.png");
        }
    }
    if(board->gameLose)
        gtk_image_set_from_file(GTK_IMAGE(gtk_grid_get_child_at(GTK_GRID(grid), lastTileClicked.col, lastTileClicked.row)), "images/tile mine revealed.png");
}

static void tile_leave(GtkEventControllerMotion* controller, double x, double y, gpointer user_data)
{
    GtkWidget *imageTile = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

    struct HandlerData *data = user_data;
    //g_print("Exited %d %d\n", data->position.row, data->position.col);

    struct Position pos = data->position;

    if(get_tile_state(data->board, pos.row, pos.col) == Covered)
        gtk_image_set_from_file(GTK_IMAGE(imageTile), "images/tile covered.png");
    else if(get_tile_state(data->board, pos.row, pos.col) == Flagged)
        gtk_image_set_from_file(GTK_IMAGE(imageTile), "images/tile flagged.png");
}

static void tile_enter(GtkEventControllerMotion* controller, double x, double y, gpointer user_data)
{
    GtkWidget *imageTile = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));

    struct HandlerData *data = user_data;
    //g_print("Hovered over %d %d\n", data->position.row, data->position.col);

    struct Position pos = data->position;
    if(get_tile_state(data->board, pos.row, pos.col) == Covered)
        gtk_image_set_from_file(GTK_IMAGE(imageTile), "images/tile hover.png");
    else if(get_tile_state(data->board, pos.row, pos.col) == Flagged)
        gtk_image_set_from_file(GTK_IMAGE(imageTile), "images/tile flagged hover.png");
}

static void tile_left_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    GtkWidget *label = g_object_get_data(G_OBJECT(gesture), "Label");
    struct HandlerData *data = user_data;
    //g_print("left click from %d %d\n", data->position.row, data->position.col);

    struct Position pos = data->position;
    if(get_tile_state(data->board, pos.row, pos.col) == Covered)
    {
        reveal_tile(data->board, pos.row, pos.col);

        set_remaining_mines_label(data->board, label);

        update_grid(data->grid, data->board, pos);
        int win = check_win(data->board);
        int lose = data->board->gameLose;
        if(win || lose)
        {
            if (win)
                gtk_label_set_text(GTK_LABEL(label), "You Win!");
            if (lose)
                gtk_label_set_text(GTK_LABEL(label), "You Lose!");
            disconnect_grid_handlers(data->grid, data->board);
        }
    }
}

static void tile_right_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    GtkWidget *imageTile = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
    GtkWidget *label = g_object_get_data(G_OBJECT(gesture), "Label");

    struct HandlerData *data = user_data;
    struct Position pos = data->position;
    if(get_tile_state(data->board, pos.row, pos.col) == Covered)
    {
        flag_tile(data->board, data->position.row, data->position.col);
        gtk_image_set_from_file(GTK_IMAGE(imageTile), "images/tile flagged hover.png");
    }
    else if(get_tile_state(data->board, pos.row, pos.col) == Flagged)
    {
        cover_tile(data->board, data->position.row, data->position.col);
        gtk_image_set_from_file(GTK_IMAGE(imageTile), "images/tile hover.png");
    }

    set_remaining_mines_label(data->board, label);

    //g_print("right click from %d %d\n", data->position.row, data->position.col);
}

static void tile_destroy(GtkWidget* widget, gpointer user_data)
{
    struct HandlerData *data = user_data;
    //g_print("Bye from %d %d\n", data->position.row, data->position.col);
    free(data);
}

static void window_destroy(GtkWidget* widget, gpointer user_data)
{
    Board *board = user_data;
    destroy_board(board);
}

static void add_tiles_to_grid(Board* board, GtkWidget* grid, GtkWidget *label)
{
    GtkWidget *image;

    GtkGesture *click;
    GtkEventController *mouseMotion;

    for(int i = 0; i < board->rows; i++)
    {
        for(int j = 0; j < board->columns; j++)
        {
            struct HandlerData *data = malloc(sizeof *data);
            data->board = board;
            data->grid = grid;
            data->position.row = i;
            data->position.col = j;

            image = gtk_image_new_from_file ("images/tile covered.png");
            gtk_image_set_pixel_size(GTK_IMAGE(image), 50);

            click = gtk_gesture_click_new();
            gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), MOUSE_LEFT);

            g_signal_connect(click, "pressed", G_CALLBACK(tile_left_clicked), data);
            gtk_widget_add_controller(image, GTK_EVENT_CONTROLLER(click));
            g_object_set_data(G_OBJECT(image), "left click", click);
            g_object_set_data(G_OBJECT(click), "Label", label);

            click = gtk_gesture_click_new();
            gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), MOUSE_RIGHT);

            g_signal_connect(click, "pressed", G_CALLBACK(tile_right_clicked), data);
            gtk_widget_add_controller(image, GTK_EVENT_CONTROLLER(click));
            g_object_set_data(G_OBJECT(image), "right click", click);
            g_object_set_data(G_OBJECT(click), "Label", label);

            mouseMotion = gtk_event_controller_motion_new();
            g_signal_connect(mouseMotion, "enter", G_CALLBACK(tile_enter), data);
            gtk_widget_add_controller(image, mouseMotion);
            g_object_set_data(G_OBJECT(image), "mouse enter", mouseMotion);

            mouseMotion = gtk_event_controller_motion_new();
            g_signal_connect(mouseMotion, "leave", G_CALLBACK(tile_leave), data);
            gtk_widget_add_controller(image, mouseMotion);
            g_object_set_data(G_OBJECT(image), "mouse leave", mouseMotion);

            g_signal_connect_after(image, "destroy", G_CALLBACK(tile_destroy), data);

            gtk_grid_attach(GTK_GRID(grid), image, j, i, 1, 1);
        }
    }
}

static void create_main_window_scene(GtkWidget* window, Board* board)
{
    GtkWidget *vbox;
    GtkWidget *label;

    GtkWidget *grid;

    label = gtk_label_new("");
    set_remaining_mines_label(board, label);

    grid = gtk_grid_new();

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top (vbox, 8);
    gtk_box_append(GTK_BOX(vbox), label);
    gtk_box_append(GTK_BOX(vbox), grid);

    gtk_window_set_child(GTK_WINDOW(window), vbox);

    g_object_set_data(G_OBJECT(window), "grid", grid);
    g_object_set_data(G_OBJECT(window), "label", label);

    add_tiles_to_grid(board, grid, label);
}

static void clear_grid(GtkWidget* grid)
{
    GtkWidget *iter = gtk_widget_get_first_child (grid);
    while (iter != NULL) {
      GtkWidget *next = gtk_widget_get_next_sibling (iter);
      gtk_grid_remove (GTK_GRID(grid), iter);
      iter = next;
    }
}

static gboolean on_key_press(GtkEventControllerKey* controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
    if(keycode != GDK_KEY_R)
        return FALSE;

    GtkWidget *window = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller));
    GtkWidget *grid = g_object_get_data(G_OBJECT(window), "grid");
    GtkWidget *label = g_object_get_data(G_OBJECT(window), "label");
    Board *board = user_data;
    clear_grid(grid);
    add_tiles_to_grid(board, grid, label);

    regenerate_board(board);

    set_remaining_mines_label(board, label);

    update_grid(grid, board, (struct Position){.row = 0, .col = 0});

    return TRUE;
}

static void on_difficulty_dialog_response(GtkDialog * dialog, int response, gpointer user_data)
{
    Board *board;
    GtkEventController *keyPress;
    GtkApplication *app = user_data;
    GtkWidget *window = g_object_get_data(G_OBJECT(dialog), "Main Window");
    int *difficulty = g_object_get_data(G_OBJECT(dialog), "Difficulty");


    if (response == GTK_RESPONSE_OK)
    {
        if(*difficulty == 0)
            board = create_board(8, 10, 10);
        else if(*difficulty == 1)
            board = create_board(14, 18, 40);
        else
            board = create_board(18, 24, 99);

        if(board == NULL)
            exit(EXIT_FAILURE);

        create_main_window_scene(window, board);
        g_signal_connect_after(window, "destroy", G_CALLBACK(window_destroy), board);

        keyPress = gtk_event_controller_key_new();
        g_signal_connect(keyPress, "key-pressed", G_CALLBACK(on_key_press), board);
        gtk_widget_add_controller(window, keyPress);

        gtk_window_close(GTK_WINDOW(dialog));
        gtk_widget_show(window);
    }
    else if(response == GTK_RESPONSE_CANCEL)
    {
        gtk_window_close(GTK_WINDOW(dialog));
        g_application_quit(G_APPLICATION(app));
    }
    else
    {
        free(difficulty);
    }
}

static void difficulty_changed(GtkWidget* widget, gpointer user_data)
{
    int *difficulty = user_data;
    if (strcmp(gtk_check_button_get_label(GTK_CHECK_BUTTON(widget)), "Easy") == 0 && gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)))
        *difficulty = 0;
    else if (strcmp(gtk_check_button_get_label(GTK_CHECK_BUTTON(widget)), "Medium") == 0 && gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)))
        *difficulty = 1;
    else if (strcmp(gtk_check_button_get_label(GTK_CHECK_BUTTON(widget)), "Hard") == 0 && gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)))
        *difficulty = 2;
}

static void activate_main_window(GtkApplication *app, gpointer data)
{
    GtkWidget *window;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Minesweeper");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    GtkWidget *dialog;

    GtkWidget *content_area;
    dialog = gtk_dialog_new_with_buttons("Difficulty", GTK_WINDOW (window), GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT|GTK_DIALOG_USE_HEADER_BAR,
                                        "_OK", GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
    gtk_widget_set_halign(content_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content_area, GTK_ALIGN_CENTER);

    GtkWidget *easyButton = gtk_check_button_new_with_label("Easy");
    gtk_box_append(GTK_BOX(content_area), easyButton);

    GtkWidget *mediumButton = gtk_check_button_new_with_label("Medium");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(mediumButton), TRUE);
    gtk_box_append(GTK_BOX(content_area), mediumButton);

    GtkWidget *hardButton = gtk_check_button_new_with_label("Hard");
    gtk_box_append(GTK_BOX(content_area), hardButton);

    gtk_check_button_set_group(GTK_CHECK_BUTTON(easyButton), GTK_CHECK_BUTTON(mediumButton));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(hardButton), GTK_CHECK_BUTTON(mediumButton));

    int *difficulty = malloc(sizeof(int));
    *difficulty = 1;

    g_object_set_data(G_OBJECT(dialog), "Difficulty", difficulty);
    g_object_set_data(G_OBJECT(dialog), "Main Window", window);

    g_signal_connect(easyButton, "toggled", G_CALLBACK(difficulty_changed), difficulty);
    g_signal_connect(mediumButton, "toggled", G_CALLBACK(difficulty_changed), difficulty);
    g_signal_connect(hardButton, "toggled", G_CALLBACK(difficulty_changed), difficulty);
    g_signal_connect(dialog, "response", G_CALLBACK (on_difficulty_dialog_response), app);

    gtk_widget_show (dialog);
}

int main(int argc, char **argv) {


    GtkApplication *app;
    int status;

    app = gtk_application_new("org.nick.minesweeper", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate_main_window), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
