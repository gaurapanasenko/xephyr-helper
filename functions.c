#include "stdafx.h"
#include "xclib.h"

//~ typedef struct SeletionAgrs {
	//~ char * display_adr;
	//~ unsigned char ** data;
	//~ unsigned char
	//~ unsigned long * n;
	//~ char ** ATOMS;
	//~ int ATOMS_LEN;
//~ } SeletionAgrs;

int get_format (int size) {
	if (size == sizeof(char )) return 8;
	if (size == sizeof(short)) return 16;
	if (size == sizeof(long )) return 32;
	return 0;
}

void debug (const char * format, ...) {
	if (DEBUG) {
		va_list arg;
		va_start (arg, format);
		char * new_format = malloc((strlen(format) + 16) * sizeof(char));
		sprintf(new_format, "DEBUG: %s\n", format);
		vfprintf(stdout, new_format, arg);
		free(new_format);
		va_end (arg);
	}
}

void error (const char * message) {
	printf("%s\n", message);
	exit(1);
}

int error_handler(Display * display, XErrorEvent * error) {
	char buffer1[1024];
	XGetErrorText(display, error->error_code, buffer1, 1024);
	fprintf(stderr,
		"_X Error of failed request:  %s\n"
		"_  Major opcode of failed request: % 3d\n"
		"_  Serial number of failed request:% 5ld\n",
		buffer1, error->request_code, error->serial);
	return 0;
}

int io_error_handler(Display * display) {
	printf("X11 IO error");
	return 0;
}

int get_property(Display * d, Window window_id, Atom atom, Atom type,
		int format, unsigned long * n, unsigned char ** data) {
	Atom got_type;
	int got_format;
	unsigned long after;
	if (Success != XGetWindowProperty(d, window_id, atom,
			0, 0, 0, AnyPropertyType, &got_type,
			&got_format, n, &after, data) || got_type != type
			|| got_format != format || n == 0 || after == 0)
		return 0;
	XFree(*data);
	if (Success == XGetWindowProperty(d, window_id, atom,
			0, after + 1, 0, AnyPropertyType, &got_type,
			&got_format, n, &after, data)
			&& got_type == type && n != 0 && after == 0)
		return 1;
	XFree(*data);
	return 0;
}

//~ int get_selection(Display * display, Window root, Atom * atoms,
		//~ unsigned long * n, unsigned char ** data) {
	//~ XEvent event;
	//~ Window window = XCreateSimpleWindow(display,root,0,0,1,1,0,0,0);
	//~ XConvertSelection(display, atoms[3], atoms[6], atoms[4], window,
		//~ CurrentTime);
	//~ do {
		//~ XNextEvent(display, &event);
	//~ } while (event.type != SelectionNotify
		//~ || event.xselection.selection != atoms[3]);
	//~ if (event.xselection.property) {
		//~ return get_property(display,window,atoms[4],atoms[6],n,data);
	//~ } else return 1;
	//~ XDestroyWindow(display, window);
//~ }

