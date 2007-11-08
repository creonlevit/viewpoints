// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: control_panel_window.cpp
//
// Class definitions:
//   Control_Panel_Window -- Control panel window
//
// Classes referenced:
//   Plot_Window -- Plot window
//
// Required packages
//    FLTK 1.1.6 -- Fast Light Toolkit graphics package
//    FLEWS 0.3 -- Extensions to FLTK 
//    OGLEXP 1.2.2 -- Access to OpenGL extension under Windows
//    GSL 1.6 -- Gnu Scientific Library package for Windows
//    Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require D__WIN32__ for the C++ compiler
//
// Purpose: Source code for <Control_Panel_Window.h>
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  13-JUL-2007
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "plot_window.h"
#include "control_panel_window.h"

// Set static data members for class Control_Panel_Window::
//

// Array of menu items for axis selection menus.
// This gets filled with strings naming the axes (later, after they're read in)
// and with their respective indices (as user_data).
Fl_Menu_Item Control_Panel_Window::varindex_menu_items[ MAXVARS+2] = { Fl_Menu_Item()};

// Array of menu items for normalization style menus.
Fl_Menu_Item Control_Panel_Window::normalization_style_menu_items[] = {
  { "none",         0, 0, (void *) NORMALIZATION_NONE,         0, 0, 0, 0, 0},
  { "minmax",       0, 0, (void *) NORMALIZATION_MINMAX,       0, 0, 0, 0, 0},
  { "zeromax",      0, 0, (void *) NORMALIZATION_ZEROMAX,      0, 0, 0, 0, 0},
  { "maxabs",       0, 0, (void *) NORMALIZATION_MAXABS,       0, 0, 0, 0, 0},
  { "trim 1e-2",    0, 0, (void *) NORMALIZATION_TRIM_1E2,     0, 0, 0, 0, 0},
  { "trim 1e-3",    0, 0, (void *) NORMALIZATION_TRIM_1E3,     0, 0, 0, 0, 0},
  { "three sigma",  0, 0, (void *) NORMALIZATION_THREESIGMA,   0, 0, 0, 0, 0},
  { "log_10",       0, 0, (void *) NORMALIZATION_LOG10,        0, 0, 0, 0, 0},
  { "atanh",        0, 0, (void *) NORMALIZATION_SQUASH,       0, 0, 0, 0, 0},
  { "rank",         0, 0, (void *) NORMALIZATION_RANK,         0, 0, 0, 0, 0},
  { "partial rank", 0, 0, (void *) NORMALIZATION_PARTIAL_RANK, 0, 0, 0, 0, 0},
  { "gaussianize",  0, 0, (void *) NORMALIZATION_GAUSSIANIZE,  0, 0, 0, 0, 0},
  { 0,              0, 0, (void *) 0,                          0, 0, 0, 0, 0}
};

//***************************************************************************
// Control_Panel_Window::Control_Panel_Window( x, y, w, h) --  Default 
// constructor.  Do nothing except call the constructor for the parent 
// class, Fl_Group.
Control_Panel_Window::Control_Panel_Window(
  int x, int y, int w, int h) : Fl_Group( x, y, w, h)
{}

