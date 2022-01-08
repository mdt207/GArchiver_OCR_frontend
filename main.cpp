#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>

//#include <vips/vips.h>
#include <vips/vips8>

//#include <cairo.h>

//#include <filesystem>
#include <vector>
//#include <cstdio>
#include <cmath>
#include <ctime>
#include <utility>
#include <string>
#include <fstream>
#include <iostream>

//#include "Image.h"
#include "utils.h"
#include "gtester_ico.h"


using namespace std;
using namespace vips;

struct 
{
  std::string err_msg;
  bool is_converted = false;
  bool is_text_buffer_content_changed = false;
  bool focus_in_text = false;
  gboolean timer;
  gdouble alpha;
  //gdouble size;
  gint  factor;
  guint counter;
  cairo_surface_t *image;
} glob;

typedef struct _Update {
	GtkWidget *drawing_area;
	VipsRect rect;
} Update;

struct
{
	VipsImage *image;
	VipsImage *display;
	VipsRegion *region;
}vips_glob;

typedef struct
{
	guint x; 
	guint y;
	guint width;
	guint height;
} Rect;

const char ver[]   = { "0.0.1" };
const char prog_name[]= {"GArchiver OCR utility"};

//Image image(10, 10, 4);
GdkPixbuf *myPixbuf = NULL;

GtkWidget *darea;
GtkWidget *text_view;
GtkWidget *scrolled_window, *scrolled_window2;
GtkWidget *aboutus_btn, *show_ocr_btn;
GtkWidget *settings_btn;


VipsImage *vimg = nullptr;

gchar *currDirPath = nullptr;
gulong kdb_handler_id;
guint timer_id;

gint width = 0, height = 0;
gboolean enable_all_controlls = TRUE;

bool isDrawing, is_session_restore, isROIcoordsCollecting = false;
bool isResize = false;
gboolean zoom_in = TRUE;

std::string img_file_name( "pic.png" );
std::vector<std::string> pCmd;
std::string tesseract_psm("0"), font_name("Sans Normal 14");

//Image image(&img_file_name[0]);

std::vector<Rect> array_roi_coords; //regions of interests
std::pair<guint, guint> start_pos, end_pos;

cairo_user_data_key_t surface_data_key;

static void copy_list_roi_to_dest();
std::string PathGetDir( std::string const &path );
std::string PathGetDirSeperator( std::string const &path );
static void do_drawing(cairo_t *);

/*-------------------local-procedures---------------------------------*/

static GdkColor change_color_text_view(std::string text_color)
{
	GdkColor _text_color, _bg_color;
	
	//gdk_color_parse ( (gchar*)&text_color[0], &_text_color);
	//gtk_widget_modify_text (text_view, GTK_STATE_NORMAL, &text_color);
	
	return _text_color;
}

static 
void copy_file_to_dest(std::string &fname, std::string dest) //noexcept(true)
{
	//cout << currDirPath << endl;
	namespace fs = std::filesystem;
	std::string currDir(currDirPath), f;
	//std::string to = dest;//PathGetDir (pCmd[1]);
	
	//f = currDir;
	//f.append(fname);//"/list_roi.txt");
	f = "\\";
	f += fs::path(fname).filename().string();
	
	//try
	{
		if ( is_file_exist (fname) )
		{
			string tmp = dest+f;
			cout << tmp << endl;
			//std::error_code err_code;
			//err_code = make_error_code (std::errc::no_such_file_or_directory);
			//cout << f << endl;
			//currDir += f;
			if ( is_file_exist (tmp) )
			{
				fs::remove (tmp);
				fs::copy (fname, tmp, fs::copy_options::overwrite_existing);
			}
			//throw(std::filesystem::filesystem_error(fname + "not exists.", err_code) );
		}
	}
	/*catch (std::filesystem::filesystem_error const& ex)
	{
		enable_all_controlls = FALSE;
		//cerr << ex.what() << endl;
		glob.err_msg.assign (ex.what());
	}*/
	
}

static bool globalInit()
{	
	ifstream ifpar;
	string conf_fname("config.txt");
	width = 551;
	height = 4;
	//vimg = vips_image_new();
	currDirPath = g_get_current_dir();
	
	try
	{
		if ( !is_file_exist (conf_fname ) ) //is ruby app exists
		{
			std::error_code err_code;
			err_code = make_error_code (std::errc::no_such_file_or_directory);
			enable_all_controlls = FALSE;
			throw(std::filesystem::filesystem_error("config.txt does not exist!", err_code) );
		}
		
		ifpar.open(conf_fname, ios::in);
	}
	catch (std::filesystem::filesystem_error const& ex)
	{
		enable_all_controlls = FALSE;
		//cerr << ex.what() << endl;
		glob.err_msg.assign (ex.what());
		return enable_all_controlls;
	}

    if(ifpar.is_open())
    {
		//cout << "check\n";
           //pCurrentDir = g_get_current_dir();
           std::string path_prefix;

           path_prefix.assign(currDirPath);

           pCmd.resize(3);
           for(short i=0; i < 2; i++)
           {
              //pCmd[i] = new char[128];
              getline(ifpar, pCmd[i]);

             //g_strstrip(pCmd[i]);
             //cout << i << ": " <<pCmd[i] << endl;
           }
           //getline(ifpar, str); //json template file name

           if( pCmd[0].size() == 0 )
           {
             pCmd[0] = "ruby\\not\\found";   //ruby path
             pCmd[1] = "script\\not\\found"; // script
			 glob.err_msg = pCmd[0] + pCmd[1];
			 enable_all_controlls = FALSE;
			 return enable_all_controlls;
           }
           else
           {
			   try 
			   {
				    if ( !is_file_exist( pCmd[0]) ) //is ruby app exists
					{
						std::error_code err_code;
						err_code = make_error_code (std::errc::no_such_file_or_directory);
						enable_all_controlls = FALSE;
						throw(std::filesystem::filesystem_error("ruby.exe application does not exist!", err_code) );
					}
					pCmd[2] = "list_roi.txt";
					path_prefix += PathGetDirSeperator(pCmd[1]);
					path_prefix += pCmd[2]; /* /list_roi.txt (/) - path delimiter */
					
					std::string dest = PathGetDir (pCmd[1]); //destination path
					//dest += PathGetDirSeperator(pCmd[1]);
					
					cout << dest << " " << path_prefix << endl;
					//if ( is_file_exist( pCmd[2] ) )
					if ( is_file_exist( path_prefix ) )
						copy_file_to_dest(path_prefix, dest);
					else
					{
						std::error_code err_code;
						err_code = make_error_code (std::errc::no_such_file_or_directory);
						
						std::string str("list_roi.txt not exists.");
						str += "\nTo make that file first select region from image\
						\nand push F9 button for conversion.";						
						
						enable_all_controlls = FALSE;
						throw(std::filesystem::filesystem_error(str, err_code) );
					}
					
					dest = PathGetDir (pCmd[1]);
					dest += PathGetDirSeperator(pCmd[1]);
					dest += "ocr_file.txt";
				
					if ( is_file_exist( dest ) )
						glob.is_converted = true;
				}
				catch (std::filesystem::filesystem_error const& ex)
				{
					//cerr << ex.what() << endl;
					glob.err_msg.assign (ex.what());
					return enable_all_controlls;
				}
		   }
      
          //path_prefix += "/" + pCmd[1];
          //pCmd[1] = path_prefix;
          
          ifpar.close();
		  
		  enable_all_controlls = TRUE;
          
          return enable_all_controlls;
         }
	enable_all_controlls = FALSE;
   return false;
}

static void globalDone()
{
	//VIPS_UNREF (vimg);
    //g_free (VIPS_OBJECT(vimg));
    try
    {
		currDirPath !=nullptr ? g_free ( currDirPath ) : throw(VError("current dir nullptr"));
		/*vips_glob.region  != nullptr ? g_object_unref ( vips_glob.region )  : throw(VError("nullptr"));
		vips_glob.display != nullptr ? g_object_unref ( vips_glob.display ) : throw(VError("nullptr"));
		vips_glob.image   != nullptr ? g_object_unref ( vips_glob.image )   : throw(VError("nullptr"));*/
		
		vips_shutdown ();
	}
	catch (const VError & e)
	{
		std::string msg(e.what ());
		//msg.assign ( e.what ());
		printf ("%s\n", msg.c_str ());//e.what ());
	}
}

/*-------------------end-of-local-procedures--------------------------*/

/*--------------------------------forwards--------------------------------*/
GtkWidget *load_pics_button, *save_pics_button, *convert_pics_to_text_btn;

static void load_pics_event( GtkWidget *widget, gpointer data );
cairo_surface_t *scale_to_half(cairo_surface_t *s, int orig_width, int orig_height);

static VipsImage *
vipsdisp_display_image( VipsImage *in, GtkWidget *drawing_area );
/*----------------------------end of forwards-----------------------------*/


/*---------------------------------libvips-graphics------------------------*/


/* The main GUI thread runs this when it's idle and there are tiles that need
 * painting. 
 */
static gboolean
render_cb( Update *update )
{
  gtk_widget_queue_draw_area( update->drawing_area,
			      update->rect.left, update->rect.top,
			      update->rect.width, update->rect.height );

  g_free( update );

  return( FALSE );
}

/* Come here from the vips_sink_screen() background thread when a tile has been
 * calculated. We can't paint the screen directly since the main GUI thread
 * might be doing something. Instead, we add an idle callback which will be
 * run by the main GUI thread when it next hits the mainloop.
 */
static void
render_notify( VipsImage *image, VipsRect *rect, void *client )
{
	GtkWidget *drawing_area = GTK_WIDGET( client );
	Update *update = g_new( Update, 1 );

	update->rect = *rect;
	update->drawing_area = drawing_area;

	g_idle_add( (GSourceFunc) render_cb, update );
}



static VipsImage *
vipsdisp_load( const char *filename )
{
	VipsImage *image;

	if( !(image = vips_image_new_from_file( filename, NULL ))) 
		return NULL;

	/* Attach an eval callback: this will tick down if we open this image
	 * via a temp file.
	 */
	vips_image_set_progress( image, TRUE ); 
	/*g_signal_connect( image, "preeval",
		G_CALLBACK( vipsdisp_preeval ), (void *) filename);
	g_signal_connect( image, "eval",
		G_CALLBACK( vipsdisp_eval ), (void *) filename);
	g_signal_connect( image, "posteval",
		G_CALLBACK( vipsdisp_posteval ), (void *) filename);*/

	return image;
}

