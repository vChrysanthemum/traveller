
    while(1) {
        rootWin->ch = getch();
        if ('0' <= rootWin->ch && rootWin->ch <= '9') {
            if (rootWin->command_mode.snumber_len > 6) 
                continue;

            rootWin->command_mode.snumber[rootWin->command_mode.snumber_len] = rootWin->ch;
            rootWin->command_mode.snumber_len++;
            rootWin->command_mode.snumber[rootWin->command_mode.snumber_len] = 0x00;

            continue;
        }
        else if(rootWin->command_mode.snumber_len > 0) {
            rootWin->command_mode.number = atoi(rootWin->command_mode.snumber);
            rootWin->command_mode.snumber_len = 0;
            rootWin->command_mode.snumber[0] = 0x00;
        }


        if (KEY_UP == rootWin->ch || 'k' == rootWin->ch) {
            moveCursorUp(rootWin->command_mode.number);
        }

        else if (KEY_DOWN == rootWin->ch || 'j' == rootWin->ch) {
            moveCursorDown(rootWin->command_mode.number);
        }

        else if (KEY_LEFT == rootWin->ch || 'h' == rootWin->ch) {
            moveCursorLeft(rootWin->command_mode.number);
        }

        else if (KEY_RIGHT == rootWin->ch || 'l' == rootWin->ch) {
            moveCursorRight(rootWin->command_mode.number);
        }

        rootWin->command_mode.number = 1;

        if (KEY_F(1) == rootWin->ch) break; /* ESC */ 
    }