//***************************************************************************
// Control_Panel_Window::broadcast_change (*master_widget) -- Broadcast an 
// interaction from the master panel to all (unlocked) panels.
// MCL XXX "locked" panels are not yet implemented.
void Control_Panel_Window::broadcast_change (Fl_Widget *master_widget)
{
  // Define a pointer to the parent of the master widget and verify
  // that it exists
  const Fl_Group *master_panel = master_widget->parent();
  assert( master_panel);

  // Indentify the index of the maser widget and verify that it has
  // a plausible value
  const int widget_index = master_panel->find(master_widget);
  assert( widget_index >= 0 && widget_index < master_panel->children());

  // Loop: Apply operation defined by the master widget to 
  // succesive windows
  for (int i=0; i<nplots; i++)
  {
    // Define a pointer to the relevant widget in this window and
    // verify that it exists
    Fl_Widget *slave_widget = cps[i]->child(widget_index);
    assert( slave_widget);
    
    // cout << "master_widget: label = " << master_widget->label() << " type = " << typeid(*master_widget).name() << endl;
    // cout << "slave_widget:  label = " << slave_widget->label() <<  " type = " << typeid(*slave_widget).name() << endl;

    // Verify that master that and slave widgets are of the same type
    assert( typeid(master_widget) == typeid(slave_widget));

    // MCL XXX downcasting to dispatch on type and/or existance of a method is
    // considered very bad form.  
    // If value() were a virtual function of Fl_Widget (like callback() and 
    // do_callback() are) it would be cleaner.  Or we could  use one of the
    // publish/subscribe extensions (from fltk or boost).  That could clean up 
    // all sort of things.....  Or we could take a stab at refactoring the
    // repetitive parts of the following code using templates.
    
    // If the master widget is a button then set the slave's value using
    // the master's value.
    {
      Fl_Button *mp, *sp;
      if( (mp = dynamic_cast <Fl_Button*> (master_widget)) && (sp = dynamic_cast <Fl_Button*> (slave_widget)))
      {
        sp->value(mp->value());
      }
    }

    // See previous comment
    {
      Fl_Valuator *mp, *sp;
      if( (mp = dynamic_cast <Fl_Valuator*> (master_widget)) && (sp = dynamic_cast <Fl_Valuator*> (slave_widget)))
      {
        sp->value(mp->value());
      }
    }

    // See previous comment
    {
      Fl_Choice *mp, *sp;
      if( (mp = dynamic_cast <Fl_Choice*> (master_widget)) && (sp = dynamic_cast <Fl_Choice*> (slave_widget)))
      {
        sp->copy(mp->menu());  // necessary when there is per menu item state info (for FL_MENU_TOGGLE, etc)
        sp->value(mp->value());
      }
    }

    // See previous comment
    {
      Fl_Menu_Button *mp, *sp;
      if( (mp = dynamic_cast <Fl_Menu_Button*> (master_widget)) && (sp = dynamic_cast <Fl_Menu_Button*> (slave_widget)))
      {
        sp->copy(mp->menu());  // necessary when there is per menu item state info (for FL_MENU_TOGGLE, etc)
        sp->value(mp->value());
      }
    }

    // If the slave widget has a callback function defined, call the
    // callback (since the slave widget's value() may have just been updated).
    if( slave_widget->callback())
    {
      // cout << ".. doing callback for widget " << widget_index 
      //      << " in panel " << i << endl;
      slave_widget->do_callback( slave_widget, cps[i]);
    }
  }
}

//***************************************************************************
// Control_Panel_Window::maybe_draw() -- Check plot window to see if they 
// need to be redrawn.
void Control_Panel_Window::maybe_redraw() 
{
  // kludge.  Avoid double redraw when setting "don't clear".
  if( dont_clear->value()) return;
  pw->needs_redraw = 1;
}

//***************************************************************************
// Plot_Window::extract_and_redraw() -- Extract data for these (new?) axes 
// and redraw plot.  For one local control panel only.
void Control_Panel_Window::extract_and_redraw ()
{
  pw->extract_data_points();
}

