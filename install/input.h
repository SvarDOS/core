/*
 * input keyboard routines used by the Svarog386 installer.
 * Copyright (C) 2016 Mateusz Viste
 */

/* waits for a keypress and return it. Returns 0 for extended keystroke, then
   function must be called again to return scan code. */
int input_getkey(void);

/* poll the keyboard, and return the next input key if any, or -1 */
int input_getkeyifany(void);
