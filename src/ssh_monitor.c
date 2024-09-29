// ssh_monitor.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "../include/ssh_monitor.h"

// Function to retrieve your current `pts` session (ignoring tty1)
char* get_own_pts() {
    static char own_pts[64];
    FILE* fp = popen("who am i | awk '{print $2}'", "r"); // Using 'who am i' to get your current pts
    if (fp) {
        fgets(own_pts, sizeof(own_pts), fp);
        strtok(own_pts, "\n"); // Remove newline character
        pclose(fp);
    }
    return own_pts;
}

// Function to retrieve active SSH connections using `who`
SSHConnection* get_ssh_connections(int* count) {
    FILE* fp = popen("who | grep 'pts/'", "r");  // Get all active pts connections from `who`
    if (!fp) {
        printf("Failed to run who command\n");
        return NULL;
    }

    char buffer[256];
    SSHConnection* connections = malloc(sizeof(SSHConnection) * 10); // Adjust size as needed
    *count = 0;

    printf("Reading SSH connections...\n");

    while (fgets(buffer, sizeof(buffer), fp)) {
        char username[64] = "", pts[16] = "", ip_address[64] = "";

        printf("who output line: %s", buffer); // Debug statement to print each line

        // Extract the username and pts using sscanf
        int matched = sscanf(buffer, "%s %s", username, pts);
        
        if (matched == 2) {
            // Find the IP address within parentheses
            char* ip_start = strchr(buffer, '(');
            char* ip_end = strchr(buffer, ')');

            if (ip_start && ip_end && ip_end > ip_start) {
                size_t ip_length = ip_end - ip_start - 1;
                strncpy(ip_address, ip_start + 1, ip_length);
                ip_address[ip_length] = '\0'; // Null-terminate the string

                printf("Parsed data - Username: %s, PTS: %s, IP: %s\n", username, pts, ip_address);

                // Check if this is your own session or a different SSH connection
                if (strcmp(pts, get_own_pts()) != 0) {
                    strcpy(connections[*count].username, username);
                    strcpy(connections[*count].ip_address, ip_address);
                    strcpy(connections[*count].pts, pts); // Add the pts to the struct for reference
                    (*count)++;
                } else {
                    printf("Ignoring own session: %s on pts/%s\n", username, pts);
                }
            } else {
                printf("Failed to find IP in line: %s", buffer); // Debug
            }
        } else {
            printf("Failed to parse line correctly: %s", buffer); // Debug
        }
    }

    pclose(fp);

    if (*count == 0) {
        free(connections);
        connections = NULL;
    }

    return connections;
}

// Function to kick a single connection
void kick_connection(const char* pts) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "pkill -9 -t %s", pts);
    int result = system(cmd);
    if (result == -1) {
        printf("Failed to execute kick command\n");
    } else {
        printf("Connection on %s has been kicked\n", pts);
    }
}

// Kick button functionality
void on_kick_button_clicked(GtkWidget *widget, gpointer data) {
    SSHConnection *connections = (SSHConnection *)data;
    int *count_ptr = (int *)((char *)data + sizeof(SSHConnection) * 10);
    int count = *count_ptr;
    
    if (count == 1) {
        // Kick the only connection
        kick_connection(connections[0].pts);
        gtk_main_quit();
    } else {
        // Prompt the user to select which connection to kick
        GtkWidget *dialog, *content_area, *entry, *label;
        dialog = gtk_dialog_new_with_buttons("Kick SSH User", NULL, GTK_DIALOG_MODAL, "_Kick", GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);
        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        label = gtk_label_new("Enter SSH # to kick:");
        entry = gtk_entry_new();

        gtk_container_add(GTK_CONTAINER(content_area), label);
        gtk_container_add(GTK_CONTAINER(content_area), entry);
        gtk_widget_show_all(dialog);

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_OK) {
            int ssh_num = atoi(gtk_entry_get_text(GTK_ENTRY(entry))) - 1;
            if (ssh_num >= 0 && ssh_num < count) {
                kick_connection(connections[ssh_num].pts);
            }
        }
        gtk_widget_destroy(dialog);
    }
}

// Function to kick all connections except the user's own `tty`
void on_kick_all_button_clicked(GtkWidget *widget, gpointer data) {
    SSHConnection *connections = (SSHConnection *)data;
    int *count_ptr = (int *)((char *)data + sizeof(SSHConnection) * 10);
    int count = *count_ptr;

    for (int i = 0; i < count; ++i) {
        kick_connection(connections[i].pts);
    }

    gtk_main_quit();
}

void on_okay_button_clicked(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

// Function to display the GTK window
void display_gtk_window(SSHConnection *connections, int count) {
    gtk_init(NULL, NULL);

    GtkWidget *window, *vbox, *label, *button_kick, *button_okay, *button_kick_all = NULL;
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "SSH Connection Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    char label_text[1024] = "Active SSH Connections:\n";
    for (int i = 0; i < count; ++i) {
        char line[256];
        snprintf(line, sizeof(line), "%d. User: %s | IP: %s | PTS: %s\n", i + 1, connections[i].username, connections[i].ip_address, connections[i].pts);
        strcat(label_text, line);
    }

    label = gtk_label_new(label_text);
    gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 5);

    button_kick = gtk_button_new_with_label("Kick");

    SSHConnection* data = malloc(sizeof(SSHConnection) * 10 + sizeof(int)); // Allocate memory for connections + count
    memcpy(data, connections, sizeof(SSHConnection) * count);
    memcpy((char *)data + sizeof(SSHConnection) * 10, &count, sizeof(int));

    g_signal_connect(button_kick, "clicked", G_CALLBACK(on_kick_button_clicked), data);
    gtk_box_pack_start(GTK_BOX(vbox), button_kick, TRUE, TRUE, 5);

    if (count > 1) {
        button_kick_all = gtk_button_new_with_label("Kick All");
        g_signal_connect(button_kick_all, "clicked", G_CALLBACK(on_kick_all_button_clicked), data);
        gtk_box_pack_start(GTK_BOX(vbox), button_kick_all, TRUE, TRUE, 5);
    }

    button_okay = gtk_button_new_with_label("Okay");
    g_signal_connect(button_okay, "clicked", G_CALLBACK(on_okay_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), button_okay, TRUE, TRUE, 5);

    gtk_widget_show_all(window);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_main();

    free(data);
}

// The main function to initiate the program
int main() {
    int count = 0;
    SSHConnection* connections = get_ssh_connections(&count);

    if (count == 0) {
        printf("No active SSH connections.\n");
        free(connections);
        return 0;
    }

    display_gtk_window(connections, count);
    free(connections);
    return 0;
}

