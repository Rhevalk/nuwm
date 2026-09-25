#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <xcb/xcb.h>

#define PATH_IN    "/dev/shm/nuwm.in"
#define PATH_OUT   "/dev/shm/nuwm.out"

xcb_screen_t      *system_screen; 
xcb_connection_t  *system_connection; 

xcb_window_t       client_list[9];
xcb_window_t       focus_current = XCB_NONE; 
xcb_window_t       focus_previous = XCB_NONE; 

void print_clients() {
    char buf[12];
    char curr_ch = '0', prev_ch = '0';
    int len = 2;

    for (int i = 0; i < 9; i++) {
        xcb_window_t win = client_list[i];
        if (win == XCB_NONE) continue;

        char tag = '1' + i;
        if (win == focus_current)  curr_ch = tag;
        if (win == focus_previous) prev_ch = tag;

        buf[len++] = tag; 
    }

    buf[0] = curr_ch;
    buf[1] = prev_ch;

    int fd = open(PATH_OUT, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd >= 0) {
        write(fd, buf, len);
        close(fd);
    }
}

void action_spawn(const char *cmd) {
    if (fork() == 0) {
        close(xcb_get_file_descriptor(system_connection));
        setsid();
        execl("/bin/bash", "bash", "-i", "-c", cmd, NULL);
        exit(0);
    }
}

void action_swap(char **s) {
    int a = *(*s)++ - '1';
    int b = *(*s)++ - '1';

    xcb_window_t tmp = client_list[a];
    client_list[a] = client_list[b];
    client_list[b] = tmp;
}

void action_focus_client(xcb_window_t win) {
    if (win != focus_current) 
        focus_previous = focus_current; 

    focus_current = win;

    xcb_map_window(system_connection, win);
    xcb_set_input_focus(system_connection, XCB_INPUT_FOCUS_PARENT, win, XCB_CURRENT_TIME);
    xcb_configure_window(system_connection, win, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){ XCB_STACK_MODE_ABOVE });
}

void action_untrack(xcb_window_t win) {
    if (focus_previous == win) 
        focus_previous = XCB_NONE;

    if (focus_current == win) {
        focus_current = focus_previous;
        focus_previous = XCB_NONE;

        xcb_window_t next = (focus_current != XCB_NONE) ? focus_current : system_screen->root;
        xcb_set_input_focus(system_connection, XCB_INPUT_FOCUS_PARENT, next, XCB_CURRENT_TIME);
    }
}

void action_geometry_client(xcb_window_t win, char **s) {
    uint16_t mask = 0;
    uint32_t val_map[4] = {0}; 
    while (**s) {
        while (**s == ' ') (*s)++;

        int bit_idx = -1;
        switch (**s) {
            case 'x': bit_idx = 0; break;
            case 'y': bit_idx = 1; break;
            case 'w': bit_idx = 2; break;
            case 'h': bit_idx = 3; break;
            default: goto done;
        }

        mask |= (1 << bit_idx);
        (*s)++; 
        val_map[bit_idx] = (uint32_t)strtol(*s, s, 10);
    }

done:
    if (mask) {
        uint32_t val[4];
        int idx = 0;

        for (int i = 0; i < 4; i++) 
            if (mask & (1 << i))
                val[idx++] = val_map[i];

        xcb_configure_window(system_connection, win, mask, val);
    }
}



void event_map_request(xcb_generic_event_t *ev) {
    xcb_map_request_event_t *e = (xcb_map_request_event_t *)ev;

    for (int i = 0; i < 9; i++) {
        if (client_list[i] == XCB_NONE) {
            client_list[i] = e->window;
            xcb_change_window_attributes(system_connection, e->window, XCB_CW_EVENT_MASK, (uint32_t[]){ XCB_EVENT_MASK_STRUCTURE_NOTIFY });
            action_focus_client(e->window); 
            break;
        }
    }
}

void event_unmap_notify(xcb_generic_event_t *ev) {
    action_untrack(((xcb_unmap_notify_event_t *)ev)->window);
}

void event_destroy_notify(xcb_generic_event_t *ev) { 
    xcb_window_t win = ((xcb_destroy_notify_event_t *)ev)->window;

    for (int i = 0; i < 9; i++) 
        if (client_list[i] == win) 
            client_list[i] = XCB_NONE;

    action_untrack(win);
}

