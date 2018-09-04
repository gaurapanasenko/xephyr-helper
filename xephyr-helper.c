#include "stdafx.h"
#include "functions.h"

typedef struct AppDisplay {
	Display * display;
	char display_adr[16];
	Atom * atoms;
	Window last_seen, root, window, xephyr_id;
	int focus_stay, thr, used;
} AppDisplay;


typedef struct AppDisplayList {
	AppDisplay ** list;
	int max_size, size;
} AppDisplayList;

int ad_is_alive(AppDisplay * ad) {
	char fname[32] = "/tmp/.X11-unix/X";
	strcat(fname, &ad->display_adr[1]);
	struct stat buffer;
	if (stat (fname, &buffer) == 0) return 1;
	else return 0;
}

int ad_close(AppDisplay * ad) {
	if (!ad->display) return 0;
	debug("Closing display %s", ad->display_adr);
	XCloseDisplay(ad->display);
	ad->display = NULL;
	return 1;
}

int ad_open(AppDisplay * ad, char ** ATOMS, int ATOMS_LEN) {
	if (ad->display) return 2;
	debug("Opening display %s", ad->display_adr);
	ad->display = XOpenDisplay(ad->display_adr);
	if (ad->display == NULL) return 0;
	if (!XInternAtoms(ad->display, ATOMS, ATOMS_LEN, False, ad->atoms)){
		ad_close(ad);
		return 0;
	}
	ad->root = XDefaultRootWindow(ad->display);
	return 1;
}

int ad_get_active_window(AppDisplay * ad, Window * window_id) {
	unsigned long n;
	unsigned char * data = 0;
	if (!get_property(ad->display, ad->root, ad->atoms[0], XA_WINDOW,
		get_format(sizeof(Window)), &n, &data)) return 0;
	*window_id = *(Window *)data;
	XFree(data);
	if (*window_id == 0) return 0;
	if (*window_id != ad->last_seen) ad->focus_stay = 0;
	else ad->focus_stay = 1;
	ad->last_seen = *window_id;
	return 1;
}

void ad_raise_first(AppDisplay * ad) {
	Window * data = 0, n;
	if (!get_property(ad->display, ad->root, ad->atoms[1], XA_WINDOW,
		get_format(sizeof(Window)), &n, (unsigned char **)&data)) n = 0;
	int revert_to = 0;
	if (n) {
		XSetInputFocus(ad->display, data[0], revert_to, CurrentTime);
	}
	XSync(ad->display, 0);
	XFree(data);
}

void start_wm(AppDisplay * ad) {
	char exec[64];
	if (!ad->thr) {
		sprintf(exec, "DISPLAY=%s blackbox &>/dev/null &",
			(char*) ad->display_adr);
		system(exec);
		usleep(250000);
	}
	ad_raise_first(ad);
	usleep(250000);
}

void adl_init(AppDisplayList * adl) {
	debug("Creating display list");
	adl->max_size = CHUNK;
	adl->list = calloc(adl->max_size, sizeof(AppDisplay*));
	adl->size = 0;
	if (adl->list == NULL) error("Couldn't create display list");
}

int adl_add(AppDisplayList * adl, const char * number,
		Window id, int * inx, int ATOMS_LEN) {
	debug("Checking display %s", number);
	int i;
	for (i = 0; i < adl->max_size; i++)
		if (adl->list[i] != NULL && adl->list[i]->xephyr_id == id) {
			*inx = i;
			debug("Already exists", number);
			return 2;
		}
	AppDisplay ** list, * ad;
	int new_max_size;
	while (adl->size + 1 >= adl->max_size) {
		new_max_size = adl->max_size + CHUNK;
		list = realloc(adl->list,(adl->max_size)*sizeof(AppDisplay*));
		if (list == NULL) return 0;
		for (i = adl->max_size; i < new_max_size; i++)
			adl->list[i] = NULL;
		adl->max_size = new_max_size;
		adl->list = list;
	}
	for (i = 0; i < adl->max_size; i++)
		if (adl->list[i] == NULL) {
			debug("Adding display %s", number);
			ad = calloc(1,sizeof(AppDisplay));
			if (ad == NULL) return 0;
			ad->atoms = calloc(ATOMS_LEN,sizeof(Atom));
			if (ad->atoms == NULL) {
				free(ad);
				return 0;
			}
			ad->display = NULL;
			strcpy(ad->display_adr,number);
			ad->focus_stay = 0;
			ad->last_seen = 0;
			ad->root = 0;
			ad->thr = 0;
			ad->used = 0;
			ad->window = 0;
			ad->xephyr_id = id;
			adl->list[i] = ad;
			adl->size++;
			*inx = i;
			return 1;
		}
	return 0;
}

