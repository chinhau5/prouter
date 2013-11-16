/*
 * gui.c
 *
 *  Created on: Oct 4, 2013
 *      Author: chinhau5
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

void expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{

}

int main_gui()
{
	GtkWidget *widget;
	GtkDrawingArea *drawing_area;
	cairo_t *lol;

	drawing_area = gtk_drawing_area_new();
	//gtk_drawing_area_size(drawing_area, 200, 200);
//	widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
//	gtk_widget_show(widget);
//	cairo_surface
	g_signal_connect (G_OBJECT (drawing_area), "expose_event",
	                    G_CALLBACK (expose_event_callback), NULL);

	gtk_main();
}