//~ int set_selection(Display * display, Window root, Atom * atoms,
		//~ unsigned char * data, int n) {
	//~ XEvent e, s;
	//~ Atom types[2] = { atoms[4], XA_STRING };

	//~ Window window = XCreateSimpleWindow(display,root,0,0,1,1,0,0,0);
	//~ XSetSelectionOwner(display, atoms[3], window, CurrentTime);
	//~ XSync(display, 0);
	//~ while (1) {
		//~ XNextEvent(display, &e);
		//~ Window owner     = e.xselectionrequest.owner;
		//~ Atom selection   = e.xselectionrequest.selection;
		//~ Atom target      = e.xselectionrequest.target;
		//~ Atom property    = e.xselectionrequest.property;
		//~ Window requestor = e.xselectionrequest.requestor;
		//~ Time timestamp   = e.xselectionrequest.time;
		//~ Display * dispay = e.xselection.display;
		//~ s.xselection.type = SelectionNotify;
		//s.xselection.serial     - filled in by server
		//s.xselection.send_event - filled in by server
		//s.xselection.display    - filled in by server
		//~ s.xselection.requestor = e.xselectionrequest.requestor;
		//~ s.xselection.selection = e.xselectionrequest.selection;
		//~ s.xselection.target    = e.xselectionrequest.target;
		//~ s.xselection.property = e.xselectionrequest.property;
		//~ s.xselection.time = e.xselectionrequest.time;
		//~ XChangeProperty(display, window, atoms[4], XA_STRING, 8,
				//~ PropModeReplace, data, n);
		//~ if(target == atoms[4]) {
			//~ XChangeProperty(display, window, property, XA_ATOM, 32,
				//~ PropModeReplace, (unsigned char *) types,
				//~ (sizeof(types) / sizeof(Atom)));
		//~ } else if (target == XA_STRING) {
			//~ XChangeProperty(display, requestor, property, target, 8,
					//~ PropModeReplace, data, n);
		//~ } else {
			//~ s.xselection.property = 0;
		//~ }
		//~ XSendEvent(display, e.xselectionrequest.requestor, True, 0, &s);
	//~ }
	//~ XDestroyWindow(display, window);
	//~ XFlush(display);
	//~ return 1;
//~ }

//~ void set_selection(void * args) {
	//~ SeletionAgrs * arguments = (SeletionAgrs *) args;
	//~ Display * display = XOpenDisplay(arguments->display_adr);
	//~ unsigned char ** data = arguments->data;
	//~ int n = *arguments->n;
	//~ if (display == NULL) return 0;
	//~ Window root = XDefaultRootWindow(display);
	//~ Window window = XCreateSimpleWindow(display,root,0,0,1,1,0,0,0);
	//~ Atom * atoms = calloc(ATOMS_LEN, sizeof(Atom));
	//~ if (!XInternAtoms(display, ATOMS, ATOMS_LEN, False, atoms)){
		//~ XCloseDisplay(display);
		//~ return 0;
	//~ }
	//~ XSetSelectionOwner(display, atoms[3], window, CurrentTime);
	//~ unsigned int clear = 0;
	//~ unsigned int context = XCLIB_XCIN_NONE;
	//~ unsigned long sel_pos = 0;
	//~ Window cwin;
	//~ Atom pty;
	//~ XEvent evt;
	//~ while (1) {
		//~ int finished;

		//~ XNextEvent(display, &evt);

		//~ finished = xcin(display, &cwin, evt, &pty, XA_STRING, data, n, &sel_pos, &context);

		//~ if (evt.type == SelectionClear)
		//~ clear = 1;

		//~ if ((context == XCLIB_XCIN_NONE) && clear)
		//~ return 1;

		//~ if (finished)
		//~ break;
	//~ }
	//~ XDestroyWindow(display, window);
	//~ XFlush(display);
	//~ return 1;
//~ }

int search_display_in_name(char * name, char * result_name) {
	char * n = strstr(name, " on :");
	if (n == NULL) return 0;
	char * i = strstr(n + 5, " on :"), *m, *e;
	while (i != NULL) {n = i; i = strstr(i + 5, " on :"); }
	if (!isdigit(*(n+5))) return 0;
	n += 4; e = n + 1;
	while (isdigit(*(e))) e++;
	m = result_name; *m = *n;
	while (n < e) *m++ = *n++;
	*m = 0;
	return 1;
}

void simemcheck(void *ptr) {
	if (ptr == NULL) error("Error: Could not allocate memory.\n");
}

void* sicalloc (size_t num, size_t size) {
	void* out = calloc(num, size);
	simemcheck(out);
	return out;
}

void* simalloc (size_t size) {
	void* out = malloc(size);
	simemcheck(out);
	return out;
}

void* sirealloc(void* ptr, size_t size) {
	void* out = realloc(ptr, size);
	simemcheck(out);
	return out;
}
