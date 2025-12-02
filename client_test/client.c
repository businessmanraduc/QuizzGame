#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <ncurses.h>

#include "includes/utils.h"
#include "includes/history.h"
#include "includes/tui.h"
#include "includes/network.h"


int main() {
    signal(SIGINT, handle_sigint);
    signal(SIGWINCH, handle_sigwinch);
    init_tui();
}