static void vipsdisp_refresh()
{
	vips_image_invalidate_all (vips_glob.image);
	
	//vips_region_invalidate (vips_glob.region);
	
	g_object_unref (vips_glob.display);
	g_object_unref ( vips_glob.region );
	
				
	vips_glob.display = vipsdisp_display_image (vips_glob.image, darea);
	vips_glob.region = vips_region_new( vips_glob.display );
	
	gtk_widget_queue_draw (darea);

}


/* Make the image for display from the raw disc image. Could do
 * anything here, really. Uncomment sections to try different effects. Convert
 * to 8-bit RGB would be a good idea.
 */
static VipsImage *
vipsdisp_display_image( VipsImage *in, GtkWidget *drawing_area )
{
	VipsImage *image;
	VipsImage *x;

	/* Edit these to add or remove things from the pipeline we build. These
	 * should be wired up to something in a GUI.
	 */
	//const gboolean zoom_in = TRUE;
	//const gboolean zoom_out = FALSE;

	/* image represents the head of the pipeline. Hold a ref to it as we
	 * work.
	 */
	image = in;
	//g_object_ref( image ); 

	if( isResize )//zoom_out ) 
	{
		glob.factor = 1;
		isResize = false;
		if( vips_subsample( image, &x, glob.factor, glob.factor, NULL ) )
		//if( vips_zoom( image, &x, glob.factor, glob.factor, NULL ) )
		{
			g_object_unref( image );
			return( NULL ); 
		}
		g_object_unref( image );
		image = x;
	}

	if( zoom_in ) 
	{
		zoom_in = false;
		if( vips_zoom( image, &x, glob.factor, glob.factor, NULL ) )
		//if( vips_zoom( image, &x, 3, 3, NULL ) ) 
		{
			g_object_unref( image );
			return( NULL ); 
		}
		g_object_unref( image );
		image = x;
	}

	/* This won't work for CMYK, you need to mess about with ICC profiles
	 * for that, but it will do everything else.
	 */
	if( vips_colourspace( image, &x, VIPS_INTERPRETATION_sRGB, NULL ) ) {
		g_object_unref( image );
		return( NULL ); 
	}
	g_object_unref( image );
	image = x;

	/* Drop any alpha.
	 */
	if( vips_extract_band( image, &x, 0, "n", 3, NULL ) ) {
		g_object_unref( image );
		return( NULL ); 
	}
	g_object_unref( image );
	image = x;

	/*x = vips_image_new();
	if( vips_sink_screen( image, x, NULL, 128, 128, 400, 0, 
		render_notify, drawing_area ) ) {
		g_object_unref( image );
		g_object_unref( x );
		return( NULL );
	}
	g_object_unref( image );
	image = x;*/
	
	//cout << "gotcha zoom\n";

	return( image );
}

static void vips_draw_text(cairo_t *cr, GtkWidget *widget, 
							gint x, gint y, std::string &str)
{
  cairo_text_extents_t extents;
  
  if (widget != nullptr)
  {
	GtkWidget *win = gtk_widget_get_toplevel(widget);
  
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(win), &w, &h);
  }

  
  cairo_select_font_face(cr, "Courier",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, 27);
  
  //cairo_text_extents(cr, "ZetCode", &extents);
  cairo_text_extents(cr, str.c_str(), &extents);

  //cairo_move_to(cr, w/2 - extents.width/2, h/2);
  cairo_move_to(cr, x, 3+(y + extents.height/2));
  cairo_set_source_rgb(cr, 1, 0, 0);
  //cairo_set_source_rgba(cr, 1, 0, 0, 1);
  cairo_show_text(cr, &str[0]);
}


static void
vipsdisp_draw_rect( GtkWidget *drawing_area, 
	cairo_t *cr, VipsRegion *region, VipsRect *expose )
{
	VipsRect image;
	VipsRect clip;
	unsigned char *cairo_buffer;
	int x, y;
	cairo_surface_t *surface;

	/*printf( "vipsdisp_draw_rect: "
		"left = %d, top = %d, width = %d, height = %d\n",
		expose->left, expose->top,
		expose->width, expose->height );*/
		
		
	/*if(isResize)
	{
		VipsImage *image;
		VipsImage *x;
		
		int width, height;
		
		isResize = false;
		
		
		
		if( vips_subsample( vips_glob.image, &x, 4, 4, NULL ) ) 
		{
			g_object_unref( vips_glob.image );
			return;
		}
		g_object_unref( vips_glob.image );
		
		
		vips_glob.image = x;
		
		cout << "gotcha zoom\n";
	}*/
	
	/* Clip against the image size ... we don't want to try painting 
	 * outside the image area.
	 */
	image.left = 0;
	image.top = 0;
	image.width = vips_glob.region->im->Xsize;
	image.height = vips_glob.region->im->Ysize;
	vips_rect_intersectrect( &image, expose, &clip );
	//g_object_unref( vips_glob.region );
	//vips_glob.region = vips_region_new (vips_glob.display);
	if( vips_rect_isempty( &clip ) ||
		vips_region_prepare( vips_glob.region, &clip ) )
		return;

	
	/* libvips is RGB, cairo is ARGB, we have to repack the data.
	 */
	cairo_buffer = reinterpret_cast<unsigned char*>(g_malloc( clip.width * clip.height * 4 ));

	for( y = 0; y < clip.height; y++ ) {
		VipsPel *p = 
			VIPS_REGION_ADDR( vips_glob.region, clip.left, clip.top + y );
		unsigned char *q = cairo_buffer + clip.width * 4 * y;

		for( x = 0; x < clip.width; x++ ) {
			q[0] = p[2];
			q[1] = p[1];
			q[2] = p[0];
			q[3] = 0;

			p += 3;
			q += 4;
		}
	}

	//surface = cairo_image_surface_create_for_data( cairo_buffer, 
	//		CAIRO_FORMAT_RGB24, clip.width, clip.height, clip.width * 4 );
			
	if (glob.image != nullptr)
	{
		//while (cairo_surface_get_reference_count (glob.image) > 0)
		{
			cairo_surface_destroy( glob.image );
		}
	}
	
	glob.image = cairo_image_surface_create_for_data( cairo_buffer, 
			CAIRO_FORMAT_RGB24, clip.width, clip.height, clip.width * 4 );
	
	
	cairo_set_source_surface( cr, /*surface*/glob.image, clip.left, clip.top );

	cairo_paint( cr );

	g_free( cairo_buffer );
	
	if(isResize)
	{
		/*VipsImage *image;
		VipsImage *x;
		
		int width, height;*/
		
		//isResize = false;
		
		
		
		/*if( vips_subsample( vips_glob.image, &x, 4, 4, NULL ) ) 
		{
			g_object_unref( vips_glob.image );
			return;
		}
		g_object_unref( vips_glob.image );
		
		
		vips_glob.image = x;
		
		//cout << "gotcha zoom\n";
		
		/*x = vips_image_new();
		if( vips_sink_screen( vips_glob.image, x, NULL, 128, 128, 400, 0, 
			render_notify, drawing_area ) ) 
		{
			g_object_unref( vips_glob.image );
			g_object_unref( x );
			return;
		}
		g_object_unref( vips_glob.image );
		vips_glob.image = x;*/
		
		//width = cairo_image_surface_get_width (glob.image);
		//height = cairo_image_surface_get_width (glob.image);
		
		//glob.image = scale_to_half(glob.image, width, height);
		
		//cairo_surface_mark_dirty(glob.image);
	}
	
	/*cairo_set_source_rgb(cr, 0.1, 0, 0);
	cairo_select_font_face(cr, "Courier",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

	cairo_set_font_size(cr, 30);
	cairo_move_to(cr, 10, 30);
	cairo_show_text(cr, "test text");*/
	
	width  = end_pos.first - start_pos.first;
	height = end_pos.second - start_pos.second; 
    
	cairo_set_source_rgb(cr, 0.1, 0, 0);
	//cairo_rectangle(cr, 30, 60, (width + 1) + glob.size, (height + 1) + glob.size);
	cairo_rectangle(cr, start_pos.first-2, start_pos.second-2, width+2, height+2);
	cairo_set_line_width(cr, 2);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
	cairo_stroke(cr);
	
	//with the mouse drag drawing
	
	cairo_set_source_rgba(cr, 0, 0, 0, 0);//glob.alpha );//i*0.1);
	cairo_rectangle(cr, start_pos.first, start_pos.second, width, height);
	cairo_fill(cr);
	//end section of the mouse drag drawing
	
	if(array_roi_coords.size() != 0)
	{
		std::string str;
		
		for (int i = 0; i <= array_roi_coords.size()-1; ++i) 
		{
		
			/*cairo_set_source_rgb(cr, 0.1, 0, 0);
	
			cairo_rectangle(cr, array_roi_coords[i].x-2, array_roi_coords[i].y-2, 
							array_roi_coords[i].width+2, array_roi_coords[i].height+2);
			cairo_set_line_width(cr, 2);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
			cairo_stroke(cr);
		
			cairo_set_source_rgba(cr, 0, 0, 0, 0);//1, i*0.1);
			cairo_rectangle(cr, array_roi_coords[i].x, array_roi_coords[i].y,
		                    array_roi_coords[i].width, array_roi_coords[i].height);//50*i, 170, 40, 40);
			cairo_fill(cr);*/
			
			str = std::to_string(i+1); //order number of roi
			vips_draw_text(cr, nullptr, 
							array_roi_coords[i].x, array_roi_coords[i].y,
							str);
		}
  }

	//cairo_surface_destroy( glob.image ); //surface ); 
}