//***************************************************************************
// Control_Panel_Window::make_widgets( cpw) -- Make widgets
void Control_Panel_Window::make_widgets( Control_Panel_Window *cpw)
{
  // Since these (virtual) control panels are really groups inside a tab 
  // inside a window, set their child widget's coordinates relative to 
  // their enclosing window's position. 
  int xpos = this->x()+50;
  int ypos = this->y()+20;

  Fl_Button *b;
  // Fl_Round_Button *rb;
  Fl_Choice *c;

  // the following portion of the panel deals with axes and their properties

  xpos = 50;
  int subwidth=105;  // ~1/3 of (width - extra room)

  // label for row of variable chooser menus
  b = new Fl_Button (xpos, ypos+=5, 45, 25, ""); // cleaner look with nothing here?
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // dynamically build the variables (axes selection) menu(s).
  // cout << "starting axes menu build, nvars = " << nvars << endl;
  for( int i=0; i<=nvars; i++) {
    // cout << "label " << i 
    //      << " = " << column_labels[i].c_str() << endl;
    varindex_menu_items[i].label((const char *)(column_labels[i].c_str()));
    varindex_menu_items[i].user_data((void *)i);
  }
  varindex_menu_items[nvars+1].label(0);

  // X-axis variable selection menu
  varindex1 = new Fl_Choice (xpos, ypos, subwidth-15, 25, "X axis");
  varindex1->align(FL_ALIGN_TOP);
  varindex1->textsize(12);
  varindex1->copy( varindex_menu_items);
  varindex1->mode( nvars, FL_MENU_INACTIVE);  // disable "--nothing--" as a choice for axis1
  varindex1->clear_visible_focus();
  varindex1->callback( (Fl_Callback*)static_extract_and_redraw, this);
  varindex1->tooltip("select variable for this plot's x-axis");

  // Y-axis variable selection menu
  varindex2 = new Fl_Choice (xpos+subwidth, ypos, subwidth-15, 25, "Y axis");
  varindex2->align(FL_ALIGN_TOP);
  varindex2->textsize(12);
  varindex2->copy( varindex_menu_items);
  varindex2->mode( nvars, FL_MENU_INACTIVE);  // disable "--nothing--" as a choice for axis2
  varindex2->clear_visible_focus();
  varindex2->callback( (Fl_Callback*)static_extract_and_redraw, this);
  varindex2->tooltip("select variable for this plot's y-axis");

  // Z-axis variable selection menu
  varindex3 = new Fl_Choice (xpos+2*subwidth, ypos, subwidth-15, 25, "Z axis");
  varindex3->align(FL_ALIGN_TOP);
  varindex3->textsize(12);
  varindex3->copy( varindex_menu_items);
  varindex3->value(nvars);  // initially, axis3 == "-nothing-"
  varindex3->clear_visible_focus();
  varindex3->callback( (Fl_Callback*)static_extract_and_redraw, this);
  varindex3->tooltip("select variable for this plot's z-axis");

  // one label for row of lock axis buttons
  b = new Fl_Button (xpos, ypos+=25, 65, 25, "locked");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // Lock x-axis button
  lock_axis1_button = b = new Fl_Button(xpos+0*subwidth, ypos, 20, 20);
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);
  b->tooltip("make this plot's x axis immune from 'change axis' events'");

  // Lock y-axis button
  lock_axis2_button = b = new Fl_Button(xpos+1*subwidth, ypos, 20, 20);
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);
  b->tooltip("make this plot's y axis immune from 'change axis' events'");

  // Lock z-axis button
  lock_axis3_button = b = new Fl_Button(xpos+2*subwidth, ypos, 20, 20);
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);
  b->tooltip("make this plot's z axis immune from 'change axis' events'");

  // label for row of normalization menus
  b = new Fl_Button (xpos, ypos+=25, 45, 25, "scale");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // X-axis normalization and scaling menu
  x_normalization_style = c = new Fl_Choice( xpos, ypos, subwidth-15, 25);
  c->textsize( 12);
  c->menu( normalization_style_menu_items);
  c->value( NORMALIZATION_MINMAX);
  c->clear_visible_focus();
  c->callback( (Fl_Callback*)static_extract_and_redraw, this);
  c->tooltip("choose normalization and/or scaling for x-axis");


  // Y-axis normalization and scaling menu
  y_normalization_style = c = new Fl_Choice( xpos+subwidth, ypos, subwidth-15, 25);
  c->textsize(12);
  c->menu(normalization_style_menu_items);
  c->value(NORMALIZATION_MINMAX); 
  c->clear_visible_focus();
  c->callback( (Fl_Callback*)static_extract_and_redraw, this);
  c->tooltip("choose normalization and/or scaling for y-axis");
 
  // Z-axis normalization and scaling menu
  z_normalization_style = c = new Fl_Choice( xpos+2*subwidth, ypos, subwidth-15, 25);
  c->textsize(12);
  c->menu(normalization_style_menu_items);
  c->value(NORMALIZATION_MINMAX); 
  c->clear_visible_focus();
  c->callback( (Fl_Callback*)static_extract_and_redraw, this);
  c->tooltip("choose normalization and/or scaling for z-axis");
 
  // histogram controls
  ypos += 35;    

  // label for row of histogram menus
  b = new Fl_Button (xpos, ypos, 45, 25, "histog");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  Fl_Menu_Item histogram_pulldown[] = {
    {"marginal",    0, 0, (void *)HISTOGRAM_MARGINAL,    FL_MENU_TOGGLE},
    {"selection",   0, 0, (void *)HISTOGRAM_SELECTION,   FL_MENU_TOGGLE},
    {"conditional", 0, 0, (void *)HISTOGRAM_CONDITIONAL, FL_MENU_TOGGLE|FL_MENU_DIVIDER},
    {"weighted",    0, 0, (void *)HISTOGRAM_WEIGHTED,    FL_MENU_TOGGLE},
    {0}
  };
  // int n_histogram_pulldown_items = (sizeof(histogram_pulldown) / sizeof(histogram_pulldown[0])) - 1;

  // create three show histogram buttons, one for each axis.
  for (int i=0; i<3; i++) {
    show_histogram[i] = new Fl_Menu_Button(xpos+i*subwidth, ypos, subwidth-55, 20, "");
    // copy in the menu items
    show_histogram[i]->copy(histogram_pulldown);
    // normal menu, not popup.
    show_histogram[i]->type(0); 
    show_histogram[i]->callback((Fl_Callback*)redraw_one_plot, this);
    show_histogram[i]->selection_color(FL_BLUE);  
    show_histogram[i]->clear_visible_focus();
  }
  show_histogram[0]->tooltip("histogram options for x-axis");
  show_histogram[1]->tooltip("histogram options for y-axis");
  // no Z-axis histograms (yet)
  show_histogram[2]->deactivate();

  // one label for row of bin count sliders
  b = new Fl_Button (xpos, ypos+=25, 45, 25, "N bins");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // create three bin count sliders, one for each axis.
  for (int i=0; i<3; i++) {
    nbins_slider[i] = new Fl_Hor_Value_Slider_Input(xpos+i*subwidth, ypos, subwidth-15, 20);
    nbins_slider[i]->textboxsize(30);
    nbins_slider[i]->callback((Fl_Callback*)redraw_one_plot, this);
    nbins_slider[i]->range(0.0,log2((double)Plot_Window::nbins_max));
    // nbins_slider[i]->precision(0);
    nbins_slider[i]->value(log2((double)Plot_Window::nbins_default));
    nbins_slider[i]->set_changed();
  }
  nbins_slider[0]->tooltip("set base 2 log of number of bins for x-axis hostograms");
  nbins_slider[1]->tooltip("set base 2 log of number of bins for y-axis hostograms");
  // no Z-axis histograms (yet)
  nbins_slider[2]->deactivate();
    
  // one label for row of histogram height sliders
  b = new Fl_Button (xpos, ypos+=25, 45, 25, "bin ht");
  b->labelsize(14);
  b->align(FL_ALIGN_LEFT);
  b->box(FL_NO_BOX);

  // create three bin height sliders, one for each axis.
  for (int i=0; i<3; i++) {
    hscale_slider[i] = new Fl_Hor_Value_Slider_Input(xpos+i*subwidth, ypos, subwidth-15, 20);
    hscale_slider[i]->textboxsize(30);
    hscale_slider[i]->callback((Fl_Callback*)redraw_one_plot, this);
    hscale_slider[i]->range(0.0,10.0);
    hscale_slider[i]->value(1.0);
    hscale_slider[i]->set_changed();
  }    
  hscale_slider[0]->tooltip("scale bin height for x-axis histograms");
  hscale_slider[1]->tooltip("scale bin height for y-axis histograms");
  // no Z-axis histograms (yet)
  hscale_slider[2]->deactivate();
    
  ypos += 15;

  // Background color slider
  Bkg = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "Bkg");
  Bkg->align(FL_ALIGN_LEFT);
  Bkg->step(0.0001);
  Bkg->bounds(0.0,1.0);
  Bkg->callback((Fl_Callback*)replot, this);
  Bkg->value(0.0);
  Bkg->tooltip("change background brightness");

  // Luminance for this plot
  lum = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-60, 20, "lum");
  lum->align(FL_ALIGN_LEFT);
  lum->step(0.0001);
  lum->bounds(0.0,2.0);
  lum->callback((Fl_Callback*)replot, this);
  lum->value(1.0);
  lum->tooltip("adjust luminance for all points");

  // overall pointsize scale for this plot
  size = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-115, 20, "psize");
  size->align(FL_ALIGN_LEFT);
  size->step(0.0001);
  size->bounds(-4.0,4.0);
  size->callback((Fl_Callback*)replot, this);
  size->value(0.0);
  size->tooltip("adjust size of all points in this plot");

  scale_points = new Fl_Button(xpos+size->w()+5, ypos, 20, 20, "scale");
  scale_points->align(FL_ALIGN_RIGHT);
  scale_points->type(FL_TOGGLE_BUTTON);
  scale_points->selection_color(FL_BLUE);
  scale_points->callback((Fl_Callback*)replot, this);
  scale_points->tooltip("scale all points when zooming");

  // Rotation (and spin) slider
  rot_slider = new Fl_Hor_Value_Slider_Input( xpos, ypos+=25, cpw->w()-115, 20, "rot");
  rot_slider->align(FL_ALIGN_LEFT);
  rot_slider->callback((Fl_Callback*)replot, this);
  rot_slider->value(0.0);
  rot_slider->step(0.001);
  rot_slider->bounds(-180.0, 180.0);
  rot_slider->tooltip("rotate plot around screen y");

  spin = b = new Fl_Button(xpos+rot_slider->w()+5, ypos, 20, 20, "spin");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->type(FL_TOGGLE_BUTTON);
  b->tooltip("toggle continuous rotation around screen y");

  // Next portion of the panel is miscellanious stuff, per plot
  // needs to be more organized.
  // ypos+=15;

  // Initialize positions for buttons
  int xpos2 = 20;
  int ypos2 = ypos;

  // Button (1,1) Reset view in this plot
  reset_view_button = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "reset view ");
  b->align(FL_ALIGN_RIGHT); b->selection_color(FL_BLUE);
  b->callback((Fl_Callback*) reset_view, this);
  b->tooltip("reset translations and scalings for this plot");

  // Button (1,2): z-buffering control
  z_bufferring_button = b = new Fl_Button(xpos2, ypos+=25, 20, 20, "z-bufferring");
  b->callback((Fl_Callback*)redraw_one_plot, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE); 
  b->tooltip("toggle z-buffering for this plot");

  // blending menu for this plot
  Fl_Menu_Item blend_menu_items[] = {
    {"overplot",                               0, 0, (void *)BLEND_OVERPLOT,            0, 0, 0, 0, 0},
    {"overplot with alpha",                    0, 0, (void *)BLEND_OVERPLOT_WITH_ALPHA, 0, 0, 0, 0, 0},
    {"luminance blend each brush sepratately", 0, 0, (void *)BLEND_BRUSHES_SEPARATELY,  0, 0, 0, 0, 0},
    {"luminance blend all brushes",            0, 0, (void *)BLEND_ALL_BRUSHES,         0, 0, 0, 0, 0},
    {0}
  };

  blend_menu = new Fl_Choice(xpos2, ypos+=25, 20, 20);
  blend_menu->textsize(14);
  blend_menu->copy(blend_menu_items);
  blend_menu->align(FL_ALIGN_RIGHT);
  blend_menu->label("blending");
  blend_menu->value(BLEND_BRUSHES_SEPARATELY);
  blend_menu->clear_visible_focus();
  blend_menu->callback( (Fl_Callback*)redraw_one_plot, this);
  blend_menu->tooltip("select blending for this brush");

  // Button (1,3): Don't clear button - psychedelic fun!
  dont_clear = new Fl_Button(xpos2, ypos+=25, 20, 20, "don't clear");
  dont_clear->align(FL_ALIGN_RIGHT);
  dont_clear->type(FL_TOGGLE_BUTTON);
  dont_clear->selection_color(FL_BLUE);
  dont_clear->callback((Fl_Callback*)static_maybe_redraw, this);
  dont_clear->tooltip("psychedelic fun");

  ypos=ypos2;
  xpos=xpos2+120;

  // Button (1,2): Show points
  show_points = b = new Fl_Button(xpos, ypos+=25, 20, 20, "points");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);
  b->tooltip("toggle visibility of all points");

  // Button (2,2): Show deselected points
  show_deselected_points = b = new Fl_Button(xpos, ypos+=25, 20, 20, " unselected");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);
  b->tooltip("toggle visibility of brush[0] (nonseleted) points");

  // Button (3,2): Show axes
  show_axes = b = new Fl_Button(xpos, ypos+=25, 20, 20, "axes");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);
  b->tooltip("toggle visibility of axis lines");

