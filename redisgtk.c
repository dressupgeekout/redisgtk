/*
 * redisgtk.c -- GTK front-end to the Redis server
 * Christian Koch <cfkoch@sdf.lonestar.org>
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <vte/vte.h>

#define SPECIALPID -99 /* Arbitrary number less than -1 */

static GPid pid = SPECIALPID;

static void window_destroy(GtkWidget* win, gpointer _);
static void start_server_btn_clicked(GtkWidget* btn, GtkWidget** widgets);
static void stop_server_btn_clicked(GtkWidget* btn, GtkWidget** widgets);


static void
window_destroy(GtkWidget* win, gpointer _)
{
  gtk_main_quit();
}


/*
 * Pass-in order: terminal, host_entry, port_entry, timeout_entry,
 * ndb_entry, dumpfloc_entry
 */
static void
start_server_btn_clicked(GtkWidget* btn, GtkWidget** widgets)
{
  GtkWidget* terminal = widgets[0];
  GtkWidget* host_entry = widgets[1];
  GtkWidget* port_entry = widgets[2];
  GtkWidget* timeout_entry = widgets[3];
  GtkWidget* ndb_entry = widgets[4];
  GtkWidget* dumpfloc_entry = widgets[5];

  if (pid > 0) {
    const char* msg = "\e[1;31m---(server is already running)---\e[0m\r\n";
    vte_terminal_feed(VTE_TERMINAL(terminal), msg, strlen(msg));
    return;
  }

  /* Convert the ndb int value to a string. */
  char* ndb;
  ndb = malloc(4);
  snprintf(
    ndb, 4, "%d",
    gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ndb_entry))
  );

  char* argv[] = {
    REDIS_SERVER_LOC,
    "--bind", gtk_entry_get_text(GTK_ENTRY(host_entry)),
    "--port", gtk_entry_get_text(GTK_ENTRY(port_entry)),
    "--timeout", gtk_entry_get_text(GTK_ENTRY(timeout_entry)),
    "--databases", ndb,
    "--dbfilename", gtk_entry_get_text(GTK_ENTRY(dumpfloc_entry)),
    NULL
  };

  /* Announce the command line. */
  int i = 0;
  while (argv[i] != NULL) {
    vte_terminal_feed(VTE_TERMINAL(terminal), "\e[0;32m", 7);
    vte_terminal_feed(VTE_TERMINAL(terminal), argv[i], strlen(argv[i]));
    vte_terminal_feed(VTE_TERMINAL(terminal), " ", 1);
    i++;
  }

  vte_terminal_feed(VTE_TERMINAL(terminal), "\e[0m\r\n", 6);

  /* The actual fork. */
  vte_terminal_fork_command_full(
    VTE_TERMINAL(terminal), VTE_PTY_NO_HELPER, NULL, argv, NULL, 0, NULL,
    NULL, &pid, NULL
  );

  free(ndb);
}


/*
 * Pass-in order: terminal
 */
static void
stop_server_btn_clicked(GtkWidget* btn, GtkWidget** widgets)
{
  GtkWidget* terminal = widgets[0];
  const char* msg = "\e[1;33m---(killed)---\e[0m\r\n";

  if (pid < 0) {
    const char* msg = "\e[1;31m---(server is not yet running)---\e[0m\r\n";
    vte_terminal_feed(VTE_TERMINAL(terminal), msg, strlen(msg));
    return;
  }

  kill(pid, SIGINT);
  vte_terminal_feed(VTE_TERMINAL(terminal), msg, strlen(msg));
  pid = SPECIALPID;
}