static void
vipsdisp_draw( GtkWidget *drawing_area, cairo_t *cr, VipsRegion *region )
{
	cairo_rectangle_list_t *rectangle_list = cairo_copy_clip_rectangle_list( cr );

	//printf( "vipsdisp_draw:\n" ); 

	if( rectangle_list->status == CAIRO_STATUS_SUCCESS ) 
	{ 
		int i;

		for( i = 0; i < rectangle_list->num_rectangles; i++ ) 
		{
			VipsRect expose;

			expose.left = rectangle_list->rectangles[i].x;
			expose.top = rectangle_list->rectangles[i].y;
			expose.width = rectangle_list->rectangles[i].width;
			expose.height = rectangle_list->rectangles[i].height;

			vipsdisp_draw_rect( drawing_area, cr, region, &expose );
		}
	}

	cairo_rectangle_list_destroy( rectangle_list );
}



/*-------------------------------end-of-libvips-graphics-------------------*/

/*static gboolean
kbd_press_event ( GtkWidget *text_view, GdkEventKey *event)
{
	switch (event->keyval)
    {
		case GDK_KEY_a:
        {
			std::cout << "ctrl gotcha\n"; 
			isROIcoordsCollecting ? array_roi_coords.clear(), isROIcoordsCollecting = false : isROIcoordsCollecting = true;
			//if (!isROIcoordsCollecting)
			//   array_roi_coords.clear();
		}break;
            
		default:break;
	}
	return FALSE;
}*/

static void
size_allocate_cb (GtkWidget *widget, GdkRectangle *r, gpointer user_data)
{
  GtkAdjustment *hadjustment = GTK_ADJUSTMENT(user_data);
	
  //printf("size_allocate_cb called\n");
  int h = gtk_widget_get_allocated_width(widget);
  //printf("allocated_width %d\n", h);
  //printf("allocated_width %d\n", r->width);
  gtk_adjustment_set_upper(hadjustment, r->width);
}

static gboolean
change_value_cb (GtkWidget *widget, gdouble val, gpointer user_data)
{
  //printf("change_value_cb called, %f\n", val);
  return FALSE;
}



static gboolean
kbd_press_event ( GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	   GtkWidget *window = GTK_WIDGET(data);
       guint32 _time;
       guint *keyval;
       //GdkKeymap keymap;
       GdkModifierType consumed;
       /*gdk_keymap_translate_keyboard_state (&keymap, event->hardware_keycode,
                                            (GdkModifierType)event->state, event->group, keyval,
                                            NULL, NULL, &consumed);*/
    //if(event->keyval == GDK_Control_L)
    //if( (event->state & GDK_CONTROL_MASK ) == GDK_CONTROL_MASK )
    {
      /*if ( (/*(event->keyval == GDK_Up ||  event->keyval ==  GDK_KP_Up)/ event->keyval ==  GDK_KEY_Clear || event->keyval ==  GDK_KEY_KP_Begin
       /*(event->keyval == GDK_Down ||  event->keyval == GDK_KP_Down)/) &&
      !gtk_widget_is_focus (file_tree_view)  && !bfocus_in_text )
      gtk_widget_grab_focus (file_tree_view);*/
      
      //cout << "Keycode:" << event->hardware_keycode << " " << gdk_keyval_name(event->keyval) <<  endl;

       if(event->keyval == GDK_KEY_Escape )
       {
            //if(is_mark_changed) Sync_Mark();
            //GetNextMark();//chMarkDir);
        }

       //if((!bfocus_in_text && is_marking) && (event->keyval == GDK_KEY_space ||
       //                                                        event->keyval == GDK_KEY_1 ||
       //                                                        event->keyval == GDK_KEY_2))//Control_L)
       //if( event->hardware_keycode == 37 )
       {
          //pKeyEvent = (GdkEventKey*)gdk_event_copy((GdkEvent*)event);

         /*GtkTreeIter iter, parent_iter;
         GtkTreePath *path;
         GtkTreeViewColumn *column;*/

         /*switch(gdk_keyboard_grab(event->window, FALSE, event->time))
         {
             case GDK_GRAB_SUCCESS: cout << "allright" << endl; break;
             case GDK_GRAB_ALREADY_GRABBED: cout << "ALREADY_GRABBED" << endl; break;
             case GDK_GRAB_INVALID_TIME: cout << "INVALID_TIME" << endl; break;
             case GDK_GRAB_NOT_VIEWABLE: cout << "NOT_VIEWABLE" << endl; break;
             case GDK_GRAB_FROZEN: cout << "FROZEN" << endl; break;
         }*/


         //gtk_widget_grab_focus (GTK_WIDGET(irbis_tree_view));

         //chMarkDir = 0;
         //(event->keyval == GDK_KEY_1) ? gMarkDir = 0 : event->keyval == GDK_KEY_2 ? gMarkDir = 1 : gMarkDir;
         //if(is_mark_changed) Sync_Mark();
         //GetNextMark();//chMarkDir);

         /*gtk_tree_view_get_cursor(GTK_TREE_VIEW(irbis_tree_view), &path, NULL);
         gtk_tree_path_next(path);
         gtk_tree_selection_select_path (selection, path);
         gtk_tree_view_set_cursor(GTK_TREE_VIEW(irbis_tree_view), path, NULL, FALSE);
         gtk_tree_path_free (path);*/

       //g_signal_emit_by_name(G_OBJECT (change_mark), "clicked");//good but not enough
       //g_signal_emit_by_name(G_OBJECT (irbis_tree_view), "key-press-event"); //failed

       //return FALSE;
       //cout << "Keycode:" << event->hardware_keycode << (event->state & GDK_CONTROL_MASK) << endl;

       //g_signal_emitv(&gVal, kdb_handler_id, (GQuark)0, NULL);

       }
       //cout << "Keycode:" << event->hardware_keycode << " " << (event->state & GDK_CONTROL_MASK) << " " << event->keyval << endl;
       /*switch(event->type)
       {
         case GDK_KEY_PRESS:  cout << "press" << endl; break;
         case GDK_KEY_RELEASE:cout << "release" << endl; break;
       }*/
    }
    //gdk_keyboard_ungrab(event->time);
        switch (event->keyval)
        {
			case GDK_KP_Add:
			{
				if(glob.factor < 4)
				{
					zoom_in = TRUE;
					isResize = false;
					
					++glob.factor;
				
					/*vips_region_invalidate (vips_glob.region);
					g_object_unref ( vips_glob.region );
				
				
					vips_glob.display = vipsdisp_display_image (vips_glob.image, darea);
				
					vips_glob.region = vips_region_new( vips_glob.display );*/
					vipsdisp_refresh();
				}
				
			}break;
			case GDK_KP_Subtract:
			{
				VipsRect image;
				VipsImage *x;
				VipsRegion *region;
				
				//int width, height;
				//
				if(glob.factor > 1)
				{
					zoom_in = FALSE;
					isResize = true;
					vipsdisp_refresh();
				}
				
				//glob.factor > 1 ? --glob.factor : glob.factor;
				
				//width = cairo_image_surface_get_width (glob.image);
				//height = cairo_image_surface_get_width (glob.image);
				
				//glob.image = scale_to_half(glob.image, width, height);
				//width  -= 3;
				//height -= 1;
				
				/*if( vips_subsample( vips_glob.display, &x, 4, 4, NULL ) ) 
				{
					g_object_unref( vips_glob.display );
					return false;
				}
				
				g_object_unref( vips_glob.display );
				vips_glob.display = x;*/
				
				//vips_region_invalidate (vips_glob.region);
				//g_object_unref ( vips_glob.region );
				
				
				/*if( !(vips_glob.image = vipsdisp_load(&img_file_name[0]) ) )
					vips_error( "unable to load %s", &img_file_name[0] );*/
				
				/*vips_glob.display = vipsdisp_display_image (vips_glob.image, darea);
				
				vips_glob.region = vips_region_new( vips_glob.display );*/
				
				
				/*image.left = 0;
				image.top = 0;
				image.width = region->im->Xsize;
				image.height = region->im->Ysize;*/
				
				//VIPS_UNREF ( vips_glob.region );
				//g_free (VIPS_OBJECT(vips_glob.region));
				
				//vips_glob.region = region;
				
				//vips_region_copy ( region, vips_glob.region, &image, 0, 0 );
				//vips_region_prepare_to ( region, vips_glob.region, &image, 0, 0 );
				
				//vips_region_black (vips_glob.region);
				
				//VIPS_UNREF ( region );
				//g_free (VIPS_OBJECT(region));
				
				//cout << "gotcha zoom " << image.width <<"\n";
				
				//cairo_surface_mark_dirty(glob.image);
				//cairo_surface_flush (glob.image);
				//g_signal_emit_by_name(G_OBJECT (window), "draw");
				//gtk_widget_queue_draw(widget);
			}break;
			
		   case GDK_KEY_a:
           {
			 //std::cout << "ctrl gotcha\n"; 
			 isROIcoordsCollecting ? array_roi_coords.clear(), isROIcoordsCollecting = false : isROIcoordsCollecting = true;
			 //if (!isROIcoordsCollecting)
			 //   array_roi_coords.clear();
		   }break;
		   
           case GDK_KEY_F1:
           {
                //help_index = 1;
                g_signal_emit_by_name (G_OBJECT (aboutus_btn), "clicked");
                //help_index = 0;
           }break;
           case GDK_KEY_F2:
           {
               //cout << "F2" << endl;
               //g_signal_emit_by_name(G_OBJECT (save_all_cards), "clicked");
           }break;
           case GDK_KEY_F3:
            {
               g_signal_emit_by_name(G_OBJECT (load_pics_button), "clicked");
            }break;
            case GDK_KEY_F5:
           {
			   if (glob.is_converted || glob.is_text_buffer_content_changed)
			   {
				 glob.is_converted = false;
				 glob.is_text_buffer_content_changed = false;
				 
				 if (!gtk_widget_is_focus (scrolled_window) )
					gtk_widget_grab_focus (scrolled_window);
				 
				 g_signal_emit_by_name (G_OBJECT (show_ocr_btn), "clicked");
			   }
                
           }break;
            case GDK_KEY_Control_L:
            {
                //gMarkDir = 0; /*g_mark_indx = psys->get_marks_num()-1;*/
                //g_signal_emit_by_name(G_OBJECT (change_mark), "clicked");
                
                //g_signal_emit_by_name(G_OBJECT (change_mark), "button-press-event");
            }break;
            case GDK_KEY_Alt_L:
            {
                    //gMarkDir = 1; /*g_mark_indx = 0;*/
                    //g_signal_emit_by_name(G_OBJECT (change_mark), "clicked");

            }break;
           case GDK_KEY_F7:
           {
               g_signal_emit_by_name(G_OBJECT (settings_btn), "clicked");
           } break;
           case GDK_KEY_F9:
           {
               //cout << "F9" << endl;
             if( gtk_widget_is_sensitive (convert_pics_to_text_btn) )
             {
                g_signal_emit_by_name (G_OBJECT (convert_pics_to_text_btn), "clicked");
             }
           }break;
           case GDK_KEY_F10:
           {
             //quit the game
             g_signal_emit_by_name (G_OBJECT (window), "destroy");
           }break;
           default:break;
        }
    /*else*/ return false;
}