void adl_del(AppDisplayList * adl, int id) {
	if (id < 0 || id >= adl->max_size) return;
	AppDisplay * d = adl->list[id];
	if (d == NULL) return;
	ad_close(adl->list[id]);
	debug("Deleting display %s", adl->list[id]->display_adr);
	free(d);
	adl->list[id] = NULL;
	adl->size--;
}

void adl_destruct(AppDisplayList * adl) {
	debug("Destructing display list");
	for (int i = 0; i < adl->max_size; i++)
		if (adl->list[i] != NULL)
			adl_del(adl, i);
	adl->max_size = 0;
	free(adl->list);
	adl->size = 0;
}

int update_data(AppDisplay * ad, Display * display, Atom * atoms) {
	Window window_id;
	int status = ad_get_active_window(ad, &window_id);
	if (status == 0) return status;
	debug("Updating data for %s", ad->display_adr);
	if (!ad->focus_stay) {
		unsigned long n = 0;
		unsigned char * icon = 0;
		debug("Updating icon for %s", ad->display_adr);
		if (get_property(ad->display, window_id, ad->atoms[2],
				XA_CARDINAL, 32, &n, &icon))
			XChangeProperty(display, ad->xephyr_id, atoms[2],
				XA_CARDINAL, 32, PropModeReplace, icon, (int) n);
		XFree(icon);
	}
	char * raw_name, * name;
	if (XFetchName(ad->display, window_id, &raw_name) > 0) {
		debug("Updating name for %s", ad->display_adr);
		name = (char *) calloc(strlen(raw_name) +
			strlen(ad->display_adr) + 5, sizeof(char));
		sprintf(name,"%s on %s", raw_name, ad->display_adr);
		XStoreName(display, ad->xephyr_id, name);
		XFree(raw_name);
		XFree(name);
	} else XFree(raw_name);
	XFlush(display);
	return 1;
}



