// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit, all rights reserved.
//***************************************************************************
// File name: vp.cpp
//
// Class definitions: none
//
// Classes referenced:
//   Various BLITZ templates
//   Any classes in global_definitions_vp.h
//   Plot_Window -- Plot window
//   Control_Panel_Window -- Control panel window
//   Data_File_Manager -- Manage data files
//
// Required packages
//    FLTK 1.1.6 -- Fast Light Toolkit graphics package
//    FLEWS 0.3 -- Extensions to FLTK 
//    OGLEXP 1.2.2 -- Access to OpenGL extension under Windows
//    GSL 1.6 -- Gnu Scientific Library package for Windows
//    Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require -D__WIN32__ to compile on windows
//   See Makefile for linux and OSX compile & link info.
//
// Purpose: viewpoints - interactive linked scatterplots and more.
//
// General design philosophy:
//   1) This code is represents a battle between Creon Levit's passion for 
//      speed and efficiency and Paul Gazis's obsession with organization and 
//      clarity, unified by a shared desire to produce a powerful and easy to 
//      use tool for exploratory data analysis.  Creon's code reflects a 
//      strong 'C' heritage.  Paul's code is written in C++ using the 'if 
//      only it were JAVA' programming style.
//
// Functions:
//   usage() -- Print help information
//   make_help_about_window( *o) -- Draw the 'About' window
//   create_main_control_panel( main_x, main_y, main_w, main_h, cWindowLabel) 
//     -- Create the main control panel window.
//   create_broadcast_group() -- Create special panel under tabs
//   manage_plot_window_array( *o) -- Manage plot window array
//   make_main_menu_bar() -- Create main menu bar (unused)
//   make_global_widgets() -- Controls for main control panel
//   choose_color_deselected( *o) -- Color of nonselected points
//   change_all_axes( *o) -- Change all axes
//   clearAlphaPlanes() -- Clear alpha planes
//   npoints_changed( *o) -- Update number of points changed
//   write_data( *o, *user_data) -- Write data widget
//   reset_all_plots( void) -- Reset all plots
//   reload_Plot_Window_array( *o) -- Reload plot windows
//   read_data( *o, *user_data) -- Read data widget
//   redraw_if_changing( *dummy) -- Redraw changing plots
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  10-NOV-2006
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals, and turn on initializers (using #define EXTERN)
// initialize globals
#define EXTERN
#define INIT(x) = x
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "plot_window.h"
#include "control_panel_window.h"

// Define and initialize number of screens
static int number_of_screens = 0;

// Approximate values of window manager borders & desktop borders (stay out of
// these).  These are used by the main method when the main control panel 
// window is defined.  And when the plot windows are tiled to fit the screen.
// Too bad they are only "hints" according most window managers (and we all 
// know how well managers take hints).
#ifdef __APPLE__
 static int top_frame=35, bottom_frame=0, left_frame=0, right_frame=5;
 static int top_safe = 1, bottom_safe=5, left_safe=5, right_safe=1;
#else // __APPLE__
 static int top_frame=25, bottom_frame=5, left_frame=4, right_frame=5;
 static int top_safe = 1, bottom_safe=10, left_safe=10, right_safe=1;
#endif // __APPLE__

// These are needed to pass to manage_plot_window_array
static int global_argc;
static char **global_argv;

// Define and set default border style for the plot windows
static int borderless=0;  // By default, use window manager borders

// Needed to track position in help window
static int help_topline;

// Define variables to hold main control panel window, tabs widget, and 
// virtual control panel positions.  Consolidated here for reasons of clarity.

// Increase this when the main panel needs to get wider:
static const int main_w = 350; 			

// Increase this when the main panel needs to get taller, including situations
// when cp_widget_h increases:
static const int main_h = 750;			

// Increase this when the controls for individual windows need more height to 
// fit in their subpanel
// static const int cp_widget_h = 505; 
static const int cp_widget_h = 525; 

// The rest of these should not have to change
static const int tabs_widget_h = cp_widget_h+20;
static const int global_widgets_y = tabs_widget_h+20;
static const int tabs_widget_x = 3, tabs_widget_y = 30;
static const int cp_widget_x = 3, cp_widget_y = tabs_widget_y+20;
static const int global_widgets_x = 10;

// Define class to hold data file manager
Data_File_Manager dfm;

// Define pointers to hold main control panel, main menu bar, and any pop-up 
// windows.  NOTE: help_view_widget must be defined here so it will be 
// available to callback functions
Fl_Window *main_control_panel;
Fl_Menu_Bar *main_menu_bar;
Fl_Window *about_window;
Fl_Window *help_view_window;
Fl_Help_View *help_view_widget;

// Function definitions for the main method
void usage();
void make_help_about_window( Fl_Widget *o);
void create_main_control_panel( 
  int main_x, int main_y, int main_w, int main_h, char* cWindowLabel);
void create_broadcast_group();
void manage_plot_window_array( Fl_Widget *o);
void make_main_menu_bar();
void make_help_view_window( Fl_Widget *o);
void close_help_window( Fl_Widget *o, void* user_data);
void step_help_view_widget( Fl_Widget *o, void* user_data);
void make_global_widgets();
void choose_color_deselected( Fl_Widget *o);
void change_all_axes( Fl_Widget *o);
void clearAlphaPlanes();
void resize_selection_index_arrays( int nplots_old, int nplots);
void npoints_changed( Fl_Widget *o);
void write_data( Fl_Widget *o, void* user_data);
void reset_all_plots( void);
void read_data( Fl_Widget* o, void* user_data);
void redraw_if_changing( void *dummy);

//***************************************************************************
// usage() -- Print help information to the console and exit.  NOTE: Problems 
// may arise when window is too narrow.
void usage()
{
  cerr << endl;
  cerr << "Usage: vp {optional arguments} {optional filename}" << endl;
  cerr << endl;
  cerr << "Optional arguments:" << endl;
  cerr << "  -b, --borderless            "
       << "don't show decorations on plot windows" << endl;
  cerr << "  -c, --cols=NCOLS            "
       << "startup showing this many columns of plot windows, default=2" << endl;
  cerr << "  -d, --delimiter=CHAR        "
       << "interpret CHAR as field separator, default is whitespace" << endl;
  cerr << "  -f, --format={ascii,binary} "
       << "input file format, default=ascii" << endl;
  cerr << "  -i, --input_file=FILENAME   "
       << "read input data from FILENAME" << endl;
  cerr << "  -m, --monitors=NSCREENS     "
       << "try and force output to display across NSCREENS screens if available" << endl;
  cerr << "  -M, --missing_values=NUMBER "
       << "set the value of any unreadable, nonnumeric, empty, or missing values to NUMBER, default=0.0" << endl;
  cerr << "  -n, --npoints=NPOINTS       "
       << "read at most NPOINTS from input file, default is min(until_EOF, 2000000)" << endl;
  cerr << "  -o, --ordering={rowmajor,columnmajor} "
       << "ordering for binary data, default=columnmajor" << endl;
  cerr << "  -r, --rows=NROWS            "
       << "startup showing this many rows of plot windows, default=2" << endl;
  cerr << "  -s, --skip_header_lines=NLINES "
       << "skip over NLINES lines at start of input file, default=0" << endl;
  cerr << "  -v, --nvars=NVARS           "
       << "input has NVARS values per point (only for row major binary data)" << endl;
  cerr << "  -h, --help                  "
       << "display this message and then exit" << endl;
  cerr << "  -V, --version               "
       << "output version information and then exit" << endl;

  exit( -1);
}

//***************************************************************************
// make_help_about_window( *o) -- Create the 'Help|About' window.
void make_help_about_window( Fl_Widget *o)
{
  if( about_window != NULL) about_window->hide();
   
  // Create Help|About window
  Fl::scheme( "plastic");  // optional
  about_window = new Fl_Window( 300, 200, "About vp");
  about_window->begin();
  about_window->selection_color( FL_BLUE);
  about_window->labelsize( 10);
  
  string sAbout = "viewpoints $Rev$\n";
  sAbout += "(c) 2006 C. Levit and P. R. Gazis\n\n";
  sAbout += "contact information:\n";
  sAbout += " Creon Levit creon.levit@@nasa.gov\n";
  sAbout += " Paul R Gazis pgazis@@mail.arc.nasa.gov\n\n";

  // Write text to box label and align it inside box
  Fl_Box* output_box = new Fl_Box( 5, 5, 290, 160);
  output_box->box(FL_SHADOW_BOX);
  output_box->color(7);
  output_box->selection_color(52);
  output_box->labelfont(FL_HELVETICA);
  output_box->labelsize(15);
  output_box->align(FL_ALIGN_TOP|FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
  output_box->copy_label(sAbout.c_str());

  // Invoke universal callback function to close window
  Fl_Button* close = new Fl_Button( 200, 170, 60, 25, "&Close");
  close->callback( (Fl_Callback*) close_help_window, about_window);

  // Done creating the 'Help|About' window
  about_window->resizable( about_window);
  about_window->end();
  about_window->show();
}

//***************************************************************************
// create_main_control_panel( main_x, main_y, main_w, main_h, cWindowLabel) 
// -- Create the main control panel window.
void create_main_control_panel( 
  int main_x, int main_y, int main_w, int main_h, char* cWindowLabel)
{
  // Create main control panel window
  Fl::scheme( "plastic");  // optional
  main_control_panel = 
    new Fl_Window( main_x, main_y, main_w, main_h, cWindowLabel);
  main_control_panel->resizable( main_control_panel);

  // Make main menu bar and add the global widgets to control panel
  make_main_menu_bar();
  make_global_widgets ();

  // Inside the main control panel, there is a tab widget, cpt, 
  // that contains the sub-panels (groups), one per plot.
  cpt = 
    new Fl_Tabs( tabs_widget_x, tabs_widget_y, main_w-6, tabs_widget_h);
  cpt->selection_color( FL_BLUE);
  cpt->labelsize( 10);

  // Done creating main control panel (except for the tabbed 
  // sub-panels created by manage_plot_window_array)
  main_control_panel->end();
}

//***************************************************************************
// create_broadcast_group () -- Create a special panel (really a group under 
// a tab) with label "+" this group's widgets effect all the others (unless 
// a plot's tab is "locked" - TBI).  MCL XXX should this be a method of 
// Control_Panel_Window or should it be a singleton?
void create_broadcast_group ()
{
  Fl_Group::current(cpt);  
  Control_Panel_Window *cp = cps[nplots];
  cp = new Control_Panel_Window( cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
  cp->label("all");
  cp->labelsize( 10);
  cp->resizable( cp);
  cp->make_widgets( cp);
  cp->end();

  // this group's index is highest (and it has no associated plot window)
  cp->index = nplots;

  // this group's callbacks all broadcast any "event" to the other 
  // (unlocked) tabs groups.  with a few exceptions... (for now)
  for (int i=0; i<cp->children(); i++) {
    Fl_Widget *wp = cp->child(i);
    wp->callback( (Fl_Callback *)(Control_Panel_Window::broadcast_change), cp);
  }

  // MCL XXX these widgets cause crashes or misbehaviors in the global 
  // panel, so skip them for now.
  cp->choose_selection_color_button->deactivate();
  cp->sum_vs_difference->deactivate();
  cp->polar->deactivate();
  cp->no_transform->deactivate();

  // Initially, this group has no axes (XXX or anything else, for that matter)
  cp->varindex1->value(nvars);  // initially == "-nothing-"
  cp->varindex2->value(nvars);  // initially == "-nothing-"
  cp->varindex3->value(nvars);  // initially == "-nothing-"
}

//***************************************************************************
// manage_plot_window_array( o) -- General-purpose method to create, manage, 
// and reload the plot window array.  It saves any existing axis information, 
// deletes old tabs, creates new tabs, restores existing axis information, 
// and loads new data into new plot windows.  There are four possible 
// behaviors, which all must be recognized, identified, and treated 
// differently:
// 1) Initialization -- NULL argument.  Set nplots_old = 0.  
// 2) New data -- Called from button or menu.  Set nplots_old = 0.
// 3) Resize operation -- Called from menu.  Set nplots_old = 
//    nplots, then calculate a new value of nplots.  
// 4) Reload operation -- Called from button or menu.  Keep 
//    nplots = nplots_old.  
// NOTE: Little attempt has been made to optimize this method for speed.  
// WARNINGS: 1) Tbis method is delicate, and slight changes in the FLTK calls 
// could lead to elusive segmentation faults!  Test any changes carefully!  
// 2) There is little protection against missing data!
void manage_plot_window_array( Fl_Widget *o)
{
  // Define an enumeration to hold a list of operation types
  enum operationType { INITIALIZE = 0, NEW_DATA, RESIZE, RELOAD};
  
  // Define and initialize the operationType switch, old number of plots,
  // widget title, and pointers to the pMenu_ and pButton objects.
  operationType thisOperation = INITIALIZE;
  int nplots_old = nplots;
  char widgetTitle[ 80];
  strcpy( widgetTitle, "");
  Fl_Menu_* pMenu_;
  Fl_Button* pButton;

  // Determine how the method was invoked, and set flags and parameters
  // accordingly.  If method was called with a NULL arguments, assume this 
  // is an initialization operation and set the old number of plots to zero, 
  // otherwise identify the argument type via a dynamic cast, extract the 
  // widget title, and set the old and new numbers of plots accordingly.
  // CASE 1: If the widget was NULL, this is an initialzation operation
  if( o == NULL) {
    thisOperation = INITIALIZE;
    nplots_old = 0;
  }

  // CASE 2: If this was an Fl_Menu_ widget, default to a resize operation,
  // then figure out what operation was requested and revise the switches
  // and array descriptions accordingly
  else if( (pMenu_ = dynamic_cast <Fl_Menu_*> (o))) {
    thisOperation = RESIZE;
    nplots_old = nplots;

    strcpy( widgetTitle, ((Fl_Menu_*) o)->text());
    if( strncmp( widgetTitle, "Add Row ", 8) == 0) nrows++;
    else if( strncmp( widgetTitle, "Add Colu", 8) == 0) ncols++;
    else if( strncmp( widgetTitle, "Remove R", 8) == 0 && nrows>1) nrows--;
    else if( strncmp( widgetTitle, "Remove C", 8) == 0 && ncols>1) ncols--;

    // When reading new data, invoke Fl_Gl_Window.hide() (instead of the 
    // destructor!) to destroy all plot windows along with their context, 
    // including VBOs
    if( strncmp( widgetTitle, "Read", 4) == 0) {
      thisOperation = NEW_DATA;
      nplots_old = 0;
      for( int i=0; i<nplots; i++) pws[i]->hide();
    }
  }

  // CASE 3: If this was a button widget, assume it was a reload operation,
  // since no other buttons can invoke this method
  else if( (pButton = dynamic_cast <Fl_Button*> (o))) {
    thisOperation = RELOAD;
    nplots_old = nplots;
    strcpy( widgetTitle, ((Fl_Menu_*) o)->label());
  }

  // DEFAULT: Default to a reload operation
  else {
    thisOperation = RELOAD;
    nplots_old = nplots;
  }
  
  // Recalculate number of plots
  nplots = nrows * ncols;

  // If this was a resise operation, resize the selection arrays:
  // 'indices_selected' and 'number_selected'.
  if( thisOperation == RESIZE)
    resize_selection_index_arrays( nplots_old, nplots);

  // Always save old variable indices and normalization styles, if any.  
  // QUESTION: are these array declarations safe on all compilers when 
  // nplots_old = 0?
  int ivar_old[ nplots_old];
  int jvar_old[ nplots_old];
  int kvar_old[ nplots_old];
  int x_normalization_style_old[ nplots_old];
  int y_normalization_style_old[ nplots_old];
  int z_normalization_style_old[ nplots_old];
  int x_axis_locked[ nplots_old];
  int y_axis_locked[ nplots_old];
  int z_axis_locked[ nplots_old];
  for( int i=0; i<nplots_old; i++) {
    ivar_old[ i] = cps[i]->varindex1->value();
    jvar_old[ i] = cps[i]->varindex2->value();
    kvar_old[ i] = cps[i]->varindex3->value();
    x_normalization_style_old[ i] = cps[i]->x_normalization_style->value();
    y_normalization_style_old[ i] = cps[i]->y_normalization_style->value();
    z_normalization_style_old[ i] = cps[i]->z_normalization_style->value();
    x_axis_locked[ i] = cps[i]->lock_axis1_button->value();
    y_axis_locked[ i] = cps[i]->lock_axis2_button->value();
    z_axis_locked[ i] = cps[i]->lock_axis3_button->value();
  }
  
  // Clear children of the tab widget to delete old tabs
  cpt->clear();

  // Create and add the virtual sub-panels, each group under a tab, one
  // group per plot.
  for( int i=0; i<nplots; i++) {
    int row = i/ncols;
    int col = i%ncols;

    // Account for the 'borderless' option
    if( borderless)
      top_frame = bottom_frame = left_frame = right_frame = 1;

    // Determine plot window size and position
    int pw_w =
      ( ( number_of_screens*Fl::w() - 
          (main_w+left_frame+right_frame+right_safe+left_safe+20)) / ncols) -
      (left_frame + right_frame);
    int pw_h = 
      ( (Fl::h() - (top_safe+bottom_safe))/ nrows) - 
      (top_frame + bottom_frame);

    int pw_x = 
      left_safe + left_frame + 
      col * (pw_w + left_frame + right_frame);
    int pw_y = 
      top_safe + top_frame + 
      row * (pw_h + top_frame + bottom_frame);

    // Create a label for this tab
    ostringstream oss;
    oss << "" << i+1;
    string labstr = oss.str();

    // Set the pointer to the current group to the tab widget defined by
    // create_control_panel and add a new virtual control panel under this
    // tab widget
    Fl_Group::current( cpt);  
    cps[i] = new Control_Panel_Window( 
      cp_widget_x, cp_widget_y, main_w - 6, cp_widget_h);
    cps[i]->index = i;
    cps[i]->copy_label( labstr.c_str());
    cps[i]->labelsize( 10);
    cps[i]->resizable( cps[i]);
    cps[i]->make_widgets( cps[i]);

    // End the group here so that we can create new plot windows at the top
    // level, then set the pointer to the current group to the top level.
    cps[i]->end();
    Fl_Group::current( 0); 

    // If this was an initialize, resize, or new_data operation, then create 
    // or restore the relevant windows.  NOTE: If this code was executed 
    // during a reload operation, it would cause a segmentation fault due to
    // problems with the way shown() and hide() work.
    if( thisOperation == INITIALIZE || 
        thisOperation == RESIZE || 
        thisOperation == NEW_DATA) {
      if( i >= nplots_old) {
        pws[i] = new Plot_Window( pw_w, pw_h, i);
        cps[i]->pw = pws[i];
        pws[i]->cp = cps[i];
      }
      else {
        pws[i]->index = i;
        cps[i]->pw = pws[i];
        pws[i]->cp = cps[i];
        pws[i]->size( pw_w, pw_h);
      }
      pws[i]->copy_label( labstr.c_str());
      pws[i]->position(pw_x, pw_y);
      pws[i]->row = row; 
      pws[i]->column = col;
      pws[i]->end();
    }

    // Always link the plot window and its associated virtual control panel
    assert ((pws[i]->index == i) && (cps[i]->index == i));
    cps[i]->pw = pws[i];
    pws[i]->cp = cps[i];

    // Always invoke Plot_Window::upper_triangle_incr to determine which 
    // variables to plot in new panels.
    int ivar, jvar;
    if( i==0) {
      ivar = 0;
      jvar = 1;
      
      // If this is an initialize operation, then the plot window array is being 
      // created, the tabs should come up free of context.  It also might be
      // desirable that the first plot's tab be shown with its axes locked.
      if( thisOperation == INITIALIZE) {
        cps[i]->hide();  
      }
    }
    else Plot_Window::upper_triangle_incr( ivar, jvar, nvars);

    // If the number of plots has changed, restore the old variable indices and
    // normalization styles for the old panels.  Otherwise set new variable 
    // indices for the new panels    
    if( nplots != nplots_old && i<nplots_old) {
        cps[i]->varindex1->value( ivar_old[i]);  
        cps[i]->varindex2->value( jvar_old[i]);
        cps[i]->varindex3->value( kvar_old[i]);
        cps[i]->x_normalization_style->value( x_normalization_style_old[i]);  
        cps[i]->y_normalization_style->value( y_normalization_style_old[i]);  
        cps[i]->z_normalization_style->value( z_normalization_style_old[i]);  
        cps[i]->lock_axis1_button->value( x_axis_locked[i]);  
        cps[i]->lock_axis2_button->value( y_axis_locked[i]);  
        cps[i]->lock_axis3_button->value( z_axis_locked[i]);  
    } 
    else {
      cps[i]->varindex1->value(ivar);  
      cps[i]->varindex2->value(jvar);  
      cps[i]->varindex3->value(nvars);  
    }

    // If this is an initialization, resize, or new_data operation, test for 
    // missing data, extract data, reset panels, and make them resizable.  
    // Otherwise it must be a reload operation and we must invoke the 
    // relevant Plot_Window member functions to initialize and draw panels.
    if( thisOperation == INITIALIZE || 
        thisOperation == RESIZE ||
        thisOperation == NEW_DATA) {
      if( npoints > 1) {
        pws[i]->extract_data_points();
        pws[i]->reset_view();
      }
      pws[i]->size_range( 10, 10);
      pws[i]->resizable( pws[i]);
    }
    else {
      pws[i]->initialize();
      // pws[i]->color_array_from_selection();  // Not needed here
      pws[i]->extract_data_points();
    }

    // Account for the 'borderless' option
    if( borderless) pws[i]->border(0);

    // Make sure the window has been shown and check again to make absolutely 
    // sure it is resizable.  NOTE: pws[i]->show() with no arguments is not 
    // sufficient when windows are created.
    if( !pws[i]->shown()) pws[i]->show( global_argc, global_argv);
    pws[i]->resizable( pws[i]);

    // Turn on the 'show' capability of Plot_Window::reset_view();
    pws[i]->do_reset_view_with_show = 1;
  }

  // Set the color arrays to make sure points get drawn.
  pws[0]->color_array_from_selection();
  
  // Invoke Fl_Gl_Window::hide() (rather than the destructor, which may
  // produce strange behavior) to rid of any superfluous plot windows
  // along with their contexts.
  if( nplots < nplots_old)
    for( int i=nplots; i<nplots_old; i++) pws[i]->hide();
  
  // Create a master control panel to encompass all the tabs
  create_broadcast_group ();
}

//***************************************************************************
// make_main_menu_bar() -- Make main menu bar.  NOTE: because the FLTK 
// documentation recommends against manipulating the Fl_Menu_Item array 
// directly, this is done via the add() method of Fl_Menu_.
void make_main_menu_bar()
{
  // Instantiate the Fl_Menu_Bar object
  main_menu_bar =
    new Fl_Menu_Bar( 0, 0, main_w, 25);

  // Add File menu items
  main_menu_bar->add( 
    "File/Read ASCII file   ", 0, 
    (Fl_Callback *) read_data, (void*) "ASCII");
  main_menu_bar->add( 
    "File/Read binary file   ", 0, 
    (Fl_Callback *) read_data, (void*) "binary", FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Write ASCII file   ", 0, 
    (Fl_Callback *) write_data, (void*) "ASCII");
  main_menu_bar->add( 
    "File/Write binary file   ", 0, 
    (Fl_Callback *) write_data, (void*) "binary");
  main_menu_bar->add( 
    "File/Write selected ASCII data   ", 0, 
    (Fl_Callback *) write_data, (void*) "selected ASCII");
  main_menu_bar->add( 
    "File/Write selected binary data   ", 0, 
    (Fl_Callback *) write_data, (void*) "selected binary", FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "File/Quit   ", 0, (Fl_Callback *) exit);

  // Add View menu items
  main_menu_bar->add( 
    "View/Add Row   ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Add Column   ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Remove Row   ", 0, 
    (Fl_Callback *) manage_plot_window_array);
  main_menu_bar->add( 
    "View/Remove Column   ", 0, 
    (Fl_Callback *) manage_plot_window_array, 0, FL_MENU_DIVIDER);
  main_menu_bar->add( 
    "View/Reload Plots   ", 0, 
    (Fl_Callback *) manage_plot_window_array);

  // Add Help menu items
  main_menu_bar->add( 
    "Help/Viewpoints Help   ", 0, (Fl_Callback *) make_help_view_window);
  main_menu_bar->add( 
    "Help/About   ", 0, (Fl_Callback *) make_help_about_window);
  
  // Set colors, fonts, etc
  main_menu_bar->color( FL_BACKGROUND_COLOR);
  main_menu_bar->textfont( FL_HELVETICA);
  main_menu_bar->textsize( 14);
  main_menu_bar->down_box( FL_FLAT_BOX);
  main_menu_bar->selection_color( FL_SELECTION_COLOR);
  
  // This example is included to illustrate how awkward it can  be to access 
  // elements of the Fl_Menu_Item array directly.
  // for( int i=0; i<main_menu_bar->size(); i++) {
  //   Fl_Menu_Item *pMenuItem = 
  //     (Fl_Menu_Item*) &(main_menu_bar->menu()[i]);
  //   pMenuItem->labelsize(32);
  // }
}

//***************************************************************************
// make_help_view_window( *o) -- Create the 'Help|Help' window.
void make_help_view_window( Fl_Widget *o)
{
  if( help_view_window != NULL) help_view_window->hide();
  
  // Create Help|Help window
  Fl::scheme( "plastic");  // optional
  help_view_window = new Fl_Window( 600, 400, "Viewpoints Help");
  help_view_window->begin();
  help_view_window->selection_color( FL_BLUE);
  help_view_window->labelsize( 10);

  // Define Fl_Help_View widget
  help_view_widget = new Fl_Help_View( 5, 5, 590, 350, "");
  (void) help_view_widget->load( "vp_help_manual.htm");
  help_view_widget->labelsize( 14);
  help_topline = help_view_widget->topline();
  
  // Invoke callback function to move through help_view widget
  Fl_Button* back = new Fl_Button( 325, 365, 70, 30, "&Back");
  back->callback( (Fl_Callback*) step_help_view_widget, (void*) -60);
  Fl_Button* fwd = new Fl_Button( 400, 365, 70, 30, "&Fwd");
  fwd->callback( (Fl_Callback*) step_help_view_widget, (void*) 60);

  // Invoke callback function to close window
  Fl_Button* close = new Fl_Button( 500, 365, 70, 30, "&Close");
  close->callback( (Fl_Callback*) close_help_window, help_view_window);

  // Done creating the 'Help|Help' window
  help_view_window->resizable( help_view_window);
  help_view_window->end();
  help_view_window->show();
}

//***************************************************************************
// close_help_window( *o, *user_data) -- Close a Help window
void close_help_window( Fl_Widget *o, void* user_data)
{
  // WARNING: No error checking is done on user_data!
  ((Fl_Window*) user_data)->hide();
}

//***************************************************************************
// step_help_view_window( *o, *user_data) -- Step through the 'Help|Help' 
// window.
void step_help_view_widget( Fl_Widget *o, void* user_data)
{
  help_topline += (int) user_data;
  if( help_topline < 0) help_topline=0;
  help_view_widget->topline( help_topline);
}

//***************************************************************************
// make_global_widgets() -- Make controls for main control panel
void make_global_widgets()
{
  int xpos = global_widgets_x, ypos = global_widgets_y;
#if 0
  // Draw 'npoints' horizontal slider at top of subpanel
  // XXX MCL the new point coloring scheme broke the npoints slider :-(
  // the good news is that I'm not sure anyone uses it.
  // the better news is that I think it is only broken for FAST_APPLE_VERTEX_EXTENSIONS
  // but it needs to be fixed anyway, so I've disabled it.
  npoints_slider = 
    new Fl_Hor_Value_Slider_Input( xpos+30, ypos+=25, 300-30, 20, "npts");
  npoints_slider->align( FL_ALIGN_LEFT);
  npoints_slider->callback( npoints_changed);
  npoints_slider->value( npoints);
  npoints_slider->step( 1.0);
  npoints_slider->bounds( 1, npoints);
#endif

  // Define a pointer and initialize positions for buttons
  Fl_Button *b;
  int xpos1 = xpos, ypos1 = ypos;

  // Button(1,1): Show nonselected points (on by default)
  show_deselected_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "show nonselected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON);
  b->value( 1);
  b->callback( (Fl_Callback*) Plot_Window::toggle_display_deselected);

  // Button(2,1): Add to the selection
  add_to_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "add to selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON);
  b->value( 0);  

  // Button(3,1): Invert selected and nonselected data
  invert_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "invert selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*) Plot_Window::invert_selection);

  // Button(4,1): Clear selection
  clear_selection_button = b = 
    new Fl_Button(xpos, ypos+=25, 20, 20, "clear selection");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( Plot_Window::clear_selection);

  // Button(5,1): Delete selected data
  delete_selection_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "kill selected");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( Plot_Window::delete_selection);

  // Advance to column 2
  xpos = xpos1 + 150; ypos = ypos1;

  // Button(1,2): Chose color of non-selcted points
  choose_color_deselected_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "unselected color");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*)choose_color_deselected);

  // Button(3,2): Randomly change all axes
  change_all_axes_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "change axes");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( (Fl_Callback*)change_all_axes);

  // Button(4,2): Link all axes
  link_all_axes_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "link axes");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->type( FL_TOGGLE_BUTTON); 
  b->value( 0);

  // Button(5,2): Reload plot window array
  reload_plot_window_array_button = b = 
    new Fl_Button( xpos, ypos+=25, 20, 20, "reload plots");
  b->align( FL_ALIGN_RIGHT); 
  b->selection_color( FL_BLUE); 
  b->callback( manage_plot_window_array);
}