static gboolean mouse_moved(GtkWidget *widget,GdkEvent *event, gpointer user_data) 
{

    if (event->type==GDK_MOTION_NOTIFY) 
    {
        GdkEventMotion* e = (GdkEventMotion*)event;
        //printf("Coordinates: (%u,%u)\n", (guint)e->x,(guint)e->y);
        
        if(isDrawing)
        {
			//todo:
			end_pos.first  = (guint)e->x;
			end_pos.second = (guint)e->y;
		}
        gtk_widget_queue_draw(widget);
        
        return TRUE;
    }
    return TRUE;
}

static gboolean on_mouse_button_clicked(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	//todo:
	if (event->type==GDK_BUTTON_PRESS) 
    {
		GdkEventButton* e=(GdkEventButton*)event;
		//printf("Coordinates: (%u,%u)\n", (guint)e->x,(guint)e->y);
		//e->button == 1 ? printf("Left\n") : e->button==3 ? printf("Right\n") : printf("Pressed\n");
		if( e->button == 1 )//|| e->button == 3)
		{
			start_pos.first = (guint)e->x;
			start_pos.second = (guint)e->y;
			
			end_pos.first = (guint)e->x;
			end_pos.second = (guint)e->y;
			
			isDrawing = true;
			
			gtk_widget_queue_draw(widget);
		}
	}
  return TRUE;
}

static gboolean on_mouse_button_released(GtkWidget *widget,GdkEvent *event,gpointer user_data)
{
	//todo:
	Rect rect;
	GdkEventButton* e = (GdkEventButton*)event;
	
	if( e->button == 1 )
	{
	  end_pos.first  = (guint)e->x;
	  end_pos.second = (guint)e->y;
			
	  isDrawing = false;
	}
	
	if(isROIcoordsCollecting && array_roi_coords.size() < 10)
	{
		if ( (end_pos > start_pos) && (end_pos.second > start_pos.second) )
		{
			//check and fix mouse x pos if it's out of img region
			if ( end_pos.first  > vips_glob.region->im->Xsize )
			{
				end_pos.first  = vips_glob.region->im->Xsize;
				//width  = end_pos.first - start_pos.first;
			}
			//check and fix mouse y pos if it's out of img region
			if ( end_pos.second > vips_glob.region->im->Ysize )
			{
				end_pos.second = vips_glob.region->im->Ysize;
				//height = end_pos.second - start_pos.second;
			}
			
			rect.x      = start_pos.first;
			rect.y      = start_pos.second;
			rect.width  = end_pos.first - start_pos.first;
			rect.height = end_pos.second - start_pos.second;
			array_roi_coords.push_back(rect);
		}
    }
	
	gtk_widget_queue_draw(widget);
	
	//cout << "start point: " << start_pos.first << " " << start_pos.second << endl;
	//cout << "wh: " << width << " " << height << endl;
	
	return TRUE;
}


cairo_surface_t *scale_to_half(cairo_surface_t *s, int orig_width, int orig_height)
{
    cairo_surface_t *result = cairo_surface_create_similar(s,
            cairo_surface_get_content(s), orig_width, orig_height);
    cairo_t *cr = cairo_create(result);
    cairo_scale(cr, 0.5, 0.5);
    cairo_set_source_surface(cr, s, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);
    return result;
}


static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, 
    gpointer user_data)
{      
  //do_drawing(cr);
  vipsdisp_draw(widget, cr, reinterpret_cast<VipsRegion *> (user_data) );

  return FALSE;
}

static void do_drawing(cairo_t *cr)
{
  cairo_set_source_surface(cr, glob.image, 10, 10);
  cairo_paint(cr);
  
  gint i;
  //for ( i = 1; i <= 10; i++) 
  {
	if (glob.alpha > 0 && glob.timer)
	{ 
	  glob.alpha -= 0.01;
	  //glob.size  -= 3.0;
	  //(width + glob.size) > 0 || (height + glob.size) > 0 ? glob.size -= 3.0 : glob.size;
	}
	if(glob.alpha <= 1.0 && !glob.timer)
	{
		glob.alpha += 0.01;
		//((width + glob.size) <= width) || ((height + glob.size) <= height) ? glob.size  += 3.0 : glob.size;
	}  
	
	/*cairo_set_source_rgba(cr, 0.1, 0.1, 1, 0.7);//glob.alpha );//i*0.1);
	cairo_rectangle(cr, 31, 61, width + glob.size, height + glob.size);
	cairo_fill(cr);*/
  }
  
    width  = end_pos.first - start_pos.first;
	height = end_pos.second - start_pos.second; 
    
	cairo_set_source_rgb(cr, 0.1, 0, 0);
	//cairo_rectangle(cr, 30, 60, (width + 1) + glob.size, (height + 1) + glob.size);
	cairo_rectangle(cr, start_pos.first-2, start_pos.second-2, width+2, height+2);
	cairo_set_line_width(cr, 2);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
	cairo_stroke(cr);
	
	//with the mouse drag drawing
	
	cairo_set_source_rgba(cr, 0, 0, 0, 0);//glob.alpha );//i*0.1);
	cairo_rectangle(cr, start_pos.first, start_pos.second, width, height);
	cairo_fill(cr);
	//end section of the mouse drag drawing
  if(isResize)
  {
	isResize = false;
    
	//glob.image = scale_to_half(glob.image, 640, 360);
	cairo_surface_mark_dirty(glob.image);
	//gtk_widget_queue_draw(widget);
  }
  	
  if(array_roi_coords.size() != 0)
  {
	for ( i = 0; i <= array_roi_coords.size()-1; ++i) 
	{
		
		cairo_set_source_rgb(cr, 0.1, 0, 0);
	
		cairo_rectangle(cr, array_roi_coords[i].x-2, array_roi_coords[i].y-2, 
							array_roi_coords[i].width+2, array_roi_coords[i].height+2);
		cairo_set_line_width(cr, 2);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND); 
		cairo_stroke(cr);
		
		cairo_set_source_rgba(cr, 0, 0, 0, 0);//1, i*0.1);
		cairo_rectangle(cr, array_roi_coords[i].x, array_roi_coords[i].y,
		                    array_roi_coords[i].width, array_roi_coords[i].height);//50*i, 170, 40, 40);
		cairo_fill(cr);
	}
  }
  
  if (glob.alpha <= 0) 
  {
      glob.timer = FALSE;
      //glob.alpha = 1.0;
  }
  if(glob.alpha == 1.0)
  {
	  glob.timer = TRUE;
	  ++glob.counter;
  }
}

static gboolean time_handler(GtkWidget *widget)
{ 
  //if (!glob.timer) glob.size += 1.0;
  
  /*if (glob.counter >= 5)*/ //return FALSE;
  //static int i = 0;
  //std::cout << ++i << std::endl;
  gtk_widget_queue_draw(widget);

  return TRUE;
}


static void surface_destroy()
{
	
}

static VipsImage* vips_load_file_type(const char* file_name)
{
  VipsImage *image = nullptr;
  ImageType type = getFileType(file_name); 
  int success;
  
  try
  {
	VImage in;  
	switch (type) 
	{ 
	    case PDF:
        { 
			//success = stbi_write_png(filename, w, h, channels, data, 0);
			//try
			{
				//cout << file_name << endl;
    
				//VImage in;// = VImage::new_from_file (argv[1]);
				in = VImage::pdfload(file_name, VImage::option()
											->set("page", 0)
											->set("n", 1));
											
				//g_object_unref ( vips_glob.image );
				//vips_glob.image = in.get_image();
				image = in.get_image();
				
				/*if (vips_glob.image != nullptr)
				{
					g_object_ref ( VIPS_OBJECT(vips_glob.image ) );
					vipsdisp_refresh();
				}*/
			}
			/*catch (const VError & e)
			{
				std::string msg;//(e.what ());
				msg.assign ( e.what ());
				printf ("%s\n", msg.c_str ());//e.what ());
			}*/
		}
        break;
        case PNG:
        { 
			//success = stbi_write_png(filename, w, h, channels, data, 0);
			in    = VImage::pngload(file_name);
			image = in.get_image();
		}
        break;
        case JPG:
        { 
			//success = stbi_write_jpg(filename, w, h, channels, data, 100);
			in    = VImage::jpegload(file_name);
			image = in.get_image();
		}
        break;
        
        //case BMP:
        //{ 
			//success = stbi_write_bmp(filename, w, h, channels, data);
			//in    = VImage::ppmload(file_name);
			//image = in.get_image();
		//}break;
        //break;
        //case TGA: success = stbi_write_tga(filename, w, h, channels, data);
        //break;
	}
  }
  catch (const VError & e)
  {
	std::string msg;//(e.what ());
	msg.assign ( e.what ());
	printf ("%s\n", msg.c_str ());//e.what ());
  }
  return image;//success != 0;
}