int
main(int argc, char* argv[])
{
  GtkWidget* window;
  GtkWidget* notebook;

  GtkWidget *terminal;
  VtePty* terminal_pty;

  GtkWidget *terminal_page_vbox, *config_page_vbox;
  GtkWidget *terminal_page_label, *config_page_label;

  GtkWidget *start_server_btn, *stop_server_btn;

  GtkWidget *host_entry_label, *host_entry;
  GtkWidget *port_entry_label, *port_entry;
  GtkWidget *timeout_entry_label, *timeout_entry;
  GtkWidget *ndb_entry_label, *ndb_entry;
  GtkWidget *dumpfloc_entry_label, *dumpfloc_entry;

  /* */

  gtk_init(&argc, &argv);

  /* */

  host_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(host_entry), "127.0.0.1");
  host_entry_label = gtk_label_new("Host");
  gtk_widget_set_tooltip_text(host_entry, "Specifies an interface.");
  
  port_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(port_entry), "6379");
  port_entry_label = gtk_label_new("Port");
  gtk_widget_set_tooltip_text(port_entry,
                              "Specifies the port on which to listen for "
                              "incoming connections.");

  timeout_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(timeout_entry), "0");
  timeout_entry_label = gtk_label_new("Timeout (0 to disable)");
  gtk_widget_set_tooltip_text(timeout_entry,
                              "Close a client connection if it is idle for "
                              "this number of seconds.");

  GtkObject* ndb_entry_adj = gtk_adjustment_new(16, 1, 99, 1, 1, 0); 
  ndb_entry = gtk_spin_button_new(GTK_ADJUSTMENT(ndb_entry_adj), 1, 0);
  ndb_entry_label = gtk_label_new("Number of databases");
  gtk_widget_set_tooltip_text(ndb_entry,
                              "One Redis server can maintain multiple "
                              "keyspaces. This option indicates how many.");

  dumpfloc_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(dumpfloc_entry), "dump.rdb");
  dumpfloc_entry_label = gtk_label_new("Dumpfile location");
  gtk_widget_set_tooltip_text(dumpfloc_entry,
                              "Specifies where to save/load the database "
                              "file.");

  /* */

  config_page_vbox = gtk_vbox_new(FALSE, 0);
  config_page_label = gtk_label_new("Configuration");

  terminal_page_vbox = gtk_vbox_new(FALSE, 0);
  terminal_page_label = gtk_label_new("Terminal");

  /* */

  /* The terminal emulator. */

  /* XXX This doesn't do anything... */
  terminal_pty = vte_pty_new(VTE_PTY_NO_HELPER, NULL);
  vte_pty_set_size(terminal_pty, 50, 80, NULL);

  terminal = vte_terminal_new();
  vte_terminal_set_pty_object(VTE_TERMINAL(terminal), terminal_pty);
  vte_terminal_set_audible_bell(VTE_TERMINAL(terminal), FALSE);
  vte_terminal_set_visible_bell(VTE_TERMINAL(terminal), FALSE);
  vte_terminal_set_scroll_on_output(VTE_TERMINAL(terminal), TRUE);
  vte_terminal_set_allow_bold(VTE_TERMINAL(terminal), TRUE);
  vte_terminal_set_cursor_shape(VTE_TERMINAL(terminal), VTE_CURSOR_SHAPE_BLOCK);
  vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(terminal), VTE_CURSOR_BLINK_OFF);

  start_server_btn = gtk_button_new_with_label("Start server");
  GtkWidget* start_server_btn_pass[] = {
    terminal, host_entry, port_entry, timeout_entry, ndb_entry,
    dumpfloc_entry
  };
  g_signal_connect(start_server_btn,
                   "clicked",
                   G_CALLBACK(start_server_btn_clicked),
                   start_server_btn_pass);

  stop_server_btn = gtk_button_new_with_label("Stop server");
  GtkWidget* stop_server_btn_pass[] = { terminal };
  g_signal_connect(stop_server_btn,
                   "clicked",
                   G_CALLBACK(stop_server_btn_clicked),
                   stop_server_btn_pass);

  /* */

  /* Packing stuff. */
  gtk_box_pack_start(GTK_BOX(config_page_vbox), host_entry_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), host_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), port_entry_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), port_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), timeout_entry_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), timeout_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), ndb_entry_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), ndb_entry, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), dumpfloc_entry_label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(config_page_vbox), dumpfloc_entry, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(terminal_page_vbox), terminal, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(terminal_page_vbox), start_server_btn, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(terminal_page_vbox), stop_server_btn, FALSE, FALSE, 0);

  /* */

  /* The notebook organizer. */
  notebook = gtk_notebook_new();
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), config_page_vbox, config_page_label);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), terminal_page_vbox, terminal_page_label);

  /* */

  /* The top-level window. */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Redis");
  g_signal_connect(window, "destroy", G_CALLBACK(window_destroy), NULL);
  gtk_container_add(GTK_CONTAINER(window), notebook);

  /* */

  /* All that for this: */
  gtk_widget_show_all(window);
  gtk_main();
  return EXIT_SUCCESS;
}