//***************************************************************************
// choose_color_deselected( *o) -- Choose color of deselected points, update 
// the selection color table and redraw all plots.  NOTE: Could this become a 
// static member function of class Plot_Window?
void choose_color_deselected( Fl_Widget *o)
{
  (void) fl_color_chooser( 
    "deselected", 
    Plot_Window::r_deselected, 
    Plot_Window::g_deselected, 
    Plot_Window::b_deselected);

  // Update selection color table and redraw all plots
  pws[ 0]->update_selection_color_table ();
  Plot_Window::redraw_all_plots (0);
}

//***************************************************************************
// change_all_axes( *o) -- Invoke the change_axes method of each Plot_Window 
// to change all unlocked axes.
void change_all_axes( Fl_Widget *o)
{
  // Loop: Examine successive plots and change the axes of those for which 
  // the x or y axis is unlocked.
  for( int i=0; i<nplots; i++) {
    if( !( cps[i]->lock_axis1_button->value() && 
           cps[i]->lock_axis2_button->value()))
      pws[i]->change_axes( 0);
  }
  Plot_Window::redraw_all_plots(0);
}

//***************************************************************************
// clearAlphaPlanes() -- Those filthy alpha planes!  It seems that no matter 
// how hard you try, you just can't keep them clean!
void clearAlphaPlanes()
{
  glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
  glClearColor( 0.0, 0.0, 0.0, 0.0);
  glClear( GL_COLOR_BUFFER_BIT);
  glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

//***************************************************************************
// npoints_changed( 0) -- Examine slider widget to determine the new number 
// of points, then invoke a static method of class Plot_Window to redraw all 
// plots.
void npoints_changed( Fl_Widget *o) 
{
  npoints = int( ( (Fl_Slider *)o)->value());
  Plot_Window::redraw_all_plots( 0);
}

//***************************************************************************
// resize_selection_index_arrays( nplots_old, nplots) -- Resize the arrays 
// that depend on the value of nplots and initialize any new values of the 
// selection arrays.  This should be called whenever nplots is changed
//
// MCL XXX - this whole function and the globals and public functions it 
// references could go away if we made the globals and public functions 
// into members of class Plot_Window.  However, this requires some way of 
// handling the extra "non-selected" selection, which does not really 
// belong to any one plot window.
void resize_selection_index_arrays( int nplots_old, int nplots)
{
  blitz::Range NPTS( 0, npoints-1);
  pws[0]->indices_selected.resizeAndPreserve(nplots+1,npoints);
  pws[0]->number_selected.resizeAndPreserve(nplots+1);
  for( int i=nplots_old+1; i<nplots+1; i++) {
    pws[0]->indices_selected(i,NPTS) = 0;
    pws[0]->number_selected(i) = 0;
    #ifdef USE_VBO
      pws[i]->initialize_indexVBO(i);
      pws[i]->fill_indexVBO(i);
    #endif // USE_VBO
  }
}

//***************************************************************************
// write_data( o) -- Write data widget.  Invoked by main control panel.  
// Invokes write method to write a binary data file.
void write_data( Fl_Widget *o, void* user_data)
{
  // Evaluate user_data to get ASCII or binary file format
  if( strstr( (char *) user_data, "binary") != NULL) dfm.ascii_output( 0);
  else dfm.ascii_output( 1);

  // Evaluate user_data to determine if only selected data are to be used
  if( strstr( (char *) user_data, "selected") != NULL) dfm.selected_data( 1);
  else dfm.selected_data( 0);

  // Query user to find name of output file.  If no file was specified, 
  // return immediately and hope the calling routine can handle this.
  int iQueryStatus = 0;
  iQueryStatus = dfm.findOutputFile();
  if( iQueryStatus != 0) {
    cout << "No output file was selected" << endl;
    return;
  }
  
  // Invoke the data file manager to save the file or fail gracefully
  dfm.save_data_file();
}

//***************************************************************************
// reset_all_plots() -- Reset all plots.  Invoked by main control panel.
void reset_all_plots()
{
  for( int i=0; i<nplots; i++) pws[i]->reset_view();
}

//***************************************************************************
// read_data( o, user_data) -- Widget that invokes the data_file_manager to 
// open and read data from a binary or ASCII file.
void read_data( Fl_Widget* o, void* user_data)
{
  // Evaluate user_data to get file format
  if( strstr( (char *) user_data, "binary") != NULL) dfm.ascii_input( 0);
  else dfm.ascii_input( 1);

  // Query user to find name of the input file.  If no file was specified, 
  // return immediately and hope the calling routine can handle this.
  int iQueryStatus = 0;
  iQueryStatus = dfm.findInputFile();
  if( iQueryStatus != 0) {
    cout << "No input file was selected" << endl;
    return;
  }

  // Invoke the load_data_file() method of the data file manager to read an 
  // ASCII or BINARY file.  Error reporting is handled by the method itself.
  dfm.load_data_file();

  // If only one or fewer records are available then quit before something 
  // terrible happens!
  if( npoints <= 1) {
    cout << "Insufficient data, " << npoints
         << " samples.  Loading default data." << endl;
    dfm.create_default_data( 10);
  }
  else {
    cout << "Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // Resize the slider
  // npoints_slider->bounds(1,npoints);
  // npoints_slider->value(npoints);

  // Fewer points -> bigger starting pointsize
  pointsize = max( 1.0, 6.0 - (int) log10f( (float) npoints));

  // Clear children of tab widget and reload plot window array.
  manage_plot_window_array( o);

  // KLUDGE: Make sure points are drawn in plot windows.  This is now handled 
  // near the end of manage_plot_window_array().
  // pws[ 0]->color_array_from_selection();
  // Plot_Window::redraw_all_plots( 0);  // Probably not needed
}

//***************************************************************************
// redraw_if_changing( dummy) -- Callback function for use by FLTK 
// Fl::add_idle.  When an idle callback occurs, redraw any plot that is 
// spinning or otherwise needs to be redrawn.
void redraw_if_changing( void *dummy)
{
  // DEBUG( cout << "in redraw_if_changing" << endl) ;
  for( int i=0; i<nplots; i++) {
    // DEBUG ( cout << "  i=" << i << ", needs_redraw=" << pws[i]->needs_redraw << endl );
    if( cps[i]->spin->value() || pws[i]->needs_redraw) {
      pws[i]->redraw();
      pws[i]->needs_redraw = 0;
    }
  }
#if 0
  float fps_max = 300.0;
  struct timeval tp;
  static long useconds=0;
  static long seconds=0;

  // has at least 1/fps_max seconds elapsed? (sort of)
  busy:

  (void) gettimeofday(&tp, (struct timezone *)0);
  if( (tp.tv_sec > seconds) || 
      (((float)(tp.tv_usec - useconds)/1000000.0) > 1.0/fps_max)) {
    seconds = tp.tv_sec;
    useconds = tp.tv_usec;
    return;
  }
  else {
       
    // DANGER: Don't monkey with syntax in the call to usleep!
    usleep (1000000/(5*(int)fps_max));
    goto busy;
  }
#else // 0
    Fl::repeat_timeout(0.001, redraw_if_changing);
#endif // 0
  return;
}

//***************************************************************************
// Main routine
//
// Purpose: Driver to run everything.  STEP 1: Read and parse the command 
//  line.  STEP 2: Read the input data file or create a default data set.  
//  STEP 3: Create the main control panel.  STEP 4: Create the plot window 
//  array.  STEP 5: Enter the main execution loop.
//
// Functions:
//   main() -- main routine
//
// Author:   Creon Levit   unknown
// Modified: P. R. Gazis   04-OCT-2006
//***************************************************************************
//***************************************************************************
// Main -- Driver routine
int main( int argc, char **argv)
{
  cout << "vp: Creon Levit's viewpoints" << endl;
  cout << "Revision $Rev$" << endl;

  // STEP 1: Parse the command line
  // cout << "argc<" << argc << ">" << endl;
  // for( int i=0; i<argc; i++) {
  //   cout << "argv[ " << i << "]: <" << argv[ i] << ">" << endl;
  // }

  // Define structure of command-line options
  static struct option long_options[] = {
    { "format", required_argument, 0, 'f'},
    { "npoints", required_argument, 0, 'n'},
    { "nvars", required_argument, 0, 'v'},
    { "skip_header_lines", required_argument, 0, 's'},
    { "ordering", required_argument, 0, 'o'},
    { "rows", required_argument, 0, 'r'},
    { "cols", required_argument, 0, 'c'},
    { "monitors", required_argument, 0, 'm'},
    { "input_file", required_argument, 0, 'i'},
    { "missing_values", required_argument, 0, 'M'},
    { "delimiter", required_argument, 0, 'd'},
    { "borderless", no_argument, 0, 'b'},
    { "help", no_argument, 0, 'h'},
    { "version", no_argument, 0, 'V'},
    { 0, 0, 0, 0}
  };

  // Initialize the data file manager, just in case
  dfm.initialize();

  // Loop: Invoke GETOPT_LONG to parse successive command-line arguments 
  // (Windows version of GETOPT_LONG is implemented in LIBGW32).  NOTES: 1) 
  // The possible options MUST be listed in the call to GETOPT_LONG, 2) This 
  // process does NOT effect arc and argv in any way.
  int c;
  string inFileSpec = "";
  while( 
    ( c = getopt_long( 
        argc, argv, 
        "f:n:v:s:o:r:c:m:i:M:d:bhV", long_options, NULL)) != -1) {
  
    // Examine command-line options and extract any optional arguments
    switch( c) {

      // format: Extract format of input file
      case 'f':
        if( !strncmp( optarg, "binary", 1)) dfm.ascii_input( 0);
        else if( !strncmp( optarg, "ascii", 1)) dfm.ascii_input( 1);
        else {
          usage();
          exit( -1);
        }
        break;

      // npoints: Extract maximum number of points (samples, rows of data) to 
      // read from the data file
      case 'n':
        dfm.npoints_cmd_line = atoi( optarg);
        if( dfm.npoints_cmd_line < 1)  {
          usage();
          exit( -1);
        }
        break;
      
      // nvars: Extract maximum number of variables (attributes) to read from 
      // each line of data file
      case 'v':
        dfm.nvars_cmd_line = atoi( optarg);
        if( dfm.nvars_cmd_line < 1)  {
          usage();
          exit( -1);
        }
        break;
      
      // nSkipHeaderLines: Extract number of header lines to skip at the
      // beginning of the data file
      case 's':
        dfm.n_skip_header_lines( atoi( optarg));
        if( dfm.n_skip_header_lines() < 0)  {
          usage();
          exit( -1);
        }
        break;
      
      // ordering: Extract the ordering of ("columnmajor or rowmajor") of a 
      // binary input file
      case 'o':
        if( !strncmp( optarg, "columnmajor", 1))
          dfm.column_major( 1);
        else if ( !strncmp( optarg, "rowmajor", 1))
          dfm.column_major( 0);
        else {
          usage();
          exit( -1);
        }
        break;

      // rows: Extract the number of rows of plot windows
      case 'r':
        nrows = atoi( optarg);
        if( nrows < 1)  {
          usage();
          exit( -1);
        }
        break;

      // cols: Extract the number of columns of plot windows
      case 'c':
        ncols = atoi( optarg);
        if( ncols < 1)  {
          usage();
          exit( -1);
        }
        break;

      // monitors: Extract the number of monitors
      case 'm':
        number_of_screens = atoi( optarg);
        if( number_of_screens < 1)  {
          usage();
          exit( -1);
        }
        break;

      // Missing or unreadable values get set to this number
      case 'M':
        bad_value_proxy = strtof (optarg, NULL);
        if( !bad_value_proxy) {
          usage();
          exit( -1);
        }
        break;

      // Missing or unreadable values get set to this number
      case 'd':
        if (optarg!=NULL)
          delimiter_char = optarg[0];
        else {
          usage();
          exit( -1);
        }
        break;

      // inputfile: Extract data filespec
      case 'i':
        inFileSpec.append( optarg);
        break;

      // borders: Turn off window manager borders on plot windows
      case 'b':
        borderless = 1;
        break;

      // show version information (managed by svn), and exit
      case 'V':
        cout << "$Id$" << endl;
        exit (-1);
        break;

      // help, or unknown option, or missing argument
      case 'h':
      case ':':
      case '?':
      default:
        usage();
        exit( -1);
        break;
    }
  }

  // If the command line was used with no path information in WIN32, Linux,
  // or MacOS, and no arguments were specified, provide usage information, 
  // then quit.  If the icon was clicked, path information should exist and
  // the GUI should be invoked.  NOTE: This test may not always work, and
  // has been abandoned.
  // if( argc == 1 && 
  //     ( strcmp( argv[ 0], "vp") == 0 || 
  //       strcmp( argv[ 0], "./vp") == 0 || 
  //       strcmp( argv[ 0], ".\\vp") == 0)) {
  //   usage();
  //   exit( 0);
  // }

  // If no data file was specified, but there was at least one argument 
  // in the command line, assume the last argument is the filespec.
  if( inFileSpec.length() <= 0 && argc > 1) inFileSpec.append( argv[ argc-1]);

  // Increment pointers to the optional arguments to get the last argument.
  argc -= optind;
  argv += optind;

  // Set random seed
  srand( (unsigned int) time(0));

  // Restrict format and restruct and set number of plots.  NOTE: nplots will 
  // be reset by manage_plot_window_array( NULL) 
  assert( nrows*ncols <= MAXPLOTS);
  nplots = nrows*ncols;

  // STEP 2: Read the data file create a 10-d default data set if the read 
  // attempt fails
  if( inFileSpec.length() <= 0) dfm.create_default_data( 10);
  else {
    dfm.input_filespec( inFileSpec);
    if( dfm.load_data_file() != 0) 
      dfm.create_default_data( 10);
  }
  
  // Fewer points -> bigger starting pointsize
  pointsize = max( 1.0, 6.0 - (int) log10f( (float) npoints));

  // STEP 3: Create main control panel.
  // Determine the number of screens.  NOTE screen_count requires OpenGL 1.7, 
  // which was not available under most Windows OS as of 10-APR-2006.
  #ifndef __WIN32__
    if( number_of_screens <= 0)
      number_of_screens = Fl::screen_count();
  #else 
    if( number_of_screens <= 0)
      number_of_screens = 1;
  #endif   // __WIN32__

  // Set the main control panel size and position.
  // const int main_w = 350, main_h = 700;
  const int main_x = 
    number_of_screens*Fl::w() - (main_w + left_frame + right_frame + right_safe);
  const int main_y = top_frame+top_safe;

  // Create the main control panel window
  create_main_control_panel( 
    main_x, main_y, main_w, main_h, "viewpoints -> creon.levit@nasa.gov");

  // Step 4: Call manage_plot_window_array with a NULL argument to
  // initialize the plot window array.  KLUDGE ALERT: argc and argv are
  // 'globalized' to make them available to manage_plot_window_array.
  global_argc = argc;
  global_argv = argv;
  manage_plot_window_array( NULL);

  // Invoke Plot_Window::initialize_selection to clear the random selection 
  // that can occur when vp is initialized on some Linux systems.  
  Plot_Window::initialize_selection();

  // Now we can show the main control panel and all its subpanels
  main_control_panel->show();

  // Step 5: Set pointer to the function to call when the window is idle and 
  // enter the main event loop
  // Fl::add_idle( redraw_if_changing);
  Fl::add_timeout(0.001, redraw_if_changing);

  // Enter the main event loop
  int result = Fl::run();
  return result;
}