static void load_pics_event( GtkWidget *widget, gpointer data )
{
  //Image image(&img_file_name[0]);
// gboolean select_multiple;
 GtkWidget *dialog;
 GtkWidget *window = GTK_WIDGET(data);
 char *filename;
 GSList *filenames;

 is_session_restore = false;

 dialog = gtk_file_chooser_dialog_new ("Open File",
				      GTK_WINDOW(window),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

 //gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (dialog), TRUE);

  //cout << loadFromPath << endl;

  /*if( loadFromPath.size() > 0 )
  {
        gchar *path;
        path = g_path_get_dirname( (const gchar *)&loadFromPath[0] );
        //cout << path << endl;

        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),(const gchar *)path);
        /gtk_file_chooser_select_filename    (GTK_FILE_CHOOSER (dialog),
                                                            (const char *)path); /

        g_free(path);
    }*/


 if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
   {
	  std::string file_name; 

     filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
     img_file_name.assign(filename);
     g_free (filename);
     //filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
    //open_file (filename);
    // g_print("%s", (gchar*)g_slist_nth_data (filenames, 1));

      guint num = 1;//g_slist_length(filenames);
      //gFrom_1_file = (int)num;
      //cout << num << endl;
        /*for( guint i=0; i < num; i++)
        {
          filename = (char*)g_slist_nth_data (filenames, i);
          //g_print("%s\n", (gchar*)filename );
          files.push_back(filename);
          g_free (filename);
        }*/
      if(img_file_name.length() > 0)//(num > 1)
      {
		  
		/*try
		{
			if(vips_pdfload(&img_file_name[0], &vimg) != 0)  
				throw (VError ("file format: not pdf"));
       
			vips_pngsave(vimg, "test2.png");
		}  
		catch (const VError & e)
		{
		if ( vimg != nullptr )
			{
				VIPS_UNREF (vimg);
				g_free(VIPS_OBJECT (vimg));
				std::cout << "memory freed\n";
			}
	
			std::string msg;//(e.what ());
			msg.assign ( e.what ());
			printf ("%s\n", msg.c_str ());//e.what ());
			goto done;
		}*/
		  
		  
		  
		  
		  
		  //std::cout << file_name <<"\n";
		  //static const cairo_user_data_key_t surface_data_key;
		  
		  cairo_format_t format = cairo_image_surface_get_format(glob.image);
		  //cout << "1.surface format: " << format << endl;
		  if (glob.image != NULL)
		  {
			  //std::cout << "glob.image destroyed  \n";
			  cairo_surface_destroy(glob.image);
			  glob.image = nullptr;
		  }
		  
		  /*unsigned char*/uint8_t* surface_data = nullptr;
		  
		  if (vips_glob.image != nullptr)
		  { 
			 g_object_unref( vips_glob.image );
			 //g_free(vips_glob.image);
		  }
		  //if( !(vips_glob.image = vipsdisp_load(&img_file_name[0]) ) )
		  //   vips_error( "unable to load %s", &img_file_name[0] );
		  
		  if( !(vips_glob.image = vips_load_file_type(&img_file_name[0]) ) )
		     vips_error( "unable to load %s", &img_file_name[0] );
		     
		  //g_object_ref( vips_glob.image );
		  
		  if (vips_glob.image != nullptr)
		  {
			glob.is_converted = false;
		    vipsdisp_refresh();
		  }
		  
		  //image.read(&img_file_name[0]);
		  
		  //std::cout << "width: " << image.w << " height: " << image.h  << " channels: " << image.channels << "\n";
		  
		  //gint stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32/*format*/, image.w);
		  
		  //glob.image = cairo_image_surface_create_for_data(image.data, format, image.w, image.h, stride);
		  
		  //glob.image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32/*format*/, image.w, image.h);
		  //glob.image = cairo_image_surface_create_from_png(&img_file_name[0]);
		  
		  //format = cairo_image_surface_get_format(glob.image);
		  //cout << "2.surface format: " << format << endl;
		  
		  //if(glob.image)
		  {
			   //std::cout << stride <<" new image surface\n";
			   //surface_data = cairo_image_surface_get_data(glob.image);
			   //if (surface_data != nullptr)
			   {
					//memcpy(surface_data, image.data, image.w * image.h * 4/*image.channels*/ * sizeof(uint8_t/*unsigned char*/));
					//cairo_surface_mark_dirty(glob.image);
			   }
			   //format = cairo_image_surface_get_format(glob.image);
		       //std::cout << "channels: " << image.channels << std::endl;
		  }
		  
		  //cairo_status_t status = cairo_surface_set_user_data(glob.image, &surface_data_key, image.data, 
		  //                               (cairo_destroy_func_t)surface_destroy);
		  
		  //if (G_UNLIKELY(CAIRO_STATUS_SUCCESS != status))
		  //{
			  //cairo_surface_destroy(glob.image);
			  //cout << "cairo surface not set data \n";
		  //}
        //psys->set_cards_files( files );


       //GlobalInit();

       //gtk_widget_set_sensitive(marking_done, TRUE);
      }
      else
      {
          //cout << "junk:" << junkFile << endl;
          //files.push_back(junkFile);
          //psys->set_cards_files( files, true );
      }
      //loadFromPath = files[0];
      //fill_list_file_name();
      //g_slist_free(filenames);
   }


 /*select_multiple = gtk_file_chooser_get_select_multiple(GTK_FILE_CHOOSER (dialog));
 if( select_multiple ) g_print("TRUE"); */
done:
    gtk_widget_destroy (dialog);
}


static void save_pics_event( GtkWidget *widget, gpointer data )
{
//  Image image(&img_file_name[0]);
// gboolean select_multiple;
 GtkWidget *dialog;
 GtkWidget *window = GTK_WIDGET(data);
 char *filename;
 GSList *filenames;

 is_session_restore = false;


  dialog = gtk_file_chooser_dialog_new ("Save File",
				      NULL,//parent_window,
				      GTK_FILE_CHOOSER_ACTION_SAVE,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);			      

 //gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (dialog), TRUE);

  //cout << loadFromPath << endl;

  /*if( loadFromPath.size() > 0 )
  {
        gchar *path;
        path = g_path_get_dirname( (const gchar *)&loadFromPath[0] );
        //cout << path << endl;

        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),(const gchar *)path);
        /gtk_file_chooser_select_filename    (GTK_FILE_CHOOSER (dialog),
                                                            (const char *)path); /

        g_free(path);
    }*/


 if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
 {
	 char *filename;
     gchar *tmpFilePath;
	 std::string file_name; 

     filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
     //file_name.assign(filename);
     
     tmpFilePath = g_get_current_dir();
     
     g_print("%s", (gchar*)filename);
     
     file_name.assign(filename);
     
     g_free(filename);
     g_free(tmpFilePath);

      
      if(file_name.length() > 0)//(num > 1)
      {
		  //std::cout << file_name <<"\n";
		  //static const cairo_user_data_key_t surface_data_key;
		  
		  //std::fstream ftmp(file_name, std::ios::out);
          //ftmp.close();
    	  
    	  	  
		  //cout << "\n" << "width: " << image.w << " height: " << image.h <<"\n";
		  cout << "width: " << width << " height: " << height <<"\n";
		  cout << vips_glob.region->im->Xsize << " " << vips_glob.region->im->Ysize << endl;
		  cout << start_pos.first << " " << start_pos.second << endl;
		  cout << end_pos.first << " " << end_pos.second << endl;
		  
		  //cout << "2.start point: " << start_pos.first << " " << start_pos.second << endl;
	      //cout << "2.wh: " << width << " " << height << endl;
		  
		  //image.crop(start_pos.first-10, start_pos.second-10, width, height );
		  //image.write(reinterpret_cast<const char*>(file_name.c_str()));
		  
		  VImage vips_in(vips_glob.display);
		  VImage vips_out;
		  //check and fix mouse x pos if it's out of img region
		  if ( end_pos.first  > vips_glob.region->im->Xsize )
		  {
			  end_pos.first  = vips_glob.region->im->Xsize;
			  width  = end_pos.first - start_pos.first;
		  }
		  //check and fix mouse y pos if it's out of img region
		  if ( end_pos.second > vips_glob.region->im->Ysize )
		  {
			
			end_pos.second = vips_glob.region->im->Ysize;
			height = end_pos.second - start_pos.second;
		  }
		  
		  vips_out = vips_in.crop(start_pos.first, start_pos.second, width, height);
		  vips_out.pngsave("test_vips.png");
		  
      }
      else
      {
          //cout << "junk:" << junkFile << endl;
      }
   }


    gtk_widget_destroy (dialog);
}


std::string get_path_from_uri(GUri *uri)
{
	std::string str("empty uri");
	if(uri != NULL)
	{
		const gchar *path = g_uri_get_path(uri);
		str.assign(path);
	}
	return str;
}


std::string PathGetFileNameOrExtension( std::string const &path, int which_one = 0 )
{
	std::string res;
	
	//find the last dot
	size_t dotIdx = path.find_last_of( "." );
	if( dotIdx != std::string::npos )
	{
		//find the last directory seperator, if any
		size_t dirSepIdx = path.find_last_of( "\\" );//( "/\\" );
		//cout << dirSepIdx << endl;
		if( dotIdx > dirSepIdx + 1 )
		{
			//cout << dotIdx << " " << dirSepIdx+1 << endl;
			switch(which_one)
			{
				case 0:
			    {
					res = path.substr( dirSepIdx+1, abs(int(dotIdx - (dirSepIdx + 1))) ); //file name
					//res = path.substr( 0, dotIdx ); //file name
				}break;
				case 1: 
				{
					res = path.substr( dotIdx + 1); //extension
				}break;
			}
		}
	}
	
	return res;
}


std::string PathGetDirSeperator( std::string const &path )
{
	std::string sep;
	
		//find the last directory seperator, if any
		size_t dirSepIdx = path.find_last_of( "\\" );//( "/\\" );
		if( dirSepIdx != std::string::npos )
		{
			sep = path.substr( dirSepIdx, (dirSepIdx + 1) - dirSepIdx);
		}
	
	//std::cout << "seperator:" << sep << " " << dirSepIdx << std::endl;
	return sep;
}

std::string PathGetDir( std::string const &path )
{
	std::string sep;
	
		//find the last directory seperator, if any
		size_t dirSepIdx = path.find_last_of( "/\\" );
		if( dirSepIdx != std::string::npos )
		{
			sep = path.substr( 0, dirSepIdx );
		}
	
	//std::cout << "path: " << sep << std::endl;
	return sep;
}


static void save_pics_in_cache_dir(std::string &file_name, Rect &rect)
{
	VImage vips_in(vips_glob.display);
	VImage vips_out;
	  
	vips_out = vips_in.crop(rect.x, rect.y, rect.width, rect.height);
	vips_out.pngsave(&file_name[0]);
		  
	/*time_t rawTime;
	gchar buf_tm[20];
	struct tm *pTime;
	 
	std::string place_to_save_img(currDirPath);
	std::string str;
	
	time( &rawTime );
	pTime = localtime( &rawTime );
	strftime(buf_tm, sizeof(buf_tm), "%Y.%m.%d_%H%M%S", pTime);
		  
	str = PathGetDirSeperator(place_to_save_img);
	str += file_name;
	str.append(buf_tm);*/
	
}

void 
about_us( GtkWidget *widget,
          gpointer   data )
{
 //GdkPixbuf *myPixbuf = gdk_pixbuf_from_pixdata(&GTesterLogo, FALSE, NULL);
 const gchar *authors[] = {
 "TKL K96 207", "zdamin@inbox.ru",
   NULL
  };

// gtk_about_dialog_set_logo_icon_name()
 gtk_show_about_dialog (GTK_WINDOW(data),
        "program-name", prog_name,
        "copyright", "Muqobil Dasturlar To'plami (c) Build: January 1 2022 - GTK+ 3.22.0",
        "license-type", GTK_LICENSE_GPL_3_0,
        "version", ver,
        "comments", "O'zbekiston, Toshkent shahri\n  muqobildasturlar@gmail.com",
        "title", ("About GArchiver"),
        "authors", authors,
        //"logo-icon-name", "gtester.ico",
        "logo", (const GdkPixbuf*)myPixbuf,
       NULL);

 //g_free(myPixbuf);
}


int Print_Message(char* _str, gpointer data, int options = 0)
{
	std::string foreground_color, background_color;
	GtkTextBuffer *text_buffer = reinterpret_cast<GtkTextBuffer *>(data);
	
    gchar *entry_text;
    GtkTextIter iter, start, end;
    
    gtk_text_buffer_get_start_iter (text_buffer, &start);
    gtk_text_buffer_get_end_iter (text_buffer, &end);

    gtk_text_buffer_delete(text_buffer, &start, &end);
    gtk_text_buffer_get_iter_at_offset (text_buffer, &iter, 0);
    
    switch (options)
    {
		case 1:
		{
			foreground_color = "red_foreground";
			background_color = "white_background";
			entry_text = g_convert( _str, -1, "UTF-8", "UTF-8", NULL, NULL, NULL);
			entry_text = g_strdelimit (entry_text, "\b\f\t\v", ' ');
			
			gtk_text_buffer_insert_with_tags_by_name (text_buffer,\ 
										&iter,\
										entry_text,\
										-1,\
										&foreground_color[0],\
										&background_color[0],\
										nullptr);
			return 0;
		}break;
		case 2:
		{
			entry_text = g_convert( _str, -1, "UTF-8", "UTF-8", NULL, NULL, NULL);
			entry_text = g_strdelimit (entry_text, "\b\f\n\r\t\v", ' ');
		}break;
		default: entry_text = g_convert( _str, -1, "UTF-8", "UTF-8", NULL, NULL, NULL); break;
	}
    
    gtk_text_buffer_insert (text_buffer, &iter, entry_text , -1);
    //entry_text = g_convert( _str, -1, "UTF-8", "UTF-8", NULL, NULL, NULL);
	
}


static void
toggle_ocr_psm_cb (GtkToggleButton *check_button, gpointer data)
{
	int len;
	//gchar *which = reinterpret_cast<gchar*>(data);
	int *which = reinterpret_cast<int*>(data);
 
	if(gtk_toggle_button_get_active(check_button))
	{
		//switch(atoi((char*)which))
		tesseract_psm = to_string(*which);
		/*switch(*which)
		{
			case 0: tesseract_psm = "0"; break;
			case 1: tesseract_psm = "1"; break;
			case 2: tesseract_psm = "2"; break;
			case 3: tesseract_psm = "3"; break;
			default: tesseract_psm = "0";
		}*/
		//cout << "ocr pst: " << *which << " " << tesseract_psm << endl;
	}
}


void font_select_cb(GtkFontButton *widget,
                  gpointer       user_data)
{
  GtkWidget *font_btn = GTK_WIDGET(widget);
  gchar *fnt_name = nullptr;

  //cout << "font: " << fnt_name <<endl;
  
  int *sig = (int*)(user_data);
  if( *sig != 1)
  {
    if(fnt_name != nullptr) g_free(fnt_name);
    fnt_name = g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(font_btn)) );
	//cout << "font: " << fnt_name <<endl;
    //if(strlen(font_name) )
  }

  //cout << font_name << endl;
  gtk_font_button_set_font_name(GTK_FONT_BUTTON(font_btn), fnt_name);

  PangoFontDescription *font_desc;
 /* Change default font throughout the widget */
  font_desc = pango_font_description_from_string (fnt_name);
  gtk_widget_modify_font (text_view, font_desc);
  pango_font_description_free (font_desc);
  
  font_name.assign(fnt_name);
  g_free(fnt_name);
}

