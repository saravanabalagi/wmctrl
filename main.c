
/* license {{{ */
/* 

wmctrl
A command line tool to interact with an EWMH/NetWM compatible X Window Manager.

Author, current maintainer: Tomas Styblo <tripie@cpan.org>

Copyright (C) 2003

This program is free software which I release under the GNU General Public
License. You may redistribute and/or modify this program under the terms
of that license as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

To get a copy of the GNU General Puplic License,  write to the
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
/* }}} */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <glib.h>

/* help {{{ */
#define HELP "wmctrl " VERSION "\n" \
"Usage: wmctrl [OPTION]...\n" \
"Actions:\n" \
"  -m                   Show information about the window manager.\n" \
"  -l                   List windows managed by the window manager.\n" \
"  -d                   List desktops. The current desktop is marked\n" \
"                       with an asterisk.\n" \
"  -s <DESK>            Switch to the specified desktop.\n" \
"  -a <WIN>             Activate window by switching to its desktop and\n" \
"                       raising it.\n" \
"  -c <WIN>             Close window gracefully.\n" \
"  -r <WIN>             Move window to the current desktop and activate it.\n" \
"  -r <WIN> -t <DESK>   Move window to the specified desktop.\n" \
"  -h                   Print help.\n" \
"\n" \
"Options:\n" \
"  -i                   Interpret <WIN> as a numerical window ID.\n" \
"  -p                   Include PIDs in the window list. Very few\n" \
"                       X applications support this feature.\n" \
"  -u                   Override auto-detection and force UTF-8 mode.\n" \
"  -v                   Be verbose. Useful for debugging.\n" \
"\n" \
"Arguments:\n" \
"  <WIN>                This argument specifies a window. By default it's\n" \
"                       interpreted as a string. The string is matched\n" \
"                       against window titles and the first matching\n" \
"                       window is used. The matching isn't case sensitive\n" \
"                       and the string may appear in any position\n" \
"                       of the title.\n" \
"\n" \
"                       The -t option may be used to interpret the argument\n" \
"                       as a numerical window ID represented as a decimal\n" \
"                       number. If it starts with \"0x\", then\n" \
"                       it will be interpreted as a hexadecimal number.\n" \
"\n" \
"  <DESK>               A desktop number. Desktops are counted from zero.\n" \
"\n" \
"Format of the window list:\n" \
"(\"<WS>\" stands for any number of whitespace characters):\n" \
"\n" \
"  <window ID> <WS> <desktop ID> <WS> <client machine> <SPACE> <window title>\n" \
"\n" \
"Format of the desktop list:\n" \
"\n" \
"  <desktop ID> <WS> [-*] <SPACE> <desktop title>\n" \
"\n" \
"\n" \
"Author, current maintainer: Tomas Styblo <tripie@cpan.org>\n" \
"Released under the GNU General Public License.\n" \
"Copyright (C) 2003\n"
/* }}} */

#define MAX_PROPERTY_VALUE_LEN 4096

#define p_verbose(...) if (options.verbose) { \
    fprintf(stderr, __VA_ARGS__); \
}

/* declarations of static functions *//*{{{*/
static Window *get_client_list (Display *disp, unsigned long *size);
static int list_windows (Display *disp);
static int list_desktops (Display *disp);
static int switch_desktop (Display *disp);
static int wm_info (Display *disp);
static gchar *get_output_str (gchar *str, gboolean is_utf8);
static int activate_window_pid (Display *disp, char mode);
static int activate_window_str (Display *disp, char mode);
static int activate_window (Display *disp, Window win);
static int close_window (Display *disp, Window win);
static int window_to_desktop (Display *disp, Window win, int desktop);
static gchar *get_window_title (Display *disp, Window win);
static gchar *get_property (Display *disp, Window win, 
        Atom xa_prop_type, gchar *prop_name, unsigned long *size);
static void init_charset();
/*}}}*/
   
static struct {
    int verbose;
    int force_utf8;
    int show_pid;
    int match_by_id;
    char *param_window;
    char *param_desktop;
} options = {
    0, 0, 0, 0, NULL, NULL
};

static gboolean envir_utf8;

