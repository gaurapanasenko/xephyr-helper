#include "stdafx.h"

int get_format (int size);

void debug (const char * format, ...);

void error (const char * message);

int error_handler(Display * display, XErrorEvent * error);

int io_error_handler(Display * display);

int get_property(Display * d, Window window_id, Atom atom, Atom type,
		int format, unsigned long * n, unsigned char ** data);

//~ int get_selection(Display * display, Window root, Atom * atoms,
		//~ unsigned long * n, unsigned char ** data);

//~ int set_selection(Display * display, Window root, Atom * atoms,
		//~ unsigned char * data, int n);

void set_selection(void * args);

int search_display_in_name(char * name, char * result_name);

void simemcheck(void *ptr);

void* sicalloc (size_t num, size_t size);

void* simalloc (size_t size);

void* sirealloc (void* ptr, size_t size);