//config dialog
static void
conf_dialog_clicked (GtkButton *btn,
			    gpointer   user_data)
{
  GtkWidget *dialog, *content_area;
  GtkWidget *hbox, *vbox, *button;
  GtkWidget *stock, *radio[14];//1, *radio2, *radio3;
  GtkWidget *table;
  GtkWidget *edcheck, *auto_search;
  //GtkWidget *local_entry2;
  GtkWidget *label, *font_btn;
  gint response, text_or_bg;
  
  std::array<int, 14> vcase_char;
  std::string s2num("0"), case2, case3;

  dialog = gtk_dialog_new_with_buttons("Settings:",
					(GtkWindow*)user_data,//GTK_WINDOW (window),
					(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					GTK_STOCK_OK,
					GTK_RESPONSE_OK,
					"_Cancel",
					GTK_RESPONSE_CANCEL,
					NULL);
  
  gtk_window_set_destroy_with_parent (GTK_WINDOW(dialog), TRUE);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(user_data));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  //gtk_widget_set_can_default (GTK_WIDGET(dialog), TRUE);
  gtk_widget_grab_default (GTK_WIDGET(dialog));
  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
  
  				
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_box_pack_start (GTK_BOX (content_area), vbox, FALSE, FALSE, 0);
  
  //hbox = gtk_hbox_new (FALSE, 8);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  //gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);

  stock = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), stock, FALSE, FALSE, 0);
  
  
  radio[0] = gtk_radio_button_new_with_label_from_widget (nullptr, "PSM");
  //vcase_char.push_back(to_string(0)[0]);
  vcase_char[0] = 0;//s2num[0];
  g_signal_connect (radio[0], "toggled",  G_CALLBACK (toggle_ocr_psm_cb), (gpointer)&(vcase_char[0]));
  gtk_box_pack_start (GTK_BOX (vbox), radio[0], TRUE, TRUE, 2);
  
  
  /* Create a radio button with a label */
  for(int i = 1; i < 14; ++i)
  {
	radio[i] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio[i-1]),
               "PSM");
	//vcase_char.push_back(to_string(i)[0]);
	//s2num = to_string(i);
	vcase_char[i] = i;//s2num[0];
	//cout << vcase_char[i] << "\n";
	g_signal_connect (radio[i], "toggled",  G_CALLBACK (toggle_ocr_psm_cb), (gpointer)&(vcase_char[i]));
   
    gtk_box_pack_start (GTK_BOX (vbox), radio[i], TRUE, TRUE, 2);
   }
               
   /*radio[1] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio[0]), "DC");
   case2 = to_string(2);
   vcase_char.push_back(case2[0]);
   g_signal_connect (radio[1], "toggled",  G_CALLBACK (toggle_ocr_psm), (gpointer)&(vcase_char[0]));
   
   radio[2] = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio[1]), "mV");
   case2 = "3";
   g_signal_connect (radio[2], "toggled",  G_CALLBACK (toggle_ocr_psm), (gpointer)&case2[0]);
   
   gtk_box_pack_start (GTK_BOX (vbox), radio[1], TRUE, TRUE, 2);
   gtk_box_pack_start (GTK_BOX (vbox), radio[2], TRUE, TRUE, 2);*/
   
 /*-----------------------------end of the radio buttons--------------------------*/
 
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

/*----------------------------------------font selection--------------------------------------*/
  response = 0;
  label = gtk_label_new ("Font:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  font_btn = gtk_font_button_new ();

  gtk_font_button_set_font_name(GTK_FONT_BUTTON(font_btn),   &font_name[0]);
  gtk_table_attach_defaults (GTK_TABLE (table), label,    0, 1, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), font_btn, 1, 2, 0, 1);
  g_signal_connect(G_OBJECT(font_btn), "font-set",
                        G_CALLBACK(font_select_cb), &response);
/*-------------------------------------end of font selection--------------------------------------*/

/*----------------------------------------color selection--------------------------------------*/
  text_or_bg = 0;
  label = gtk_label_new ("Text color:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  button = gtk_button_new_with_mnemonic ("_Text color");

  gtk_table_attach_defaults (GTK_TABLE (table), label,    0, 1, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 1, 2);
  //g_signal_connect(G_OBJECT(button), "clicked",
  //                      G_CALLBACK(color_select_cb), GINT_TO_POINTER(text_or_bg));
/*-------------------------------------end of color selection------------------------------------*/

/*----------------------------------------bg color selection--------------------------------------*/
  text_or_bg = 1;
  label = gtk_label_new ("Background color:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  button = gtk_button_new_with_mnemonic ("_Background color");

  gtk_table_attach_defaults (GTK_TABLE (table), label,    0, 1, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 1, 2, 2, 3);
  //g_signal_connect(G_OBJECT(button), "clicked",
  //                      G_CALLBACK(color_select_cb),  GINT_TO_POINTER(text_or_bg));
/*-------------------------------------end of bg color selection-----------------------------------*/


  gtk_widget_show_all (vbox);
  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == GTK_RESPONSE_OK)
    {
      //settings();

    }
    else if(response == GTK_RESPONSE_CANCEL )
    {
        //_auto_search = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( auto_search ) );
    }

  gtk_widget_destroy (dialog);
}



std::string get_run_cmd ()
{
	string cmd;
	cmd  =  pCmd[0]; //ruby executable path
    cmd += " ";
    cmd += pCmd[1]; //card2isis.pl
    //cmd += " temp.txt ";
    /*cmd += " ";
    cmd +=tmpFilePath;
    cmd += "/temp.txt ";
    cmd += filename;*/
    
	return cmd;
}