int main (int argc, char **argv) { /* {{{ */
    int opt;
    int action = 0;
    int ret = EXIT_SUCCESS;
    int missing_option = 1;
    Display *disp;

    /* make "--help" work. I don't want to use
     * getopt_long for portability reasons */
    if (argc == 2 && argv[1] && strcmp(argv[1], "--help") == 0) {
        fputs(HELP, stdout);
        return EXIT_SUCCESS;
    }
   
    while ((opt = getopt(argc, argv, "Vvhlupidma:r:s:c:t:")) != -1) {
        missing_option = 0;
        switch (opt) {
            case 'i':
                options.match_by_id = 1;
                break;
            case 'v':
                options.verbose = 1;
                break;
            case 'u':
                options.force_utf8 = 1;
                break;
            case 'p':
                options.show_pid = 1;
                break;
            case 'a': case 'r': case 'c':
                options.param_window = optarg;
                action = opt;
                break;
            case 't':
                options.param_desktop = optarg;
                break;
            case 's':
                options.param_desktop = optarg;
                action = opt;
                break;
            case '?':
                fputs(HELP, stderr);
                return EXIT_FAILURE;
            default:
                action = opt;
        }
    }
   
    if (missing_option) {
        fputs(HELP, stderr);
        return EXIT_FAILURE;
    }
   
    init_charset();
    
    if (! (disp = XOpenDisplay(NULL))) {
        fputs("Cannot open display.\n", stderr);
        return EXIT_FAILURE;
    }
   
    switch (action) {
        case 'V':
            puts(VERSION);
            break;
        case 'h':
            fputs(HELP, stdout);
            break;
        case 'l':
            ret = list_windows(disp);
            break;
        case 'd':
            ret = list_desktops(disp);
            break;
        case 's':
            ret = switch_desktop(disp);
            break;
        case 'm':
            ret = wm_info(disp);
            break;
        case 'a': case 'r': case 'c':
            if (options.match_by_id) {
                ret = activate_window_pid(disp, action);
            }
            else {
                ret = activate_window_str(disp, action);
            }
            break;
    }
    
    XCloseDisplay(disp);
    return ret;
}
/* }}} */

static void init_charset () {/*{{{*/
    const gchar *charset; /* unused */
    gchar *lang = g_ascii_strup(getenv("LANG"), -1);
    gchar *lc_ctype = g_ascii_strup(getenv("LC_CTYPE"), -1);
    
    /* this glib function doesn't work on my system ... */
    envir_utf8 = g_get_charset(&charset);

    /* ... therefore we will examine the environment variables */
    if (lc_ctype && (strstr(lc_ctype, "UTF8") || strstr(lc_ctype, "UTF-8"))) {
        envir_utf8 = TRUE;
    }
    else if (lang && (strstr(lang, "UTF8") || strstr(lang, "UTF-8"))) {
        envir_utf8 = TRUE;
    }

    g_free(lang);
    g_free(lc_ctype);
    
    if (options.force_utf8) {
        envir_utf8 = TRUE;
    }
    p_verbose("envir_utf8: %d\n", envir_utf8);
}/*}}}*/

static int client_msg(Display *disp, Window win, char *msg, unsigned long data) {/*{{{*/
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data;
    
    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Cannot send %s event.\n", msg);
        return EXIT_FAILURE;
    }
}/*}}}*/

static gchar *get_output_str (gchar *str, gboolean is_utf8) {/*{{{*/
    gchar *out;
   
    if (str == NULL) {
        return NULL;
    }
    
    if (envir_utf8) {
        if (is_utf8) {
            out = g_strdup(str);
        }
        else {
            if (! (out = g_locale_to_utf8(str, -1, NULL, NULL, NULL))) {
                p_verbose("Cannot convert string from locale charset to UTF-8.\n");
                out = g_strdup(str);
            }
        }
    }
    else {
        if (is_utf8) {
            if (! (out = g_locale_from_utf8(str, -1, NULL, NULL, NULL))) {
                p_verbose("Cannot convert string from UTF-8 to locale charset.\n");
                out = g_strdup(str);
            }
        }
        else {
            out = g_strdup(str);
        }
    }

    return out;
}/*}}}*/

