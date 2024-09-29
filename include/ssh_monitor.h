// ssh_monitor.h

#ifndef SSH_MONITOR_H
#define SSH_MONITOR_H

#include <gtk/gtk.h>

// Define the SSHConnection struct with an additional field for 'pts'
typedef struct {
    char username[64];
    char ip_address[64];
    char pts[16]; // Add the pts field to store the terminal session
} SSHConnection;

SSHConnection* get_ssh_connections(int* count);
void display_gtk_window(SSHConnection* connections, int count);

#endif // SSH_MONITOR_H