//convert into text and save it
static void export_img_roi_event( GtkWidget *widget, gpointer data )
{

// gboolean select_multiple;
 GtkWidget *dialog;
 //GtkWidget *window = GTK_WIDGET(data);
 GtkTextBuffer *text_buffer = reinterpret_cast<GtkTextBuffer *>(data);
 char *filename;
 GSList *filenames;

 is_session_restore = false;


  dialog = gtk_file_chooser_dialog_new ("Save OCR data",
				      NULL,//parent_window,
				      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				      NULL);
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);			      

 //gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER (dialog), TRUE);

  //cout << loadFromPath << endl;

  /*if( loadFromPath.size() > 0 )
  {
        gchar *path;
        path = g_path_get_dirname( (const gchar *)&loadFromPath[0] );
        //cout << path << endl;

        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),(const gchar *)path);
        /gtk_file_chooser_select_filename    (GTK_FILE_CHOOSER (dialog),
                                                            (const char *)path); /

        g_free(path);
    }*/


 if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
 {
	 //Image image(&img_file_name[0]);
	 
	 time_t rawTime;
	 gchar buf_tm[20];
	 struct tm *pTime;
	 
	 GError err;
	 GError *pErr = NULL;
	 GUri *gUri = nullptr;
	 GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
	 
	 gchar *filename;
     gchar *tmpFilePath;
	 std::string place_to_save_img_roi, file_name, str, ext;
	 std::string fname;
	 
	 //pErr = &err;

     filename = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));
     
     //gUri = g_uri_parse(filename, G_URI_FLAGS_NONE, &pErr);
     //place_to_save_img_roi = get_path_from_uri(gUri);
     
     //std::cout << pErr->message << "\n";
     
     //filename = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
     //file_name.assign(filename);
     
     tmpFilePath = g_get_current_dir();
     
     place_to_save_img_roi.assign(tmpFilePath);
     place_to_save_img_roi.append("\\cache");
     
     if (is_file_exist(place_to_save_img_roi))
		cout << "cache dir exists.\n";
     else
     {
		cout << "cache dir not exists.\n";
		filesystem::create_directory(place_to_save_img_roi);
	 }
     
     
     g_print("%s\n%s\n", (gchar*)filename, (gchar*)tmpFilePath);
     
     //place_to_save_img_roi.assign(filename);
     
     g_free(filename);
     g_free(tmpFilePath);
      
      if(place_to_save_img_roi.length() > 0)//(num > 1)
      {
		  char *buffer = nullptr;
		  std::string dest;
		  //place_to_save_img_roi = "c:\\home\\tkl\\mine\\pic";
		  
		  time( &rawTime );
		  pTime = localtime( &rawTime );
		  strftime(buf_tm, sizeof(buf_tm), "%Y.%m.%d_%H%M%S", pTime);
		  
		  ext = PathGetFileNameOrExtension ( img_file_name, 1 );
		  //std::cout << "extension:" << ext << " " << fname << std::endl;
		  
		  file_name = PathGetDirSeperator(place_to_save_img_roi);
		  file_name += PathGetFileNameOrExtension ( img_file_name ) + "_" + ext + "_";
		  file_name.append(buf_tm);
		  
		  std::cout << "file name: " << PathGetFileNameOrExtension ( img_file_name ) <<"\n";
		  //static const cairo_user_data_key_t surface_data_key;
		  str = place_to_save_img_roi + file_name + "_";
		  
		  //ext = filesystem::path(img_file_name).extension();//clang libc++ doesn't have it
		  
		  
		  //fstream ftmp(file_name, ios::out);
          //ftmp.close();
    	  
    	  	  
		  //cout << "\n" << "width: " << image.w << " height: " << image.h <<"\n";
		  //cout << "width: " << width << " height: " << height <<"\n";
		  
		  //cout << "2.start point: " << start_pos.first << " " << start_pos.second << endl;
	      //cout << "2.wh: " << width << " " << height << endl;
		  
		  if ( array_roi_coords.size() != 0 )
		  {			   
					
			  fstream frio_list_name(pCmd[2], ios::out);
			  
			  for (int i = 0; i <= array_roi_coords.size()-1; ++i)
			  {
				//image.crop(array_roi_coords[i].x-10, array_roi_coords[i].y-10, 
				//array_roi_coords[i].width, array_roi_coords[i].height );
				str += std::to_string(i) + '.' + "png";//ext;
				//image.write(reinterpret_cast<const char*>(str.c_str()));
				std::cout << str << "\n";
				frio_list_name << str << endl;
				save_pics_in_cache_dir (str, array_roi_coords[i]);
				str = place_to_save_img_roi + file_name + "_";
			   }
			  frio_list_name.close();
			  
			  //list_roi.txt file to the place where ruby script located
			  if ( is_file_exist( pCmd[2] ) )
			  {
				fname.assign(currDirPath);  
				fname += PathGetDirSeperator(pCmd[1]);
				
				fname += pCmd[2]; /* /list_roi.txt (/) - path delimiter */
				dest = PathGetDir (pCmd[1]); //destination path
				
				cout << "copying: " << dest << " " << fname << endl;
				
				copy_file_to_dest(fname, dest);
			  }
			  else
			  {
				  glob.err_msg = "There happened some mulfunctions: couldn't find list_roi.txt file.";
				  cout << glob.err_msg << endl;
				  Print_Message(&glob.err_msg[0], text_buffer, 1);
				  goto leave_gracefully;
			  }
		  }
		  
		  //run ocr_converter_utility.rb ruby script
		  std::string cmd = get_run_cmd ();
		  cout << "ruby script run: " << cmd << endl;
		  //VImage::system(&cmd[0]);
		  system(&cmd[0]);
		  
		  
		  fname = dest;
		  fname += PathGetDirSeperator(pCmd[1]);
		  fname += "ocr_file.txt";

		  if ( g_file_test (&fname[0], G_FILE_TEST_EXISTS) )//is_file_exist(fname))
		  {
			  
			  //str = "Image Converted!";
			  glob.is_converted = true;
			  buffer = file_io(&fname[0]);
			  str.assign ( buffer );
			  delete [] buffer;
			  Print_Message(&str[0], text_buffer);
		  }
		  else
		  {
			  glob.is_converted = false;
			  str = "Image Not Converted!";
			  Print_Message(&str[0], text_buffer);
			  //dest.assign (currDirPath);
			  //copy_file_to_dest (fname, dest);
		  }
      }
      else
      {
          //cout << "junk:" << junkFile << endl;
      }
   }

leave_gracefully:
    gtk_widget_destroy (dialog);
}


static void show_ocr_file_cb( GtkWidget *widget, gpointer data )
{
	GtkTextBuffer *text_buffer = reinterpret_cast<GtkTextBuffer *>(data);
	std::string str, fname, dest;
	
	fname = PathGetDir (pCmd[1]);
	fname += PathGetDirSeperator(pCmd[1]);
	fname += "ocr_file.txt";
	
	str.assign ( file_io(&fname[0]) );
	Print_Message(&str[0], text_buffer);
}


static gboolean focus_out_event( GtkWidget      *widget,
                                    GdkEvent      *event,
                                    gpointer       data)
{
   //cout << "text buffer view" << endl;
   glob.focus_in_text = false;
   return false;
}

static gboolean focus_in_event( GtkWidget      *widget,
                                    GdkEvent      *event,
                                    gpointer       data)
{
   //cout << "text buffer view" << endl;
   glob.focus_in_text = true;
   return false;
}

void
text_buffer_changed_event_cb (GtkTextBuffer *textbuffer,
								gpointer       user_data)
{
	if (glob.focus_in_text)
		glob.is_text_buffer_content_changed = true;
}