static int wm_info (Display *disp) {/*{{{*/
    Window *sup_window = NULL;
    gchar *wm_name = NULL;
    unsigned long *wm_pid = NULL;
    gboolean name_is_utf8 = TRUE;
    gchar *name_out;
    
    if (! (sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                    XA_WINDOW, "_NET_SUPPORTING_WM_CHECK", NULL))) {
        if (! (sup_window = (Window *)get_property(disp, DefaultRootWindow(disp),
                        XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK", NULL))) {
            fputs("Cannot get window manager info properties.\n"
                  "(_NET_SUPPORTING_WM_CHECK or _WIN_SUPPORTING_WM_CHECK)\n", stderr);
            return EXIT_FAILURE;
        }
    }

    if (! (wm_name = get_property(disp, *sup_window,
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL))) {
        name_is_utf8 = FALSE;
        if (! (wm_name = get_property(disp, *sup_window,
                XA_STRING, "_NET_WM_NAME", NULL))) {
            p_verbose("Cannot get name of the window manager (_NET_WM_NAME).\n");
        }
    }
    name_out = get_output_str(wm_name, name_is_utf8);
  
    if (! (wm_pid = (unsigned long *)get_property(disp, *sup_window,
                    XA_CARDINAL, "_NET_WM_PID", NULL))) {
        p_verbose("Cannot get pid of the window manager (_NET_WM_PID).\n");
    }
    
    printf("Name: %s\n", name_out ? name_out : "N/A");
    if (wm_pid) {
        printf("PID: %lu\n", *wm_pid);
    }
    else {
        printf("PID: N/A\n");
    }
  
    g_free(name_out);
    g_free(sup_window);
    g_free(wm_name);
    g_free(wm_pid);
    
    return EXIT_SUCCESS;
}/*}}}*/

static int switch_desktop (Display *disp) {/*{{{*/
    int target = -1;
    
    target = atoi(options.param_desktop); 
    if (target == -1) {
        fputs("Invalid desktop ID.\n", stderr);
        return EXIT_FAILURE;
    }
    
    return client_msg(disp, DefaultRootWindow(disp), "_NET_CURRENT_DESKTOP", 
            (unsigned long)target);
}/*}}}*/

static int window_to_desktop (Display *disp, Window win, int desktop) {/*{{{*/
    unsigned long *cur_desktop = NULL;
    Window root = DefaultRootWindow(disp);
   
    if (desktop == -1) {
        if (! (cur_desktop = (unsigned long *)get_property(disp, root,
                XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
            if (! (cur_desktop = (unsigned long *)get_property(disp, root,
                    XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
                fputs("Cannot get current desktop properties. "
                      "(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
                      "\n", stderr);
                return EXIT_FAILURE;
            }
        }
        desktop = *cur_desktop;
    }
    g_free(cur_desktop);

    return client_msg(disp, win, "_NET_WM_DESKTOP", (unsigned long)desktop);
}/*}}}*/

static int activate_window (Display *disp, Window win) {/*{{{*/
    unsigned long *desktop;

    /* desktop ID */
    if ((desktop = (unsigned long *)get_property(disp, win,
            XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
        desktop = (unsigned long *)get_property(disp, win,
                XA_CARDINAL, "_WIN_WORKSPACE", NULL);
    }

    if (desktop) {
        if (client_msg(disp, DefaultRootWindow(disp), 
                    "_NET_CURRENT_DESKTOP", *desktop) != EXIT_SUCCESS) {
            p_verbose("Cannot switch desktop.\n");
        }
        g_free(desktop);
    }
    else {
        p_verbose("Cannot find desktop ID of the window.\n");
    }

    return client_msg(disp, win, "_NET_ACTIVE_WINDOW", 0);
}/*}}}*/

static int close_window (Display *disp, Window win) {/*{{{*/
    return client_msg(disp, win, "_NET_CLOSE_WINDOW", 0);
}/*}}}*/

static int activate_window_pid (Display *disp, char mode) {/*{{{*/
    unsigned long wid;

    if (sscanf(options.param_window, "0x%lx", &wid) != 1 &&
            sscanf(options.param_window, "0X%lx", &wid) != 1 &&
            sscanf(options.param_window, "%lu", &wid) != 1) {
        fputs("Cannot convert argument to number.\n", stderr);
        return EXIT_FAILURE;
    }

    p_verbose("Activating window: %lu\n", wid);
    if (mode == 'a') {
        return activate_window(disp, (Window)wid);
    }
    else if (mode == 'c') {
        return close_window(disp, (Window)wid);
    }
    else {
        if (options.param_desktop == NULL) {
            if (window_to_desktop(disp, (Window)wid, -1) == EXIT_SUCCESS) {
                return activate_window(disp, (Window)wid);
            }
            else {
                return EXIT_FAILURE;
            }
        }
        else {
            return window_to_desktop(disp, (Window)wid, 
                    atoi(options.param_desktop));
        }
    }
}/*}}}*/

static int activate_window_str (Display *disp, char mode) {/*{{{*/
    Window activate = 0;
    Window *client_list;
    unsigned long client_list_size;
    int i;
    
    if ((client_list = get_client_list(disp, &client_list_size)) == NULL) {
        return EXIT_FAILURE; 
    }
    
    for (i = 0; i < client_list_size / 4; i++) {
        gchar *title_utf8 = get_window_title(disp, client_list[i]); /* UTF8 */
        gchar *title_utf8_cf = NULL;
        if (title_utf8) {
            gchar *match;
            if (envir_utf8) {
                match = g_utf8_casefold(options.param_window, -1);
            }
            else {
                gchar *tmp;
                if (! (tmp = g_locale_to_utf8(options.param_window, -1, NULL, NULL, NULL))) {
                    tmp = g_strdup(options.param_window);
                }
                match = g_utf8_casefold(tmp, -1);
                g_free(tmp);
            }
            
            if (! match) {
                continue;
            }

            title_utf8_cf = g_utf8_casefold(title_utf8, -1);

            if (strstr(title_utf8_cf, match)) {
                activate = client_list[i];
                g_free(match);
                g_free(title_utf8);
                g_free(title_utf8_cf);
                break;
            }
            g_free(match);
            g_free(title_utf8);
            g_free(title_utf8_cf);
        }
    }
    g_free(client_list);

    if (activate) {
        p_verbose("Activating window: %lu\n", activate);
        if (mode == 'a') {
            return activate_window(disp, activate);
        }
        else if (mode == 'c') {
            return close_window(disp, activate);
        }
        else {
            if (options.param_desktop == NULL) {
                if (window_to_desktop(disp, activate, -1) == EXIT_SUCCESS) {
                    return activate_window(disp, activate);
                }
                else {
                    return EXIT_FAILURE;
                }
            }
            else {
                return window_to_desktop(disp, activate, 
                        atoi(options.param_desktop));
            }
        }
    }
    else {
        return EXIT_FAILURE;
    }
}/*}}}*/

static int list_desktops (Display *disp) {/*{{{*/
    unsigned long *num_desktops = NULL;
    unsigned long *cur_desktop = NULL;
    unsigned long desktop_list_size;
    gchar *list = NULL;
    int i;
    int id;
    Window root = DefaultRootWindow(disp);
    int ret = EXIT_FAILURE;
    gchar **names = NULL;
    gboolean names_are_utf8 = TRUE;
    
    if (! (num_desktops = (unsigned long *)get_property(disp, root,
            XA_CARDINAL, "_NET_NUMBER_OF_DESKTOPS", NULL))) {
        if (! (num_desktops = (unsigned long *)get_property(disp, root,
                XA_CARDINAL, "_WIN_WORKSPACE_COUNT", NULL))) {
            fputs("Cannot get number of desktops properties. "
                  "(_NET_NUMBER_OF_DESKTOPS or _WIN_WORKSPACE_COUNT)"
                  "\n", stderr);
            goto clear;
        }
    }
    
    if (! (cur_desktop = (unsigned long *)get_property(disp, root,
            XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
        if (! (cur_desktop = (unsigned long *)get_property(disp, root,
                XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
            fputs("Cannot get current desktop properties. "
                  "(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE property)"
                  "\n", stderr);
            goto clear;
        }
    }

    if ((list = get_property(disp, root, 
            XInternAtom(disp, "UTF8_STRING", False), 
            "_NET_DESKTOP_NAMES", &desktop_list_size)) == NULL) {
        names_are_utf8 = FALSE;
        if ((list = get_property(disp, root, 
            XA_STRING, 
            "_WIN_WORKSPACE_NAMES", &desktop_list_size)) == NULL) {
            p_verbose("Cannot get desktop names properties. "
                  "(_NET_DESKTOP_NAMES or _WIN_WORKSPACE_NAMES)"
                  "\n");
            /* ignore the error - list the desktops without names */
        }
    }
 
    /* prepare the array of desktop names */
    names = g_malloc0(*num_desktops * sizeof(char *));
    if (list) {
        id = 0;
        names[id++] = list;
        for (i = 0; i < desktop_list_size; i++) {
            if (list[i] == '\0') {
                if (id >= *num_desktops) {
                    p_verbose("??? More desktop names than num_desktops.\n");
                    break;
                }
                names[id++] = list + i + 1;
            }
        }
    }

    /* print the list */
    for (i = 0; i < *num_desktops; i++) {
        gchar *out = get_output_str(names[i], names_are_utf8);
        printf("%-2d %c %s\n", i, i == *cur_desktop ? '*' : '-', 
                out ? out : "N/A");
        g_free(out);
    }
    
    p_verbose("Total number of desktops: %lu\n", *num_desktops);
    p_verbose("Current desktop ID (counted from zero): %lu\n", *cur_desktop);
    
    ret = EXIT_SUCCESS;
    goto clear;
    
clear:
    g_free(names);
    g_free(num_desktops);
    g_free(cur_desktop);
    g_free(list);
    
    return ret;
}/*}}}*/

static Window *get_client_list (Display *disp, unsigned long *size) {/*{{{*/
    Window *client_list;

    if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp), 
                    XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL) {
        if ((client_list = (Window *)get_property(disp, DefaultRootWindow(disp), 
                        XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL) {
            fputs("Cannot get client list properties. \n"
                  "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)"
                  "\n", stderr);
            return NULL;
        }
    }

    return client_list;
}/*}}}*/

static int list_windows (Display *disp) {/*{{{*/
    Window *client_list;
    unsigned long client_list_size;
    int i;
    int max_client_machine_len = 0;
    
    if ((client_list = get_client_list(disp, &client_list_size)) == NULL) {
        return EXIT_FAILURE; 
    }
    
    /* find the longest client_machine name */
    for (i = 0; i < client_list_size / 4; i++) {
        gchar *client_machine;
        if ((client_machine = get_property(disp, client_list[i],
                XA_STRING, "WM_CLIENT_MACHINE", NULL))) {
            max_client_machine_len = strlen(client_machine);    
        }
        g_free(client_machine);
    }
    
    /* print the list */
    for (i = 0; i < client_list_size / 4; i++) {
        gchar *title_utf8 = get_window_title(disp, client_list[i]); /* UTF8 */
        gchar *title_out = get_output_str(title_utf8, TRUE);
        gchar *client_machine;
        unsigned long *pid;
        unsigned long *desktop;

        /* desktop ID */
        if ((desktop = (unsigned long *)get_property(disp, client_list[i],
                XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
            desktop = (unsigned long *)get_property(disp, client_list[i],
                    XA_CARDINAL, "_WIN_WORKSPACE", NULL);
        }

        /* client machine */
        client_machine = get_property(disp, client_list[i],
                XA_STRING, "WM_CLIENT_MACHINE", NULL);
       
        /* pid */
        pid = (unsigned long *)get_property(disp, client_list[i],
                XA_CARDINAL, "_NET_WM_PID", NULL);
       
        if (options.show_pid) {
            printf("0x%.8lx %-2lu %-6lu %*s %s\n", client_list[i], 
                    desktop ? *desktop : 0,
                    pid ? *pid : 0,
                    max_client_machine_len,
                    client_machine ? client_machine : "N/A",
                    title_out ? title_out : "N/A"
            );
        }
        else {
            printf("0x%.8lx %-2lu %*s %s\n", client_list[i], 
                    desktop ? *desktop : 0,
                    max_client_machine_len,
                    client_machine ? client_machine : "N/A",
                    title_out ? title_out : "N/A"
            );
        }
        g_free(title_utf8);
        g_free(title_out);
        g_free(desktop);
        g_free(client_machine);
        g_free(pid);
    }
    g_free(client_list);
   
    return EXIT_SUCCESS;
}/*}}}*/

static gchar *get_window_title (Display *disp, Window win) {/*{{{*/
    gchar *title_utf8;
    gchar *wm_name;
    gchar *net_wm_name;

    wm_name = get_property(disp, win, XA_STRING, "WM_NAME", NULL);
    net_wm_name = get_property(disp, win, 
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

    if (net_wm_name) {
        title_utf8 = g_strdup(net_wm_name);
    }
    else {
        if (wm_name) {
            title_utf8 = g_locale_to_utf8(wm_name, -1, NULL, NULL, NULL);
        }
        else {
            title_utf8 = NULL;
        }
    }

    g_free(wm_name);
    g_free(net_wm_name);
    
    return title_utf8;
}/*}}}*/

static gchar *get_property (Display *disp, Window win, /*{{{*/
        Atom xa_prop_type, gchar *prop_name, unsigned long *size) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    gchar *ret;
    
    xa_prop_name = XInternAtom(disp, prop_name, False);
    
    if (XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,     
            &ret_nitems, &ret_bytes_after, &ret_prop) != Success) {
        p_verbose("Cannot get %s property.\n", prop_name);
        return NULL;
    }
  
    if (xa_ret_type != xa_prop_type) {
        p_verbose("Invalid type of %s property.\n", prop_name);
        XFree(ret_prop);
        return NULL;
    }

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / 8) * ret_nitems;
    ret = g_malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }
    
    XFree(ret_prop);
    return ret;
}/*}}}*/