void event_configure_request(xcb_generic_event_t *ev) {
    xcb_configure_request_event_t *e = (xcb_configure_request_event_t *)ev;
    uint32_t vals[7]; int i = 0;

    if (e->value_mask & XCB_CONFIG_WINDOW_X)            vals[i++] = e->x;
    if (e->value_mask & XCB_CONFIG_WINDOW_Y)            vals[i++] = e->y;
    if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)        vals[i++] = e->width;
    if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)       vals[i++] = e->height;
    if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) vals[i++] = e->border_width;
    if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING)      vals[i++] = e->sibling;
    if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)   vals[i++] = e->stack_mode;

    xcb_configure_window(system_connection, e->window, e->value_mask, vals);
}

void event_key_press(xcb_generic_event_t *ev) {
    xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;

    if (e->detail == 133) {
        action_spawn("sh ~/.nuwmrc");
        print_clients();
    }
}

void handle_fifo(int fd) {
    char buf[64] = {0};
    if (read(fd, buf, sizeof(buf) - 1) <= 0) return;

    char *s = buf, *targets, *args, *p;
    char op;

    while (*s) {
        if (*s <= ' ') { s++; continue; }
        op = *s++;

        switch (op) {
            case 'q': exit(0); 
            case ':': action_spawn(s); return; 
            case 's': action_swap(&s); continue; 
        }

        targets = s;
        while (*s >= '0' && *s <= '9') s++;
        args = s;

        do {
            xcb_window_t win = focus_current;
        
            if (*targets >= '1' && *targets <= '9')
                win = client_list[*targets - '1'];
            else if (*targets == '0')
                win = focus_previous;
        
            p = args; 
            if (win != XCB_NONE) {
                switch (op) {
                    case 'f': action_focus_client(win); break;
                    case 'g': action_geometry_client(win, &p); break; 
                    case 'k': xcb_kill_client(system_connection, win); break;
                    case 'm': xcb_unmap_window(system_connection, win); break;
                }
            }
        
            targets++;
        } while (targets < args);
        s = p; 
    }
}

void handle_xevent() {
    xcb_generic_event_t *ev;
    while ((ev = xcb_poll_for_event(system_connection))) {
        switch (ev->response_type & ~0x80) {
            case XCB_KEY_PRESS:          event_key_press(ev);          break;
            case XCB_MAP_REQUEST:        event_map_request(ev);        break;
            case XCB_UNMAP_NOTIFY:       event_unmap_notify(ev);       break;
            case XCB_DESTROY_NOTIFY:     event_destroy_notify(ev);     break;
            case XCB_CONFIGURE_REQUEST:  event_configure_request(ev);  break;
        }
        free(ev);
    }
}

int main() {
    signal(SIGCHLD, SIG_IGN);

    system_connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(system_connection)) return 1;

    system_screen = xcb_setup_roots_iterator(xcb_get_setup(system_connection)).data;
    if (!system_screen) return 1;

    if (xcb_request_check(system_connection, xcb_change_window_attributes_checked(system_connection, system_screen->root, 
        XCB_CW_EVENT_MASK, (uint32_t[]){ XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY }))) return 1;

    xcb_set_input_focus(system_connection, XCB_INPUT_FOCUS_POINTER_ROOT, system_screen->root, XCB_CURRENT_TIME);
    xcb_grab_key(system_connection, 1, system_screen->root, XCB_MOD_MASK_ANY, 133, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);    
    xcb_flush(system_connection);

    mkfifo(PATH_IN, 0600);
    int fifo_in_fd = open(PATH_IN, O_RDWR | O_NONBLOCK);
    if (fifo_in_fd < 0) return 1;

    struct pollfd fds[2] = {
        { .fd = xcb_get_file_descriptor(system_connection), .events = POLLIN },
        { .fd = fifo_in_fd,                                 .events = POLLIN }
    };

    while (poll(fds, 2, -1) > 0) {
        if (fds[0].revents & POLLIN) handle_xevent();
        if (fds[1].revents & POLLIN) handle_fifo(fds[1].fd);
        xcb_flush(system_connection);
    }

    return 0;
}