static void app_activate(GtkApplication* app, gpointer user_data)
{
	//Image image(&img_file_name[0]);
    GtkWidget *window = gtk_application_window_new(app);
    GtkWidget *button;
    //GtkWidget *darea;
    GtkWidget *main_vbox, *main_hbox, *hbox;
    //GtkWidget *scrolled_window, *scrolled_window2;
    GtkWidget *frame;
    GtkWidget *vpaned;
    GtkWidget *grid;
    GtkWidget *hscrollbar;
    GtkWidget *vscrollbar;
    GtkAdjustment *hadjustment;
    GtkAdjustment *vadjustment;
    //GtkWidget *text_view;
    GtkTextBuffer *text_buffer;
  
    GtkWidget *ctrl_frame;
    //GtkWidget *load_pics_button, *save_pics_button;
    
    glob.timer  = TRUE;
    glob.alpha  = 1.0;
    //glob.size = 1.0;
    glob.factor = 1;
    glob.image  = nullptr;
    
    //img_file_name.assign( "pic.png" );
    
    //image.read(&img_file_name[0]);
    
    //glob.image = cairo_image_surface_create_from_png( &img_file_name[0] );
    
    //cairo_format_t format = cairo_image_surface_get_format(glob.image);
	//cout << "0.surface format: " << format << endl;
    
    //main_hbox = gtk_hbox_new (FALSE, 0);
    
    scrolled_window = gtk_scrolled_window_new( NULL, NULL );
	//gtk_container_add( GTK_CONTAINER( window ), scrolled_window );
    
    main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    /* Put the box into the main window. */
    //gtk_container_add (GTK_CONTAINER (main_vbox), main_hbox);
    gtk_container_add (GTK_CONTAINER (window), main_vbox);
    
    main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add (GTK_CONTAINER (main_vbox), main_hbox);
    
    vpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_size_request (vpaned, 100, -1);
    gtk_box_pack_start (GTK_BOX (main_hbox), vpaned, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER(vpaned), 5);
    gtk_paned_set_position (GTK_PANED (vpaned), -1);

    
    darea = gtk_drawing_area_new();
    
    if( !(vips_glob.image = vipsdisp_load(&img_file_name[0]) ) )
			vips_error( "unable to load %s", &img_file_name[0] );
    
    if ( !(vips_glob.display = vipsdisp_display_image( vips_glob.image, darea ))  ||
		!(vips_glob.region = vips_region_new( vips_glob.display )) )
		vips_error_exit( "unable to build display image" );
    
    
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_paned_pack1 (GTK_PANED (vpaned), frame, TRUE, FALSE);
    gtk_widget_set_size_request (frame, 60, -1);
    
    
    gtk_widget_set_size_request(darea, 3000, 2000);
	//gtk_widget_set_size_request( darea, vips_glob.display->Xsize, vips_glob.display->Ysize );
    //gtk_container_add( GTK_CONTAINER( scrolled_window ), darea );
    //old gtk_container_add(GTK_CONTAINER (window), darea);
    gtk_container_add(GTK_CONTAINER (frame), scrolled_window);
    //gtk_container_add(GTK_CONTAINER (main_hbox), scrolled_window);
    
    
    grid = gtk_grid_new();
    //darea = gtk_drawing_area_new();
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);
    /*hadjustment = gtk_adjustment_new(0.0, 0.0, 200.0, 1.0, 10.0, 100.0);
    vadjustment = gtk_adjustment_new(0.0, 0.0, 200.0, 1.0, 10.0, 100.0);
    hscrollbar = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, hadjustment);
    vscrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, vadjustment);
    gtk_widget_set_hexpand(hscrollbar, TRUE);
    gtk_widget_set_vexpand(vscrollbar, TRUE);*/
    gtk_grid_attach(GTK_GRID (grid), darea, 0, 0, 1, 1);
    /*gtk_grid_attach(GTK_GRID (grid), vscrollbar, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID (grid), hscrollbar, 0, 1, 1, 1);
    
    g_signal_connect (hscrollbar, "size-allocate",
                    G_CALLBACK (size_allocate_cb), hadjustment);
    g_signal_connect (hscrollbar, "change-value",
                    G_CALLBACK (change_value_cb), NULL);*/

    
    //gtk_container_add(GTK_CONTAINER (main_vbox), grid);
    gtk_container_add(GTK_CONTAINER (scrolled_window), grid);
    gtk_widget_grab_focus (scrolled_window);
    
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_paned_pack2 (GTK_PANED (vpaned), frame, TRUE, FALSE);
    gtk_widget_set_size_request (frame, 50, -1);
    
    
    text_view = gtk_text_view_new ();
	gtk_text_view_set_justification(GTK_TEXT_VIEW (text_view ),  GTK_JUSTIFY_LEFT);
	gtk_text_view_set_editable( GTK_TEXT_VIEW (text_view ), true);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text_view ), GTK_WRAP_WORD_CHAR);
	gtk_widget_set_size_request (text_view, -1, 1000);
	
	g_signal_connect (G_OBJECT (text_view), "focus-in-event",
                     G_CALLBACK (focus_in_event), NULL);
    g_signal_connect (G_OBJECT (text_view), "focus-out-event",
                     G_CALLBACK (focus_out_event), NULL);

	//connect buffer with view
	text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

	/*gtk_text_buffer_create_tag (text_buffer, "gray_foreground",
											  "foreground", "gray", NULL);*/
	gtk_text_buffer_create_tag (text_buffer, "blue_foreground",
											  "foreground", "blue", NULL);
	gtk_text_buffer_create_tag (text_buffer, "red_foreground",
											  "foreground", "red", NULL);
	gtk_text_buffer_create_tag (text_buffer, "green_foreground",
											  "foreground", "green", NULL);
	gtk_text_buffer_create_tag (text_buffer, "blue_background",
											  "background", "blue", NULL);
	/*gtk_text_buffer_create_tag (text_buffer, "gray_background",
											  "background", "gray", NULL);*/
	gtk_text_buffer_create_tag (text_buffer, "white_background",
											  "background", "white", NULL);
	g_signal_connect (G_OBJECT (text_buffer), "changed",
		    G_CALLBACK (text_buffer_changed_event_cb), window);

    scrolled_window2 = gtk_scrolled_window_new( NULL, NULL );
    
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window2),
		   	                        GTK_POLICY_AUTOMATIC,
				                    GTK_POLICY_AUTOMATIC);
				                    
	gtk_container_add(GTK_CONTAINER (frame), scrolled_window2);

    gtk_container_add (GTK_CONTAINER (scrolled_window2), text_view);
    
    ctrl_frame = gtk_frame_new ("Controls");
    gtk_box_pack_start (GTK_BOX (main_vbox),ctrl_frame,FALSE, FALSE, 10);
    
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_container_add (GTK_CONTAINER (ctrl_frame), hbox);
    
    
   /*-----------------------Load cards --------------------------------- */
   //Mnemonics for acceleration of access
   load_pics_button = gtk_button_new_with_mnemonic("Юкла F3");//("_Load cards");
   g_signal_connect (G_OBJECT (load_pics_button), "clicked",
		    G_CALLBACK (load_pics_event), window);

    gtk_box_pack_start (GTK_BOX (hbox), load_pics_button, FALSE, FALSE, 10);
   /*---------------------------------------------------------------------*/
    
    /*-----------------------Save cards --------------------------------- */
   //Mnemonics for acceleration of access
   save_pics_button = gtk_button_new_with_mnemonic("Сакла F2");//("_Load cards");
   g_signal_connect (G_OBJECT (save_pics_button), "clicked",
		    G_CALLBACK (save_pics_event), window);

    gtk_box_pack_start (GTK_BOX (hbox), save_pics_button, FALSE, FALSE, 10);
   /*---------------------------------------------------------------------*/
   
   /*export roi image files*/
   convert_pics_to_text_btn = gtk_button_new_with_mnemonic("Матнга угир F9");//("_Load cards");
   g_signal_connect (G_OBJECT (convert_pics_to_text_btn), "clicked",
		    G_CALLBACK (export_img_roi_event), text_buffer);//window);

    gtk_box_pack_start (GTK_BOX (hbox), convert_pics_to_text_btn, FALSE, FALSE, 10);
   /*------------------------------------------------------------------------*/
   
   /*------------------------------show ocr file content-------------------------*/
	show_ocr_btn = gtk_button_new_with_label ("Матнни кўрсат F5");
    g_signal_connect (show_ocr_btn, "clicked",
			G_CALLBACK (show_ocr_file_cb), text_buffer);
    gtk_box_pack_start (GTK_BOX (hbox), show_ocr_btn, FALSE, FALSE, 10);
   /*----------------------------------------------------------------------------*/
   
   /*---------------------------------settings dialog-------------------------*/
	//button = gtk_button_new_from_stock(GTK_STOCK_PREFERENCES);
	settings_btn = gtk_button_new_with_mnemonic ("_Preferences F7");
    g_signal_connect (settings_btn, "clicked",
			G_CALLBACK (conf_dialog_clicked), GTK_WINDOW(window));
    gtk_box_pack_start (GTK_BOX (hbox), settings_btn, FALSE, FALSE, 10);
   /*---------------------------------settings dialog-------------------------*/
   
   /*------------------------------------------------------------------------*/
	aboutus_btn = gtk_button_new_with_mnemonic("_About");
	g_signal_connect (aboutus_btn, "clicked", G_CALLBACK (about_us), GTK_WINDOW(window));
	gtk_box_pack_start (GTK_BOX (hbox), aboutus_btn, FALSE, FALSE, 1);
   /*------------------------------------------------------------------------*/
   
   kdb_handler_id = g_signal_connect/*object*/ (G_OBJECT (window),
                             "key-press-event",
                             G_CALLBACK (kbd_press_event),
                             scrolled_window);
 

    g_signal_connect(G_OBJECT(darea), "draw", G_CALLBACK(on_draw_event), vips_glob.region);
    
    //g_signal_connect (G_OBJECT (window), "motion-notify-event",G_CALLBACK (mouse_moved), NULL);
    g_signal_connect (G_OBJECT (darea), "motion-notify-event",G_CALLBACK (mouse_moved), NULL);
    g_signal_connect (G_OBJECT (darea), "button-press-event",G_CALLBACK (on_mouse_button_clicked), NULL);
    g_signal_connect (G_OBJECT (darea), "button-release-event",G_CALLBACK (on_mouse_button_released), NULL);

    gtk_widget_set_events(darea, GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    
    //timer_id = g_timeout_add(333, (GSourceFunc) time_handler, (gpointer) grid);
    
	myPixbuf = gdk_pixbuf_from_pixdata(&GTesterLogo, FALSE, NULL);
	
    gtk_window_set_title(GTK_WINDOW(window), "GArchiver OCR utility 0.0.1");
    //gtk_window_set_default_size(GTK_WINDOW(window), 500, 500);
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
    gtk_window_maximize(GTK_WINDOW(window));
    
	globalInit ();
		
	//enable_all_controlls = FALSE;
		
	gtk_widget_set_sensitive(save_pics_button, enable_all_controlls);
	gtk_widget_set_sensitive(load_pics_button, enable_all_controlls);
	gtk_widget_set_sensitive(convert_pics_to_text_btn, enable_all_controlls);
	gtk_widget_set_sensitive(show_ocr_btn, enable_all_controlls);
		
		
	if (glob.err_msg.size() != 0 )//!enable_all_controlls)
	{
		/*Print_Message(" config.txt file not found!\n\
		*** > THE APPLICATION WILL NOT WORK < *** !!!\n\
		create config.txt with the exact path of\n\
		\"ruby\" and \"ocr_script.rb\"\
		\n\
		e.g: \(both lines must be in \"config.txt\" \)\n\
		\c:\\Program Files\\ruby\\bin\\ruby.exe\n\
		c:\\the\\path\\where\\located\\an\\ocr_script.rb\n\
		\n***or some nescarrey files not on found!***",
		text_buffer, 1);*/
		Print_Message (&glob.err_msg[0], text_buffer, 1);
		
	}
	
	gtk_widget_show_all(window);
}


static void app_shutdown(GtkApplication* app, gpointer user_data)
{
  globalDone ();
  //cairo_surface_destroy(glob.image);
  if (timer_id  != 0 ) 
	  g_source_remove(timer_id);
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;
    
    //globalInit ();

    app = gtk_application_new("com.gtk.window", G_APPLICATION_FLAGS_NONE);
    
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
    g_signal_connect(app, "shutdown", G_CALLBACK(app_shutdown), NULL);
    
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
//gcc -Wall -O -o example example.c `pkg-config --cflags --libs gtk+-3.0`