#if 0
  // Button (4,2): Show axis labels
  show_labels = b = new Fl_Button(xpos, ypos+=25, 20, 20, "labels");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);
  b->tooltip("toggle visibility of axis labels");
#endif // 0

  // Button (4,2): Show axis tickmarks
  show_scale = b = new Fl_Button(xpos, ypos+=25, 20, 20, "ticks");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);  
  b->value(1);
  b->tooltip("toggle visibility of axis tickmarks");

  // Button (5,2): Show grid (needs work)
  show_grid = b = new Fl_Button(xpos, ypos+=25, 20, 20, "grid");
  b->callback((Fl_Callback*)static_maybe_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_TOGGLE_BUTTON); 
  b->selection_color(FL_BLUE);
  b->value(0);
  b->tooltip("toggle visibility of simple grid");

  ypos=ypos2;
  xpos=xpos2+225;

  // Define Fl_Group to hold plot transform styles
  // XXX - this group should probably be a menu, or at least have a box around it
  // to show that they are radio buttons.
  transform_style = new Fl_Group (xpos-1, ypos+25-1, 20+2, 4*25+2);

  // Button (4,1): No transform
  no_transform = b = new Fl_Button(xpos, ypos+=25, 20, 20, "identity");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
  b->tooltip("plot x and y values without modification");

  // Button (5,1): Sum vs difference transform
  sum_vs_difference = b = new Fl_Button(xpos, ypos+=25, 20, 20, "sum vs. diff.");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
  b->tooltip("plot (x+y) vs. (x-y)");
  
  // Button (6,1): cummulative conditional probability or rank of y given x
  cond_prop = b = new Fl_Button(xpos, ypos+=25, 20, 20, "rank(y|x)");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
  b->tooltip("plot (x) vs. (rank of y given x). i.e. conditional rank");
  
  // Button (7,1): fluctuation of y given x
  fluctuation = b = new Fl_Button(xpos, ypos+=25, 20, 20, "fluct(y|x)");
  b->callback((Fl_Callback*)static_extract_and_redraw, this);
  b->align(FL_ALIGN_RIGHT); 
  b->type(FL_RADIO_BUTTON); 
  b->selection_color(FL_BLUE);
  b->tooltip("plot (x) vs. (deviation of y given x). i.e. conditional deviation");
  
  transform_style->end();
  no_transform->setonly();
}