int main() {
	AppDisplayList display_list;
	Display * display;
	Window root;
	Atom * atoms;
	XIEventMask mask, unmask;
	int status, i, inx, app = 0;
	char * name = 0, out_name[16];
	Window * data = 0, n = 0;
	AppDisplay * ad;
	int xi_opcode, query_event, query_error;
	char * ATOMS[] = {
		"_NET_ACTIVE_WINDOW",
		"_NET_CLIENT_LIST",
		"_NET_WM_ICON",
		"CLIPBOARD",
		"XSEL_DATA",
		"UTF8_STRING",
		"TARGETS"
	};
	int ATOMS_LEN = sizeof(ATOMS)/sizeof(*ATOMS);

	if (CHUNK <= 0) error("CHUNK can't be negative or zero");
	display = XOpenDisplay(NULL);
	if (!display) error("Can't open display");
	root = XDefaultRootWindow(display);
	atoms = sicalloc(ATOMS_LEN,sizeof(Atom));
	if (!XInternAtoms(display, ATOMS, ATOMS_LEN, False, atoms))
		error("Can't open display\n");
	if (!XQueryExtension(display, "XInputExtension", &xi_opcode,
			&query_event, &query_error))
		error("X Input extension not available\n");
	mask.deviceid = XIAllMasterDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = sicalloc(mask.mask_len, sizeof(char));
	XISetMask(mask.mask, XI_RawKeyPress);
	XISetMask(mask.mask, XI_RawKeyRelease);
	XISetMask(mask.mask, XI_RawButtonPress);
	XISetMask(mask.mask, XI_RawButtonRelease);
	unmask.deviceid = mask.deviceid;
	unmask.mask_len = mask.mask_len;
	unmask.mask = sicalloc(mask.mask_len, sizeof(char));
	XSetIOErrorHandler(io_error_handler);
	XSetErrorHandler(error_handler);
	adl_init(&display_list);

	//~ unsigned char * buf; unsigned long bs;
	//~ get_selection(display, root, atoms, &bs, &buf);
	//~ for (int g = 0;g<bs;g++)
	//~ printf("%lu\n",((Atom*)buf)[g]);
	//~ printf("\n");
	//~ for (int g = 0;g<bs;g++) {
		//~ printf("%lu\n",sd[g]);
		//~ printf("    %s\n",XGetAtomName(display,sd[g]));
	//~ }
	//~ printf("\n");
	//~ printf("%lu\n",atoms[5]);
	//~ XFree(buf);

	//~ set_selection(display,root,atoms,(unsigned char*)"Hello", (int) 6);
	//~ printf("hi\n");

  //~ FILE *fp;
  //~ char path[1035];
  //~ int size = 1;
  //~ char * buf=malloc(1);
  //~ buf[0]=0;
	  //~ fp = popen("xclip -o -selection clipboard | base64", "r");
  //~ if (fp == NULL) {
    //~ printf("Failed to run command\n" );
    //~ exit(1);
  //~ }

  //~ /* Read the output a line at a time - output it. */
  //~ while (fgets(path, sizeof(path)-1, fp) != NULL) {
	  //~ size+=sizeof(path)-1;
	  //~ buf = realloc(buf,size);
	  //~ strcat(buf,path);
    //~ printf("%s", path);
  //~ }
  //~ printf("%s", buf);
  //~ printf("\n");

  //~ /* close */
  //~ pclose(fp);

	while (app > -1) {
		debug("%i\n",app);
		if (!get_property(display, root, atoms[1], XA_WINDOW,
			get_format(sizeof(Window)),&n,(unsigned char **)&data)) n=0;
		for (i = 0; i < n; i++) {
			status = XFetchName(display, data[i], &name);
			if (status > 0 && search_display_in_name(name,out_name)) {
				status = adl_add(&display_list, out_name, data[i],
					&inx, ATOMS_LEN);
				XFree(name);
			} else { XFree(name); continue; }
			if (status) status = ad_open(display_list.list[inx],
					ATOMS, ATOMS_LEN);
			else continue;
			ad = display_list.list[inx];
			ad->used = 1;
			if (status == 0) {
				adl_del(&display_list, inx);
				continue;
			} else if (status == 1 || status == 2) {
				status = update_data(ad, display, atoms);
				if (status) {
					ad_close(ad);
					continue;
				}
			}
			start_wm(ad);
			update_data(ad, display, atoms);
			ad_close(ad);
		}
		for (i = 0; i < display_list.max_size; i++) {
			if (display_list.list[i] == NULL) continue;
			ad = display_list.list[i];
			if (ad->used == 0) adl_del(&display_list, i);
			else {
				//~ ad_close(display_list.list[inx]);
				ad->used = 0;
			}
		}
		XFree(data);
		XISelectEvents(display, root, &mask, 1);
		XSync(display, 0);
		XEvent event;
		XNextEvent(display, &event);
		XISelectEvents(display, root, &unmask, 1);
		XSync(display, 0);
		usleep(250000);
		if (DEBUG) app++;
	}

	adl_destruct(&display_list);
	free(mask.mask);
	free(unmask.mask);
	XCloseDisplay(display);
}
