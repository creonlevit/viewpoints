// viewpoints - interactive linked scatterplots and more.
// copyright 2005 Creon Levit and Paul Gazis, all rights reserved.
//***************************************************************************
// File name: data_file_manager.cpp
//
// Class definitions:
//   Data_File_Manager -- Data file manager
//
// Classes referenced:
//   Various BLITZ templates
//
// Required packages
//   GSL 1.6 -- Gnu Scientific Library package for Windows
//   Blitz++ 0.9 -- Various math routines
//
// Compiler directives:
//   May require D__WIN32__ for the C++ compiler
//
// Purpose: Source code for <data_file_manager.h>
//
// Author: Creon Levit    2005-2006
// Modified: P. R. Gazis  23-SEP-2008
//***************************************************************************

// Include the necessary include libraries
#include "include_libraries_vp.h"

// Include globals
#include "global_definitions_vp.h"

// Include associated headers and source code
#include "data_file_manager.h"
#include "column_info.h"
#include "plot_window.h"

// These includes should not be necessary and have been commented out
// #include "Vp_File_Chooser.H"   // PRG's new file chooser
// #include "Vp_File_Chooser.cpp"   // PRG's new file chooser

// Be sure to define the static data member to hold column info
std::vector<Column_Info> Data_File_Manager::column_info;

// Set static data members for class Data_File_Manager::
string Data_File_Manager::SELECTION_LABEL = "SELECTION_BY_VP";
string Data_File_Manager::BINARY_FILE_WITH_ASCII_VALUES_LABEL = 
  "BINARY_FILE_WITH_ASCII_VP";
Fl_Window* Data_File_Manager::edit_labels_window = NULL;
Fl_Check_Browser* Data_File_Manager::edit_labels_widget = NULL;
int Data_File_Manager::needs_restore_panels_ = 0;

// Define and set maximums length of header lines and number of lines in the 
// header block
const int Data_File_Manager::MAX_HEADER_LENGTH = MAXVARS*100;
const int Data_File_Manager::MAX_HEADER_LINES = 2000;

const bool include_line_number = false; // MCL XXX This should be an option

//***************************************************************************
// Data_File_Manager::Data_File_Manager() -- Default constructor, calls the
// initializer.
Data_File_Manager::Data_File_Manager() : delimiter_char_( ' '), 
  bad_value_proxy_( 0.0), maxpoints_( MAXPOINTS), maxvars_( MAXVARS),
  inputFileType_( 0), outputFileType_( 0),
  readSelectionInfo_( 0), doAppend( 0), doMerge( 0), 
  writeAllData_( 1), writeSelectionInfo_( 0), doCommentedLabels_( 0),
  isColumnMajor( 0), isSavedFile_( 0), nDataRows_( 0), nDataColumns_( 0)
{
  sDirectory_ = ".";  // Default pathname
  initialize();
}

//***************************************************************************
// Data_File_Manager::initialize() -- Reset control parameters.
void Data_File_Manager::initialize()
{
  // Set default format information for file reads.
  delimiter_char_ = ' ';
  bad_value_proxy_ = 0.0;
  maxpoints_ = MAXPOINTS;
  maxvars_ = MAXVARS;

  // Set default local values for file reads.
  inputFileType_ = 0;
  outputFileType_ = 1;
  isAsciiData = 1-inputFileType_;   // Conversion needed for legacy reasons
  doAppend = 0;
  doMerge = 0;
  readSelectionInfo_ = 1;
  writeAllData_ = 1;
  writeSelectionInfo_ = 0;
  isSavedFile_ = 0;
  nDataRows_ = 0;
  nDataColumns_ = 0;
  needs_restore_panels_ = 0;

  isColumnMajor = 1;
  nSkipHeaderLines = 0;  // Number of header lines to skip
  // sDirectory_ = ".";  // Default pathname -- NOT NEEDED!
  inFileSpec = "";  // Default input filespec
  outFileSpec = "";
  dataFileSpec = "";

  // Initialize the number of points and variables specified by the command 
  // line arguments.  NOTE: 0 means read to EOF or end of line.
  npoints_cmd_line = 0;
  nvars_cmd_line = 0;
  
  // Initialize number of points and variables
  // npoints = MAXPOINTS;
  // nvars = MAXVARS;
  npoints = maxpoints_;
  nvars = maxvars_;
}

//***************************************************************************
// Data_File_Manager::copy_state( *dfm) -- Copy control parameters from 
// another object.
void Data_File_Manager::copy_state( Data_File_Manager* dfm)
{
  // Copy format information for file reads.
  delimiter_char_ = dfm->delimiter_char_;
  bad_value_proxy_ = dfm->bad_value_proxy_;
  maxpoints_ = dfm->maxpoints_;
  maxvars_ = dfm->maxvars_;

  // Copy local values for file reads.
  inputFileType_ = dfm->inputFileType_;
  outputFileType_ = dfm->outputFileType_;
  isAsciiData = 1-inputFileType_;   // Conversion needed for legacy reasons
  doAppend = dfm->doAppend;
  doMerge = dfm->doMerge;
  readSelectionInfo_ = dfm->readSelectionInfo_;
  writeAllData_ = dfm->writeAllData_;
  writeSelectionInfo_ = dfm->writeSelectionInfo_;
  isSavedFile_ = dfm->isSavedFile_;
  nDataRows_ = dfm->nDataRows_;
  nDataColumns_ = dfm->nDataColumns_;
  needs_restore_panels_ = dfm->needs_restore_panels_;

  isColumnMajor = dfm->isColumnMajor;
  nSkipHeaderLines = dfm->nSkipHeaderLines;  // Number of lines to skip
  sDirectory_ = dfm->sDirectory_;
  inFileSpec = dfm->inFileSpec;   // "";  // Default input filespec
  outFileSpec = dfm->outFileSpec;
  dataFileSpec =dfm->dataFileSpec;

  // Initialize the number of points and variables specified by the command 
  // line arguments.  NOTE: 0 means read to EOF or end of line.
  npoints_cmd_line = dfm->npoints_cmd_line;
  nvars_cmd_line = dfm->nvars_cmd_line;
  
  // Regenerate global number of points and variables.  Note that the last
  // Column_Info object is the dummy variable '-nothing-'
  nvars = n_vars();
  npoints = n_points();
}

//***************************************************************************
// Data_File_Manager::findInputFile() -- Query user to find the input file.
// Class Vp_File_Chooser is used in preference to a Vp_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the window.  
// Returns 0 if successful.  
int Data_File_Manager::findInputFile()
{
  // Print empty line to console for reasons of aesthetics
  cout << endl;

  // Generate text, file extensions, etc, for this file type.  XXX PRG: Most 
  // of this should be moved to Vp_File_Chooser
  string title;
  string pattern;
  if( inputFileType_ == 0) {
    title = "Open data file";
    pattern = "*.{txt,lis,asc}\tAll Files (*)";
  }
  else if( inputFileType_ == 1) {
    title = "Open data file";
    pattern = "*.bin\tAll Files (*)";
  }
  else if( inputFileType_ == 2) {
    title = "Open data file";
    pattern = "*.{fit,fits}\tAll Files (*)";
  }
  else {
    title = "Open data file";
    pattern = "*.bin\tAll Files (*)";
  }

  // Initialize read status and filespec.  NOTE: cInFileSpec is defined as
  // const char* for use with Vp_File_Chooser, which means it could be 
  // destroyed by the relevant destructors!
  const char *cInFileSpec = sDirectory_.c_str();
  
  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname must
  // be passed as a variable or the window will begin in some root directory.
  Vp_File_Chooser* file_chooser =
    new Vp_File_Chooser( 
      cInFileSpec, pattern.c_str(), Vp_File_Chooser::SINGLE, title.c_str());
  file_chooser->fileType( inputFileType_);
  
  // Comment this out to use the value file_chooser provides
  // file_chooser->doCommentedLabels( doCommentedLabels_);

  // Loop: Select fileSpecs until a non-directory is obtained.  NOTE: If all
  // goes well, this should be handled by the file_chooser object
  while( 1) {
    if( cInFileSpec != NULL) file_chooser->directory( cInFileSpec);

    // Loop: wait until the file selection is done
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cInFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cInFileSpec == NULL) {
      cerr << "Data_File_Manager::findInputFile: "
           << "No input file was specified" << endl;
      break;
    }

    // In FLTK 1.1.7 under Windows, the fl_filename_isdir method doesn't work, 
    // so try to open this file to see if it is a directory.  If it is, set 
    // the pathname and continue.  Otherwise merely update the pathname.  
    FILE* pFile = fopen( cInFileSpec, "r");
    if( pFile == NULL) {
      file_chooser->directory( cInFileSpec);
      directory( (string) cInFileSpec);
      continue;
    }
    else {
      directory( (string) file_chooser->directory());
    }

    fclose( pFile);
    break;         
  } 

  // If no file was specified then report, deallocate the Vp_File_Chooser 
  // object, and quit.
  if( cInFileSpec == NULL) {
    cerr << "Data_File_Manager::findInputFile: "
         << "No input file was specified" << endl;
    delete file_chooser;  // WARNING! Destroys cInFileSpec!
    return -1;
  }

  // Query Vp_File_Chooser object to get the file type, delimiter character,
  // and usage of a comment character with the column label information
  inputFileType_ = file_chooser->fileType();
  delimiter_char_ = file_chooser->delimiter_char();
  if( file_chooser->doCommentedLabels() != 0) doCommentedLabels_ = 1;
  else doCommentedLabels_ = 0;
  
  // Load the inFileSpec string
  inFileSpec.assign( (string) cInFileSpec);
  if( inputFileType_ == 0) 
    cout << "Data_File_Manager::inputFile: Reading ASCII data from <";
  else if( inputFileType_ == 1) 
    cout << "Data_File_Manager::findInputFile: Reading binary data from <";
  else if( inputFileType_ == 2) 
    cout << "Data_File_Manager::findInputFile: Reading FITS extension from <";
  else 
    cout << "Data_File_Manager::findInputFile: Reading binary data from <";
  cout << inFileSpec.c_str() << ">" << endl;

  // Deallocate the file_chooser object
  delete file_chooser;  // WARNING! This destroys cInFileSpec!

  // Perform partial initialization and return success
  // nSkipHeaderLines = 1;
  nSkipHeaderLines = 0;
  npoints_cmd_line = 0;
  nvars_cmd_line = 0;
  // npoints = MAXPOINTS;
  // nvars = MAXVARS;
  npoints = maxpoints_;
  nvars = maxvars_;

  return 0;
}

//***************************************************************************
// Data_File_Manager::load_data_file( inFileSpec) -- Copy the input filespec, 
// then invoke load_data_file to load this file.
int Data_File_Manager::load_data_file( string inFileSpec) 
{
  input_filespec( inFileSpec);
  return load_data_file();
}

//***************************************************************************
// Data_File_Manager::load_data_file() -- Read an ASCII or binary data file, 
// resize arrays to allocate meomory.  Returns 0 if successful.
int Data_File_Manager::load_data_file() 
{
  // PRG XXX: Would it be possible or desirable to examine the file directly 
  // here to determine or verify its format?
  if( inFileSpec.length() <= 0) {
    cout << "Data_File_Manager::load_data_file: "
         << "No input file was specified" << endl;
    return -1;
  }

  // If this is an append or merge operation, save the existing data and 
  // column labels in temporary buffers
  unsigned uHaveOldData = 0;
  int old_npoints=0, old_nvars=0;
  std::vector<Column_Info> old_column_info; 
  blitz::Array<int,1> old_selected;
  if( preserve_old_data_mode || doAppend > 0 || doMerge > 0) {
    uHaveOldData = 1;
    old_column_info = column_info;
    old_nvars = n_vars();
    old_npoints = n_points();
    old_selected.resize(old_npoints);
    old_selected = selected;
  }

  // Initialize READ_SELECTED here
  // read_selected.resize( npoints);
  // read_selected.resize( MAXPOINTS);
  read_selected.resize( maxpoints_);
  read_selected = 0;
  
  // Read data file.If there was a problem, create default data to prevent 
  // a crash, then quit before something terrible happens!  NOTE: The read
  // methods load selection information, but don't resize read_selected;
  cout << "Data_File_Manager::load_data_file: Reading input data from <"
       << inFileSpec.c_str() << ">" << endl;
  int iReadStatus = 0;
  if( inputFileType_ == 0) iReadStatus = read_ascii_file_with_headers();
  else if( inputFileType_ == 2) iReadStatus = read_table_from_fits_file();
  else iReadStatus = read_binary_file_with_headers();
  if( iReadStatus != 0) {
    cout << "Data_File_Manager::load_data_file: "
         << "Problems reading file <" << inFileSpec.c_str() << ">" << endl;
    if( !uHaveOldData) create_default_data( 4);
    else {
      nvars = old_nvars;
      npoints = old_npoints;
      resize_global_arrays();
      column_info = old_column_info;
      selected = old_selected;
      uHaveOldData = 0;
    }
    return -1;
  }
  else
    cout << "Data_File_Manager::load_data_file: Finished reading file <" 
         << inFileSpec.c_str() << ">" << endl;
  
  // Resize the READ_SELECTED array here
  if( npoints>0) read_selected.resizeAndPreserve( npoints);  

  // Remove trivial columns
  if( npoints>0 && trivial_columns_mode) remove_trivial_columns();

  // If only one or fewer records are available, generate default data to
  // prevent a crash, then quit before something terrible happens!
  if( npoints <= 0 ||
      (( doAppend == 0 && doMerge == 0) && ( nvars <= 1 || npoints <= 1))) {
    cerr << " -WARNING: Insufficient data, " << nvars << "x" << npoints
         << " samples.\nCheck delimiter character." << endl;
    string sWarning = "";
    sWarning.append( "WARNING: Insufficient number of attributes or samples\n.");
    sWarning.append( "Check delimiter value and 'commented labels' setting.\n");
    if( !uHaveOldData) {
      sWarning.append( "Generating default data.");
      create_default_data( 4);
    }
    else {
      sWarning.append( "Restoring existing data.");

      nvars = old_nvars;
      npoints = old_npoints;
      resize_global_arrays();
      column_info = old_column_info;
      selected = old_selected;
      uHaveOldData = 0;
    }
    make_confirmation_window( sWarning.c_str(), 1, 3);
    return -1;
  }
  else {
    cout << "Data_File_Manager::load_data_file: Loaded " << npoints
         << " samples with " << nvars << " fields" << endl;
  }
  
  // MCL XXX Now that we're done reading, we can update nvars to count possible
  // additional program-generated variables (presently only the line number).
  if( include_line_number) nvars = nvars+1;

  // Compare array sizes and finish the append or merge operation
  if( doAppend > 0 | doMerge > 0) {

    // If array sizes aren't consistent, restore the old data and column 
    // labels.  Otherwise, reverse old and new arrays along the relevant
    // dimensions, copy the old data to the new array, reverse the new array
    // again, and copy new column labels if this was a merge operation.
    if( ( doAppend > 0 && nvars != old_nvars) ||
        ( doMerge > 0 && npoints != old_npoints)) {
      cout << "Old (" << old_nvars << "x" << old_npoints
           << ") array doesn't match new (" << nvars << "x" << npoints
           << ") array" << endl;

      char cBuf[ 80];
      sprintf(
        cBuf, "Array sizes old(%ix%i) vs new(%ix%i) don't match.\n", 
        old_npoints, old_nvars, npoints, nvars);
      string sWarning = "";
      sWarning.append( cBuf);
      sWarning.append( "Restoring old data.");
      make_confirmation_window( sWarning.c_str(), 1);
      
      nvars = old_nvars;
      npoints = old_npoints;
      resize_global_arrays();
      column_info = old_column_info;
      selected = old_selected;
      return -1;
    }
    else if( doAppend > 0) {
      
      // Current (e.g., new) column info and lookup table indices in the
      // current (new) common data array must be revised first, before the 
      // current and old data arrays are appended.
      for( int j=0; j<nvars; j++) {
        column_info[j].add_info_and_update_data( old_column_info[j]);
      }

      // Enlarge buffer that contains old data to make space for current data
      // and make sure current data buffer is the right size
      int all_npoints = npoints + old_npoints;
      for( int j=0; j<nvars; j++) {
        (old_column_info[j].points).resizeAndPreserve(all_npoints);
        (column_info[j].points).resizeAndPreserve(npoints);
      }

      // Append current data to old data and resize the current data buffer
      for( int j=0; j<nvars; j++) {
        (old_column_info[j].points(blitz::Range(old_npoints,all_npoints-1))) =
          column_info[j].points;
        (column_info[j].points).resize( all_npoints-1);
      }

      // Move combined data set back to the current data buffer
      column_info = old_column_info;
      npoints = all_npoints;
      
      // Loop: Examine the vector of Column_Info objects to alphabetize 
      // ASCII values and renumber the data.
      int nReordered = 0;
      for( int j=0; j<nvars; j++) {
        if( column_info[j].update_ascii_values_and_data() >=0) nReordered++;
      }
    }
    else {
      int all_nvars = nvars + old_nvars;

      // This old code is retained for archival purposes to document how
      // BLITZ arrays had to be reversed, appended, and re-reversed
      // points.reverseSelf( blitz::firstDim);
      // old_npoints.reverseSelf( blitz::firstDim);
      // points.resizeAndPreserve( all_nvars, npoints);
      // points( 
      //   blitz::Range( nvars, all_nvars-1), 
      //   blitz::Range( 0, npoints-1)) = old_npoints;
      // points.reverseSelf( blitz::firstDim);

      // Update the number of variables
      nvars = all_nvars;

      // Add new columns of data to the old data buffer
      old_column_info.pop_back();
      for( unsigned int i=0; i<column_info.size(); i++) {
        old_column_info.push_back( column_info[ i]);
      }
      
      // Copy old data buffer to the current data buffer
      column_info = old_column_info;
    }

    // Free memory in case this isn't handled by the compiler.
    old_column_info.erase( old_column_info.begin(), old_column_info.end());
  }

  // If we read a different number of points then we anticipated, we must resize 
  // and preserve the current data buffer.  Note this can take lot of time and 
  // memory temporarily.  XXX it would be better to handle the growth/shrinkage 
  // of this buffer while reading.
  if( npoints != npoints_cmd_line)
    for( int j=0; j<nvars; j++)
      (column_info[j].points).resizeAndPreserve( npoints);

  // Now that we know the number of variables and points we've read, we can
  // allocate and/or reallocateResize the other global arrays.  NOTE: This 
  // will invoke reset_selection_arrays()
  resize_global_arrays();

  // Straighten out selection information.  WARNING: This will die horribly
  // if array sizes are not consistent
  if( doMerge) selected = old_selected;
  else if( doAppend) {
    int new_npoints = read_selected.rows();
    old_npoints = old_selected.rows();

    // Make sure arrays sizes are consistent
    if( npoints != new_npoints + old_npoints) {
        cerr << "Data_File_Manager::load_data_file: ERROR, "
             << "selection arrays aren't consistent!" << endl
             << "  old(" << old_npoints << ") + new(" << new_npoints
             << ") != total(" << npoints << ")" << endl;
    }
       
    selected( blitz::Range( 0, new_npoints-1)) = read_selected;
    selected( blitz::Range( new_npoints, npoints-1)) = old_selected;
    // selected( blitz::Range( 0, new_npoints-1)) = 
    //   read_selected( blitz::Range( 0, new_npoints-1));
    // selected( blitz::Range( new_npoints, npoints-1)) =
    //   old_selected( blitz::Range( 0, old_npoints-1));
  }
  else selected = read_selected;
  read_selected.free();
  old_selected.free();
  
  // Refresh edit window, if it exists.
  refresh_edit_column_info();

  // Update dataFileSpec, set saved file flag, and report success
  dataFileSpec = inFileSpec;
  isAsciiData = 1-inputFileType_;   // COnversion needed for legacy reasons
  if( doAppend > 0 | doMerge > 0) isSavedFile_ = 0;
  else isSavedFile_ = 1;
  return 0;
}

//***************************************************************************
// Data_File_Manager::extract_column_labels( sLine, doDefault) -- Generate 
// or extract column labels and selection flag and store them in the static 
// member vector of Column_Info objects
int Data_File_Manager::extract_column_labels( string sLine, int doDefault)
{
  // Initialize the static member vector of Column_Info objects
  int nLabels = 0;
  nvars = 0;
  column_info.erase( column_info.begin(), column_info.end());

  // If requested, examine the line, count the number of values, and use this
  // information to generate a set of default column labels.
  if( doDefault != 0) {
    
    // If the delimiter character is not a tab, replace all tabs in the
    // LINE string with spaces
    if( delimiter_char_ != '\t') replace( sLine.begin(), sLine.end(), '\t', ' ');

    // Loop: Insert the LINE string into a stream, define a buffer, read and
    // count successive tokens, generate default column labels and load the 
    // vector of Column_Info objects, and report results.  NOTE: whitespace-
    // and character-delimited files must be handled differently
    std::stringstream ss( sLine);
    std::string buf;
    Column_Info column_info_buf;
    if( delimiter_char_ == ' ') {
      while( ss >> buf) {
        nvars++;
        char cbuf[ 80];
        (void) sprintf( cbuf, "%d", nvars);
        buf = "Column_";
        buf.append( cbuf);
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    }
    else { 
      while( getline( ss, buf, delimiter_char_)) {
        nvars++;
        char cbuf[ 80];
        (void) sprintf( cbuf, "%d", nvars);
        buf = "Column_";
        buf.append( cbuf);
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    }
    nvars = column_info.size();
    cout << " -Generated " << nvars 
         << " default column labels." << endl;
  }

  // ...otherwise, examine the input line to extract column labels.
  else {

    // Discard the leading comment character, if any, of the SLINE string.
    // The rest of the line is assumed to contain column labels
    if( sLine.find_first_of( "!#%") == 0) sLine.erase( 0, 1);

    // Loop: Insert the SLINE string into a stream, define a buffer, read 
    // successive labels into the buffer, then load them into the vector of 
    // Column_Info objects, and report results.  NOTE: whitespace- and 
    // character-delimited labels must be handled differently.  Also, it is
    // necessary to trim whitespace and verify character-delimited labels.
    std::stringstream ss( sLine);
    std::string buf;
    Column_Info column_info_buf;
    if( delimiter_char_ == ' ')
      while( ss >> buf) {
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    else {
      while( getline( ss, buf, delimiter_char_)) {
        string::size_type notwhite = buf.find_first_not_of( " ");
        buf.erase( 0, notwhite);
        notwhite = buf.find_last_not_of( " \n");
        buf.erase( notwhite+1);
        if( buf.size() <= 0) buf = "Dummy";
        column_info_buf.label = string( buf);
        column_info.push_back( column_info_buf);
      }
    }
    nvars = column_info.size();
    cout << " -Extracted " << nvars << " column labels." << endl;
  }

  // If there were more than NVARS_CMD_LINE variables in the file, truncate 
  // the vector of Column_Info objects, reset NVARS, and warn the user.
  if( nvars_cmd_line > 0 && nvars > nvars_cmd_line) {
    column_info.erase( column_info.begin()+nvars_cmd_line, column_info.end());
    nvars = column_info.size();
    cerr << " -WARNING: Too many variables, truncated list to " << nvars 
         << " column labels." << endl;
  }

  // Examine the number of Column_Info objects that remain.  If it is too 
  // low or high, report error, close input file, and quit.  Otherwise 
  // report success.
  // if( nvars <= 1) {
  if( doMerge == 0 && nvars <= 1) {
    cerr << " -WARNING, insufficient number of columns (" << nvars
         << "), check for correct delimiter character"
         << endl;
    string sWarning = "";
    sWarning.append( "WARNING: Couldn't identify enough columns of data\n.");
    sWarning.append( "Check delimiter value and 'commented labels' setting.");
    make_confirmation_window( sWarning.c_str(), 1);
    return -1;
  }
  // if( nvars > MAXVARS) {
  if( nvars > maxvars_) {
    cerr << " -WARNING, too many data columns, "
         << "increase MAXVARS and recompile"
         << endl;
    make_confirmation_window( "WARNING: Too many data columns.", 1);
    return -1;
  }
  cout << " -Examined header of <" << inFileSpec.c_str() << ">," << endl
       << "  There should be " << nvars 
       << " fields (columns) per record (row)" << endl;

  // If requested, add a column to contain line numbers but don't increment
  // NVARS.  Note: this is old code thatmay no longer wirk
  Column_Info column_info_buf;
  if( include_line_number) {
    column_info_buf.label = string( "-line number-");
    column_info.push_back( column_info_buf);
  }
  
  // Add a final dummy column label that says '-nothing-' but don't 
  // increment NVARS.
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);

  // Update NLABELS to include the final dummy column label and report 
  // label information
  nLabels = column_info.size();
  cout << " -Read " << nLabels << "/" << nLabels;
  if( delimiter_char_ == ' ') cout << " whitespace-delimited ";
  else if( delimiter_char_ == ',') cout << " comma-delimited ";
  else cout << " custom-delimited ";
  cout << "column_labels:" << endl;

  // Clever output formatting to keep line lengths under control.  NOTE: 
  // This will misbehave if some label is more than 80 characters long.
  cout << "  ";
  int nLineLength = 4;
  for( unsigned int i=0; i < column_info.size(); i++ ) {
    nLineLength += 2+(column_info[ i].label).length();
    if( nLineLength > 80) {
      cout << endl << "  ";
      nLineLength = 4 + (column_info[ i].label).length();
    }
    cout << "  (" << column_info[ i].label << ")";
  }
  cout << endl;

  // Check the last column of data to see if it is the selection label, 
  // SELECTION_LABEL.  Note that since NVARS was not incremented, this 
  // will be the column BEFORE the new final column labelled '-nothing-'.
  readSelectionInfo_ = 0;
  if( (column_info[nvars-1].label).compare( 0, SELECTION_LABEL.size(), SELECTION_LABEL) == 0) {
    readSelectionInfo_ = 1;
    cout << "   -Read selection info-" << endl;
  }
  
  // Return the number of labels.  Note that this will include any 
  // additional non-data columns added after the NVARS data columns.
  return nLabels;
}

//***************************************************************************
// Data_File_Manager::extract_column_types( sLine) -- Examine a line of data
// to determine which columns contain ASCII values.  NOTE: This should be 
// consolidated with some other method to avoid duplicate code.
void Data_File_Manager::extract_column_types( string sLine)
{
  // If the delimiter character is not a tab, replace tabs with spaces
  if( delimiter_char_ != '\t')
    replace( sLine.begin(), sLine.end(), '\t', ' ');

  // Loop: Insert the string into a stream and read it
  std::stringstream ss( sLine); 
  unsigned isBadData = 0;
  string sToken;
  // double xValue;
  for( int j=0; j<nvars; j++) {
    
    // Get the next word.  NOTE: whitespace-delimited and character-
    // delimited files must be handled differently.  PROBLEM: This may not 
    // handle missing values correctly, if at all.
    if( delimiter_char_ == ' ') ss >> sToken;
    else {
      std::string buf;
      getline( ss, buf, delimiter_char_);
        
      // Check for missing data
      string::size_type notwhite = buf.find_first_not_of( " ");
      buf.erase( 0, notwhite);
      notwhite = buf.find_last_not_of( " ");
      buf.erase( notwhite+1);
      if( buf.size() <= 0) sToken = string( "BAD_VALUE_PROXY");
      else {
        stringstream bufstream;
        bufstream << buf;
        bufstream >> sToken;
      }
    }

    // Issue warning and quit if this line doesn't contain enough data
    if( ss.eof() && j<nvars-1) {
      cerr << " -WARNING, extract_column_types reports "
           << "not enough data on first line!" << endl
           << "  skipping entire line." << endl;
      isBadData = 1;
      break;
    }

    // Issue warning and quit if this line contains unreadable data.  NOTE:
    // This should never happen, because error flags were cleared above.
    if( !ss.good() && j<nvars-1) {
      cerr << " -WARNING, extract_column_types reports unreadable data "
           << "(binary or ASCII?) on first line at column " << j+1
           << "," << endl
           << "  skipping entire line." << endl;
      isBadData = 1;
      break;
    }
    
    // Attempt to use strod and examine values and pointers to determine if 
    // token can be parsed as a double.  This code doesn't work
    // int hasASCII = 0;
    // char **cEnd;
    // char cToken[80];
    // double xTest = std::strtod( sToken.c_str(), cEnd);
    // if( xValue == 0 && cEnd != &(sToken.c_str())) hasASCII = 1;
    // else hasASCII = 0;
    
    // Convert token to a stringstream and try to read it to determine if it
    // can be parsed as a double.  If the stringstream >> operator returns 0, 
    // the read failed and the token couldn't be parsed.  WARNING: It remains
    // to be determined if this works on all compilers!
    int hasASCII = 0;
    std::istringstream inpStream( sToken);
    double inpValue = 0.0;
    if( inpStream >> inpValue) hasASCII = 0;
    else hasASCII = 1;
    
    // Treat the string 'NaN' as numerical.  Yes, it might be better to 
    // convert sToken to uppercase, but I'm lazy
    if( sToken.compare( "NAN") == 0 || 
        sToken.compare( "NaN") == 0 ||
        sToken.compare( "nan") == 0) hasASCII = 0;

    // Load information into the vector of Column_Info objects
    column_info[ j].hasASCII = hasASCII;
  }
}

//***************************************************************************
// Data_File_Manager::remove_column_of_selection_info() -- If selection 
// information was found, remove the second-to-last column element in the 
// vector of Column_Info objects, which is assumed to contain the selection 
// label, SELECTION_LABEL, then update and return the number of data columns.
int Data_File_Manager::remove_column_of_selection_info()
{
  int nColumns = column_info.size();
  if( readSelectionInfo_) {
    int iTarget = nColumns-1;
    vector<Column_Info>::iterator pTarget = column_info.end();
    iTarget--;
    iTarget--;
    pTarget--;
    pTarget--;
    // if( include_line_number) {
    //   iTarget--;
    //   pTarget--;
    // }
    column_info.erase( pTarget);
    nColumns = column_info.size();
    cout << " -Removed column[" << iTarget << "/" << nColumns
         << "] with selection information" << endl;
  }
  return nColumns;
}

//***************************************************************************
// Data_File_Manager::read_ascii_file_with_headers() -- Reads and ASCII file.
// Step 1: Open an ASCII file for input.  Step 2: Read and discard the header
// block.  Step 3: Generate column labels.  Step 4: Read the data block.  
// Step 5: Close the file.  Returns 0 if successful.
int Data_File_Manager::read_ascii_file_with_headers() 
{
  istream* inStream;
  ifstream inFile;

  // STEP 1: Either read from stdin, bypassing openning of input file, or
  // attempt to open input file and make sure it exists.
  if( read_from_stdin) {
    inStream = &cin;
  }
  else {
    inFile.open( inFileSpec.c_str(), ios::in);
    if( inFile.bad() || !inFile.is_open()) {
      cerr << "read_ascii_file_with_headers:" << endl
           << " -ERROR, couldn't open <" << inFileSpec.c_str()
           << ">" << endl;
      return 1;
    }
    else {
      cout << "read_ascii_file_with_headers:" << endl
           << " -Opened <" << inFileSpec.c_str() << ">" << endl;
    }
    inStream = &inFile;
  }

  // STEP 2: Read and discard the header block, but save the last line of 
  // the header in the LASTHEADERLINE buffer

  // Loop: Read successive lines to find and store the last line of the 
  // header block. NOTE: Since tellg() and seekg() don't seem to work 
  // properly with getline() with all compilers, this must be accomplished 
  // by reading and counting each line explicitly.
  std::string line = "";
  std::string lastHeaderLine = "";
  int nRead = 0, nHeaderLines = 0;
  for( int iLine = 0; iLine < MAX_HEADER_LINES; iLine++) {
    if( inStream->eof() != 0) break;
    (void) getline( *inStream, line, '\n');
    nRead++;

    // Skip empty lines without updating the LASTHEADERLINE buffer
    if( line.length() == 0) {
      nHeaderLines++;
      continue;
    }
    
    // If this line is supposed to be skipped or if it begins with a comment 
    // character, skip it and update the LASTHEADERLINE buffer
    if( iLine < nSkipHeaderLines || 
        line.length() == 0 || line.find_first_of( "!#%") == 0) {
      lastHeaderLine = line;
      nHeaderLines++;
      continue;
    }
    break;
  }
  cout << " -Header block contains " << nHeaderLines 
       << " header lines." << endl;


  // STEP 3: Get column labels from either the last line of the header
  // block or the first line of the data block.  Under certain conditions,
  // generate default column labels.
  
  // If the DOCOMMENTEDLABELS_ flag is set, unset the UREADNEXTLINE flag to
  // preserve LINE as the first record in the data block, then examine the 
  // last line of the header block.  If this doesn't exist or is empty, 
  // generate default column labels, if it contains information, parse this 
  // to extract column labels.  Otherwise, parse the first line of the data 
  // block to extract column labels, and if these all seem to be numeric, 
  // replace them with default column labels and unset the UREADNEXTLINE 
  // flag as described above.
  int nLabels = 0;
  unsigned uReadNextLine = 1;
  if( doCommentedLabels_) {
    uReadNextLine = 0;
    if( nHeaderLines == 0 || lastHeaderLine.length() == 0)
      nLabels = extract_column_labels( line, 1);
    else nLabels = extract_column_labels( lastHeaderLine, 0);
  }
  else {
    nLabels = extract_column_labels( line, 0);
    extract_column_types( line);
    if( n_ascii_columns() <= 0 && nLabels > 0) {
      uReadNextLine = 0;
      extract_column_labels( line, 1);
    }
  }

  // If there were problems, close file and quit
  if( nLabels < 0 && !read_from_stdin) {
    cout << "Data_File_Manager::read_ascii_file_with_headers: "
         << "Couldn't identify any column labels." << endl;
    inFile.close();
    return 1;
  }


  // STEP 4: Read the data block
  
  // Now we know the number of variables, NVARS, so if we also know the 
  // number of points (e.g. from the command line) we can size the main 
  // points array once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  // else npoints = MAXPOINTS;
  else npoints = maxpoints_;
  nDataColumns_ = nvars;
  if( include_line_number) nDataColumns_++;  // Add column for line number
  if( readSelectionInfo_) nDataColumns_--;   // Don't store selection info
  for( int j=0; j<nDataColumns_; j++)
    (column_info[j].points).resize( npoints);
  
  // Loop: Read successive lines from the file
  int nSkip = 0;
  nDataRows_ = 0;
  int nTestCycle = 0, nUnreadableData = 0;
  while( !inStream->eof() && nDataRows_<npoints) {
  
    // Get the next line, check for EOF, and increment accounting information
    if( uReadNextLine) {
      (void) getline( *inStream, line, '\n');
      if( inStream->eof()) break;  // Break here to make accounting work right
      nRead++;
    }

    // Skip blank lines and comment lines
    if( line.length() == 0 || line.find_first_of( "!#%") == 0) {
      nSkip++;
      uReadNextLine = 1;
      continue;
    }
    uReadNextLine = 1;
    nTestCycle++;
    
    // Invoke member function to examine the first line of data to identify 
    // columns that contain ASCII values and load this information into the 
    // vector of column_info objects
    if( nDataRows_ == 0) extract_column_types( line);
    
    // If the delimiter character is not a tab, replace tabs with spaces
    if( delimiter_char_ != '\t')
      replace( line.begin(), line.end(), '\t', ' ');

    // Loop: Insert the string into a stream and read it
    std::stringstream ss( line); 
    unsigned isBadData = 0;
    double xValue;
    string sToken;
    for( int j=0; j<nvars; j++) {
    
      // Read the next word as double or a string, depending on whether or
      // not this column contains ASCII values.  NOTE: whitespace-delimited 
      // and character-delimited files must be handled differently.
      // PROBLEM: This isn't handling missing values correctly
      if( delimiter_char_ == ' ') {
        // if( column_info[j].hasASCII == 0) ss >> xValue;
        // else ss >> sToken;
        ss >> sToken;
        if( column_info[j].hasASCII == 0) {
          stringstream bufstream;
          bufstream << sToken;
          bufstream >> xValue;
          if( sToken.compare( 0, 3, "NaN") == 0) xValue = bad_value_proxy_;
        }
      }
      else {
        std::string buf;
        getline( ss, buf, delimiter_char_);
        
        // Check for missing data
        string::size_type notwhite = buf.find_first_not_of( " ");
        buf.erase( 0, notwhite);
        notwhite = buf.find_last_not_of( " ");
        buf.erase( notwhite+1);
        if( buf.size() <= 0) xValue = bad_value_proxy_;
        else {
          stringstream bufstream;
          bufstream << buf;
          if( column_info[j].hasASCII == 0) {
            bufstream >> xValue;
            if( buf.compare( 0, 3, "NaN") == 0) xValue = bad_value_proxy_;
          }
          else bufstream >> sToken;
        }
      }

      // Skip lines that don't appear to contain enough data
      if( ss.eof() && j<nvars-1) {
        cerr << " -WARNING, not enough data on line " << nRead
             << ", skipping this line!" << endl;
        isBadData = 1;
        break;
      }

      // If this was selection information, load it into the READ_SELECTED
      // vector, otherwise load it into the current data buffer.  In both 
      // cases inspect the SS stringstream to check for values of NULL that 
      // imply certain types of bad data and/or missing values and replace 
      // these with a default value and clear the error flags if necessary.
      if( !readSelectionInfo_ || j < nvars-1) {
        if( !ss) {
          column_info[j].points(nDataRows_) = bad_value_proxy_;
          ss.clear();
        }
        else {
          
          // If this is numerical data, load it directly, otherwise invoke
          // the member function of Column_Info to determine the order in
          // which ASCII values appeared and load that order as data.
          if( column_info[j].hasASCII == 0)
            column_info[j].points(nDataRows_) = (float) xValue;
          else
            column_info[j].points(nDataRows_) = column_info[j].add_value( sToken);
        }
      }
      else {
        if( !ss) ss.clear();
        else read_selected( nDataRows_) = (int) xValue;
      }
      
      // Check for unreadable data and flag this line to be skipped.  NOTE:
      // This should never happen, because error flags were cleared above.
      if( !ss.good() && j<nvars-1) {
        cerr << " -WARNING, unreadable data "
             << "(binary or ASCII?) at line " << nRead
             << " column " << j+1 << "," << endl
             << "  skipping entire line." << endl;
        nUnreadableData++;
        isBadData = 1;
        break;
      }
    }

    // Loop: Check for bad data flags and flag this line to be skipped
    for( int j=0; j<nDataColumns_; j++) {
      if( column_info[j].points(nDataRows_) < -90e99) {
        cerr << " -WARNING, bad data flag (<-90e99) at line " << nRead
             << ", column " << j << " - skipping entire line\n";
        isBadData = 1;
        break;
      }
    }

    // Check for too much unreadable data
    if( nTestCycle >= MAX_NTESTCYCLES) {
      if( nUnreadableData >= MAX_NUNREADABLELINES) {
        cerr << " -ERROR: " << nUnreadableData << " out of " << nTestCycle
             << " lines of unreadable data at line " << nDataRows_+1 << endl;
        sErrorMessage = "Too much unreadable data in an ASCII file";
        return 1;
      }
      nTestCycle = 0;
    }

    // If data were good, increment the number of lines
    if( !isBadData) {
      nDataRows_++;
      if( (nDataRows_+1)%10000 == 0)
        cerr << "  Read " << nDataRows_+1 << " rows of data." << endl;
    }
  }

  // DIAGNOSTIC: Examine column_info
  // cout << "data_file_manager::read_ascii_data: Column Information" << endl;
  // for( int j = 0; j < nvars; j++) {
  //   if( column_info[j].hasASCII>0) {
  //     cout << "column[" << j << "]:";
  //     for( 
  //       map<string,int>::iterator iter = (column_info[j].ascii_values_).begin();
  //       iter != (column_info[j].ascii_values_).end(); iter++) {
  //       cout << " (" << (iter->first).c_str() << "," << (iter->second) << ")";
  //     }
  //     cout << endl;
  //   }
  // }  
  
  // Check to see if the user specified that the line of column labels was 
  // commented and all columns were ASCII.  If this happened, it's possible 
  // that user made a mistake, and the column labels were actually in the
  // first uncommented line, so ask the user if this was intentional.  If
  // it wasn't, generate default data and quit so user can try again.
  if( doCommentedLabels_ != 0 && n_ascii_columns() >= nvars) {
    string sWarning = "";
    sWarning.append( "WARNING: All columns appear to be ASCII, as if\n");
    sWarning.append( "the line of column labels was left uncommented.\n");
    sWarning.append( "Do you wish to read it as is?");
    if( make_confirmation_window( sWarning.c_str(), 3, 3) <= 0) {
      return -1;
    }
  }
  
  // Loop: Examine the vector of Column_Info objects to alphabetize ASCII 
  // values and renumber the data.
  int nReordered = 0;
  npoints = nDataRows_;
  for( int j=0; j<nDataColumns_; j++) {
    if( column_info[j].update_ascii_values_and_data() >=0) nReordered++;
  }

  // DIAGNOSTIC: Examine column_info again
  // cout << "data_file_manager::read_ascii_data: Updated column Information" << endl;
  // for( int j = 0; j < nvars; j++) {
  //   if( column_info[j].hasASCII>0) {
  //     cout << "column[" << j << "]:";
  //     for( 
  //       map<string,int>::iterator iter = (column_info[j].ascii_values_).begin();
  //       iter != (column_info[j].ascii_values_).end(); iter++) {
  //       cout << " (" << (iter->first).c_str() << "," << (iter->second) << ")";
  //     }
  //     cout << endl;
  //   }
  // }

  // DIAGNOSTIC: Exercise access functions
  // for( int j=0; j<nvars; j++) {
  //   cout << "*COLUMN[ " << j << "]:";
  //   if( is_ascii_column(j) == 0) cout << " Numerical";
  //   else {
  //     for( int i=0; i<n_ascii_values(j); i++) {
  //       string sPooka = ascii_value(j,i);
  //       int iPooka = ascii_value_index(j,sPooka);
  //       cout << " (" << sPooka.c_str() << "," << iPooka << ")";
  //     }
  //   }
  //   cout << endl;
  // }

  // Check for and remove the column of selection information
  nDataColumns_ = nvars;
  cout << "read_ascii: ABOUT_TO_REMOVE (" << nDataColumns_ << "/" << nvars << ")" << endl;
  nDataColumns_ = remove_column_of_selection_info()-1;

  // STEP 5: Update NVARS and NPOINTS, resize current data buffers, and 
  // report results of the read operation to the console
  nvars = nDataColumns_;
  npoints = nDataRows_;
  for( int j=0; j<nvars; j++)
    (column_info[j].points).resizeAndPreserve( npoints);

  cout << " -Finished reading " << nvars << "x" << npoints
       << " data block with ";
  if( readSelectionInfo_ == 0) cout << "no ";
  else cout << " added column of ";
  cout << "selection information." << endl;
  cout << "  " << nHeaderLines 
       << " header + " << nDataRows_ 
       << " good data + " << nSkip 
       << " skipped lines = " << nRead << " total." << endl;

  // Close input file or stdin and report success
  if( !read_from_stdin) inFile.close();
  else read_from_stdin = false;
  return 0;
}

//***************************************************************************
// Data_File_Manager::read_binary_file_with_headers() -- Open and read a 
// binary file.  The file is asssumed to consist of a single header line of 
// ASCII with column information, terminated by a newline, followed by a block
// of binary data.  The only viable way to read this seems to be with 
// conventional C-style methods: fopen, fgets, fread, feof, and fclose, from 
// <stdio>.  Returns 0 if successful.
int Data_File_Manager::read_binary_file_with_headers() 
{
  // Attempt to open input file and make sure it exists
  FILE * pInFile;
  pInFile = fopen( inFileSpec.c_str(), "rb");
  if( pInFile == NULL) {
    cerr << "read_binary_file_with_headers: ERROR" << endl
         << " -Couldn't open binary file <" << inFileSpec.c_str() 
         << ">" << endl;
  }
  else {
    cout << "read_binary_file_with_headers:" << endl
         << " -Opening binary file <" << inFileSpec.c_str() 
         << ">" << endl;
  }

  // Use fgets to read a newline-terminated string of characters, test to make 
  // sure it wasn't too long, then load it into a string of header information.
  char cBuf[ MAX_HEADER_LENGTH];
  char *status = fgets( cBuf, MAX_HEADER_LENGTH, pInFile);
  if ( status == NULL ) {
       const char * msg = strerror(errno);
       cerr <<  "while reading file "
            <<  "<"  << inFileSpec.c_str() << "> "
            << msg << endl;
       fclose( pInFile);
       make_confirmation_window( msg, 1);
       return 1;
  } else if( strlen( cBuf) >= (int)MAX_HEADER_LENGTH) {
    cerr << " -ERROR: Header string is too long in "
         <<  "<" << inFileSpec.c_str() << ">, "
         << "increase MAX_HEADER_LENGTH and recompile"
         << endl;
    fclose( pInFile);
    make_confirmation_window( "ERROR: Header string is too long", 1);
    return 1;
  }

  std::string line;
  line.assign( cBuf);

  // There are two possibilities: the original Binary Format, in which case 
  // there is only one header line and all data are numerical, and Binary
  // With ASCII Values Format, in which case the first header line contains 
  // a type identifier, BINARY_FILE_WITH_ASCII_VALUES_LABEL, and the number 
  // of columns, followed by successive lines that contain column labels, 
  // column types, and lookup tables for any ASCII values that may be 
  // associated with data for that column.
  if( strstr( line.c_str(), BINARY_FILE_WITH_ASCII_VALUES_LABEL.c_str())) {

    // Initialize the static member vector of Column_Info objects
    int nLabels = 0;
    nvars = 0;
    column_info.erase( column_info.begin(), column_info.end());

    // For the sake of consistency, everything is tab-delimited
    char this_delimiter_ = '\t';

    // Extract the number of columns from the first (tab-delimited) line
    std::stringstream ss( line);
    std::string sToken;
    getline( ss, sToken, this_delimiter_);  // Discard 1st token (identifier)
    getline( ss, sToken, this_delimiter_);  // Extract 2nd token (nColumns)
    std::istringstream inpStream( sToken);
    // int nCR = sToken.find_first_of( '\n');
    // if( nCR > 0) sToken = sToken.substr( 0, nCR);
    int nColumns = 0;
    inpStream >> nColumns;
    
    // Loop: Read successive tab-delimited lines and extract column info
    for( int i=0; i<nColumns; i++) {
      char * status = fgets( cBuf, MAX_HEADER_LENGTH, pInFile);
      if ( status == NULL ) {
           const char * msg = strerror(errno);
           cerr <<  "while reading file "
                <<  "<"  << inFileSpec.c_str() << "> "
                << msg << endl;
           fclose( pInFile);
           make_confirmation_window( msg, 1);
           return 1;
      }
      
      std::string sLine;
      sLine.assign( cBuf);
      std::stringstream sLineBuf( sLine);

      // Get label, then generate a new Column_Info object
      std::string buf;
      getline( sLineBuf, buf, this_delimiter_);
      Column_Info column_info_buf;
      column_info_buf.label = string( buf);
      
      // Get the column type (not used)
      getline( sLineBuf, buf, this_delimiter_);
      int nCR = buf.find_first_of( '\n');
      if( nCR > 0) buf = buf.substr( 0, nCR);
      
      // Examine the rest of the line to extract ASCII values, if any
      int nValues = 0;
      column_info_buf.hasASCII = 0;
      while( getline( sLineBuf, buf, this_delimiter_)) {
        nValues++;
        int nCR = buf.find_first_of( '\n');
        if( nCR > 0) buf = buf.substr( 0, nCR);
        column_info_buf.hasASCII = 1;
        column_info_buf.add_value( buf);
      }

      // Add the new Column_Info object to the list of Column_Info objects
      nvars++;
      column_info.push_back( column_info_buf);
    }
    
    // Check the last column of data to see if it is the selection label, 
    // SELECTION_LABEL.
    readSelectionInfo_ = 0;
    if( (column_info[nvars-1].label).compare( 0, SELECTION_LABEL.size(), SELECTION_LABEL) == 0) {
      readSelectionInfo_ = 1;
      cout << "   -Read selection info-" << endl;
    }

    // Add a final dummy column label that says '-nothing-' but don't 
    // increment NVARS.
    Column_Info column_info_buf;
    column_info_buf.label = string( "-nothing-");
    column_info.push_back( column_info_buf);

    // Report status to the console
    nLabels = column_info.size();
    cout << " -About to read " << nvars
         << " variables from a "
         << BINARY_FILE_WITH_ASCII_VALUES_LABEL.c_str()
         << " with " << nLabels 
         << " fields (columns) per record (row)" << endl;
  }
  else {

    // Save existing delimiter character, then examine the line.  If it 
    // contains tabs, temporarily set the deliminer character to a tab.
    // Otherwise set it to whitespace.
    char saved_delimiter_char_ = delimiter_char_;
    if( line.find( '\t') == std::string::npos || line.size() <= line.find( '\t')) {
      cout << " -Header is WHITESPACE delimited" << endl;
      delimiter_char_ = ' ';
    }
    else {
      cout << " -Header is TAB delimited" << endl;
      delimiter_char_ = '\t';
    }

    // Invoke the extract_column_labels method to extract column labels,
    // then reset the delimiter character
    int nLabels = 0;
    nLabels = extract_column_labels( line, 0);
    delimiter_char_ = saved_delimiter_char_;
    
    // Report status to the console
    cout << " -About to read " << nvars
         << " variables from a binary file with " << nLabels
         << " fields (columns) per record (row)" << endl;
  }

  // Now we know the number of variables (nvars), so if we know the number of 
  // points (e.g. from the command line) we can size the main points array 
  // once and for all, and not waste memory.
  if( npoints_cmd_line > 0) npoints = npoints_cmd_line;
  // else npoints = MAXPOINTS;
  else npoints = maxpoints_;
  int nDataColumns_ = nvars;
  if( include_line_number) nDataColumns_++;   // Add column for line number
  if( readSelectionInfo_) nDataColumns_--;    // Don't store selection info
  
  // Check for problems, then resize current buffer
  int n_column_info = column_info.size();
  // if( nvars > MAXVARS || nDataColumns_ > MAXVARS ||
  //     n_column_info > MAXVARS || nDataColumns_ > n_column_info) {
  if( nvars > maxvars_ || nDataColumns_ > maxvars_ ||
      n_column_info > maxvars_ || nDataColumns_ > n_column_info) {
    cerr << " -WARNING, too many data columns, "
         << "restoring original data"
         << endl;
    return 1;
  }
  for( int j=0; j<nDataColumns_; j++)
    (column_info[j].points).resize( npoints);
    
  // Warn if the input buffer is non-contiguous.
  // if( !points.isStorageContiguous()) {
  //   cerr << "read_binary_file_with_headers: WARNING" << endl
  //        << " -Input buffer appears to be non-contigous."
  //        << endl;
  //   // return 1;
  // }

  // Assert possible types or ordering  
  // assert( ordering == COLUMN_MAJOR || ordering == ROW_MAJOR);

  // Read file in Column Major order (e.g., row by row)...
  if( isColumnMajor == 1) {
    cout << " -Attempting to read binary file in column-major order" << endl;
         
    // Define input buffers and make sure they're contiguous
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR: Tried to read into a noncontiguous buffer."
           << endl;
      fclose( pInFile);
      make_confirmation_window( 
        "ERROR: Tried to read into a noncontiguous buffer", 1);
      return -1;
    }

    // Loop: Read up to NPOINTS successive rows from file
    for( int i=0; i<npoints; i++) {
    
      // Read the next NVAR values using conventional C-style fread.
      unsigned int ret = 
        fread( (void *)(vars.data()), sizeof(float), nvars, pInFile);
      
      // Check for normal termination
      if( ret == 0 || feof( pInFile)) {
        cerr << " -Finished reading file at row[ " << i
             << "] with ret, inFile.eof() = ( " << ret
             << ", " << feof( pInFile) << ")" << endl;
        break;
      }
      
      // If wrong number of values was returned, report error.
      if( ret != (unsigned int) nvars) {
        cerr << " -ERROR reading row[ " << i+1 << "], "
             << "returned values " << ret 
             << " NE number of variables " << nvars << endl;
        fclose( pInFile);
        make_confirmation_window( "Error reading row of binary data", 1);
        return 1;
      }

      // Load data array and report progress
      if( !readSelectionInfo_) {
        for( int j=0; j<nvars; j++) column_info[j].points(i) = vars(j);
      }
      else {
        for( int j=0; j<nvars-1; j++) column_info[j].points(i) = vars(j);
        read_selected( i) = (int) vars( nvars-1);
      }

      nDataRows_ = i;
      if( i>0 && (i%10000 == 0)) 
        cout << "  Reading row " << nDataRows_ << endl;
    }

    // Update number of rows and report success
    npoints = nDataRows_ + 1;
    cout << " -Finished reading " << npoints << " rows of data." << endl;
  }

  // ...or read file in Row Major order (e.g., column-by-column).  NOTE: For
  // this to work, we must know the correct value of NPOINTS
  else {
    cout << " -Attempting to read binary file in row-major order "
         << "with nvars=" << nvars
         << ", npoints=" << npoints << endl;
    if( npoints_cmd_line == 0) {
      cerr << " -ERROR, --npoints must be specified for"
           << " --inputformat=rowmajor"
           << endl;
      fclose( pInFile);
      make_confirmation_window( 
        "ERROR: NPOINTS must be specified for ROWMAJOR binary files", 1);
      return 1;
    }
    else {
      npoints = npoints_cmd_line;
    }

    // Define input buffer and make sure it's contiguous
    blitz::Array<float,1> vars( npoints);
    blitz::Range NPTS( 0, npoints-1);
    if( !vars.isStorageContiguous()) {
      cerr << " -ERROR, Tried to read into noncontiguous buffer."
           << endl;
      fclose( pInFile);
      make_confirmation_window( 
        "ERROR: Tried to read into noncontiguous buffer", 1);
      return -1;
    }

    // Loop: Read successive columns from the file
    int i;
    for( i=0; i<nvars; i++) {

      // Read the next NVAR values using conventional C-style fread.
      unsigned int ret = 
        fread( (void *)(vars.data()), sizeof(float), npoints, pInFile);

      // Check for normal termination
      if( ret == 0 || feof( pInFile)) {
        cerr << " -Finished reading file at row[ " << i
             << "] with ret, inFile.eof() = ( " << ret
             << ", " << feof( pInFile) << ")" << endl;
        break;
      }
      
      // If wrong number of values was returned, report error.
      if( ret != (unsigned int)npoints) {
        cerr << " -ERROR reading column[ " << i+1 << "], "
             << "returned values " << ret 
             << " NE number of variables " << nvars << endl;
        fclose( pInFile);
        make_confirmation_window( "ERROR reading column of binary file", 1);
        return 1;
      }

      // Load data array and report progress
      if( !readSelectionInfo_ && i < nvars-1) column_info[i].points = vars;
      else {
        column_info[i].points = vars;
        // read_selected( NPTS) = blitz::cast( vars( NPTS), int());
        // read_selected( NPTS) = cast<int>(vars(NPTS));
        for( int j=0; j<npoints; j++) read_selected( j) = (int) vars( j);
      }
      cout << "  Reading column " << i+1 << endl;
    }
    
    // Update number of rows and report success
    nDataRows_ = npoints;
    cout << " -Finished reading " << i+i
         << " columns" << endl;
  }
  
  // Check for and remove the column of selection information
  nDataColumns_ = nvars;
  cout << "read_binary: ABOUT_TO_REMOVE (" << nDataColumns_ << "/" << nvars << ")" << endl;
  nDataColumns_ = remove_column_of_selection_info()-1;

  // Update NVARS and resize current data buffers
  nvars = nDataColumns_;
  for( int j=0; j<nvars; j++)
    (column_info[j].points).resizeAndPreserve( npoints);

  // Close input file and report success
  fclose( pInFile);
  return 0;
}

//***************************************************************************
// Data_File_Manager::read_table_from_fits_file() -- Open a FITS file and 
// read a table extension.  Note that unlike ASCII and binary file reads,
// this method stores selection information in the current data buffer, from
// which is must be removed by the calling method.  Returns 0 if successful.
int Data_File_Manager::read_table_from_fits_file()
{
  // Attempt to open input file and make sure it exists
  fitsfile *pFitsfile;
  int status=0;
  if( fits_open_file( &pFitsfile, inFileSpec.c_str(), READONLY, &status)) {
    cerr << "read_table_from_fits_file: ERROR" << endl
         << " -Couldn't open FITS file <" << inFileSpec.c_str() 
         << "> with status (" << status << ")" << endl;
    string sWarning = "";
    sWarning.append( "Couldn't open file as FITS file, check format.\n");
    sWarning.append( "Restoring original data.");
    make_confirmation_window( sWarning.c_str(), 1, 2);
    return 1;
  }
  else {
    cout << "read_table_from_fits_file:" << endl
         << " -Opening FITS file <" << inFileSpec.c_str() 
         << ">" << endl;
  }

  // Loop: Locate first ASCII table extension
  int hdutype;
  int iExt = -1;
  for( int i = 2; !( fits_movabs_hdu( pFitsfile, i, &hdutype, &status)); i++) {

    // DIAGNOSTIC
    // cout << "Data_File_Manager::read_table_from_fits_file: "
    //      << "Examining HDU[" << i
    //      << "] with status (" << status << ")" << endl;

    // Is this an ASCII table extension?
    if( hdutype == ASCII_TBL) {
      iExt = i;
      break;
    }
  }
  if( status || iExt < 0) {
    cerr << "read_table_from_fits_file: ERROR" << endl
         << " -Couldn't locate table extension "
         << "with status (" << status << ")" << endl;
    string sConfirm = "";
    sConfirm.append( "Couldn't locate ASCII_TBL in FITs file,\n");
    sConfirm.append( "check format.  Restoring original data.");
    make_confirmation_window( sConfirm.c_str(), 1, 2);
    return 1;
  }

  // DIAGNOSTIC
  // cout << "Data_File_Manager::read_table_from_fits_file: "
  //      << "Finished examining HDUs with iExt (" << iExt
  //      << ") and status (" << status << ")" << endl;

  // Initialize the static member vector of Column_Info objects
  int nLabels = 0;
  nvars = 0;
  column_info.erase( column_info.begin(), column_info.end());

  // Examine header to get array size
  long nrows;
  int ncols;
  if( fits_get_num_rows( pFitsfile, &nrows, &status) ||
      fits_get_num_cols( pFitsfile, &ncols, &status)) {
    cerr << "read_table_from_fits_file: ERROR" << endl
         << " -Couldn't get array size "
         << "with status (" << status << ")" << endl;
    string sConfirm = "";
    sConfirm.append( "Couldn't get array size from HD of FITs file,\n");
    sConfirm.append( "check format.  Restoring original data.");
    make_confirmation_window( sConfirm.c_str(), 1, 2);
    return 1;
  }
  npoints = (int) nrows;
  nDataRows_ = (int) nrows;
  nvars = ncols;
  blitz::Range NPTS( 0, npoints-1);

  // Make sure we don't have too many columns
  if( nvars > maxvars_) {
    cerr << " -WARNING, too many data columns, "
         << "increase MAXVARS and recompile"
         << endl;

    char cBuf[ 80];
    sprintf(
      cBuf, "WARNING: Too many data columns ( %i > %i).\n", nvars, maxvars_);
    string sWarning = "";
    sWarning.append( cBuf);
    sWarning.append( "Restoring old data.");
    make_confirmation_window( sWarning.c_str(), 1);
    return -1;
  }

  // Make sure we don't have too many rows
  if( npoints > maxpoints_) {
    cerr << " -WARNING, too many rows of data, "
         << "increase MAXPOINTS and recompile"
         << endl;

    char cBuf[ 80];
    sprintf(
      cBuf, "WARNING: Too many data points ( %i > %i).\n", npoints, maxpoints_);
    string sWarning = "";
    sWarning.append( cBuf);
    sWarning.append( "Restoring old data.");
    make_confirmation_window( sWarning.c_str(), 1);
    return -1;
  }

  // Resize current data buffer to make room for data
  // int n_column_info = nvars+1;
  // if( readSelectionInfo_) n_column_info--;
  // for( int j=0; j<n_column_info; j++)
  //   (column_info[j].points).resize( npoints);

  // Report HDU type to the console for diagnostic purposes
  if( hdutype == ASCII_TBL)
    cout << "Data_File_Manager::read_table_from_fits_file: "
         << "ASCII table extension, (" << npoints << "x" << nvars
         << ")" << endl;
  else if( hdutype == BINARY_TBL)
    cout << "Data_File_Manager::read_table_from_fits_file: "
         << "binary table extension, (" << npoints << "x" << nvars
         << ")" << endl;
  else
    cout << "Data_File_Manager::read_table_from_fits_file: "
         << "WARNING, unrecognized hdutype (" << hdutype << ")" << endl;
  
  // Loop: Extract column names, using the fact that fits_get_colname 
  // always returns the next name that matches the template.
  ncols = 0;
  status = 0;
  for( int i=0; i<nvars; i++) {
    char cname[ 80];
    int icol;
    char pattern[sizeof("*")];
    strcpy(pattern,"*");
    fits_get_colname( pFitsfile, CASEINSEN, pattern, cname, &icol, &status);

    // DIAGNOSTIC
    // cout << "Data_File_Manager::read_table_from_fits_file: "
    //      << "column[" << i << "].name (" << cname
    //      << ") with status (" << status << ")" << endl;

    if( status == COL_NOT_FOUND) break;

    Column_Info column_info_buf;
    column_info_buf.label = string( cname);
    column_info.push_back( column_info_buf);
    ncols++;
    
    // DIAGNOSTIC
    // cout << "Data_File_Manager::read_table_from_fits_file: "
    //      << "column[" << i << "].label ("
    //      << (column_info[i].label).c_str()
    //      << ")" << endl;
  }
  if( ncols < nvars) {
    cerr << "read_table_from_fits_file: ERROR" << endl
         << " -Couldn't find enough columns (" << ncols << "/" << nvars
         << ") with status (" << status << ")" << endl;
    string sConfirm = "";
    sConfirm.append( "Couldn't find enough columns in FITS table,\n");
    sConfirm.append( "check format.  Restoring original data.");
    make_confirmation_window( sConfirm.c_str(), 1, 2);
    return 1;
  }
  
  // Examine last column to see if it contains selection information
  readSelectionInfo_ = 0;
  if( (column_info[nvars-1].label).compare( 0, SELECTION_LABEL.size(), SELECTION_LABEL) == 0) {
    readSelectionInfo_ = 1;
    cout << "   -Read selection info-" << endl;
  }

  // Resize current data buffer to make room for data
  // int n_column_info = nvars+1;
  // if( readSelectionInfo_) n_column_info--;
  // for( int j=0; j<n_column_info; j++)
  //   (column_info[j].points).resize( npoints);
  nDataColumns_ = nvars;
  for( int j=0; j<nvars; j++)
    (column_info[j].points).resize( npoints);

  // If requested, add a column to contain line numbers
  Column_Info column_info_buf;
  // if( include_line_number) {
  //   column_info_buf.label = string( "-line number-");
  //   column_info.push_back( column_info_buf);
  // }
  
  // Add a final column label that says 'nothing'.
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);

  // Report label information
  nLabels = column_info.size();
  cout << " -Read " << nvars << "/" << nLabels;
  cout << " FITS table extension ";
  cout << "column_labels:" << endl;

  // Clever output formatting to keep line lengths under control.  NOTE: 
  // This will misbehave if some label is more than 80 characters long.
  cout << "  ";
  int nLineLength = 4;
  for( unsigned int i=0; i < column_info.size(); i++ ) {
    nLineLength += 2+(column_info[ i].label).length();
    if( nLineLength > 80) {
      cout << endl << "  ";
      nLineLength = 4 + (column_info[ i].label).length();
    }
    cout << "  " << column_info[ i].label;
  }
  cout << endl;

  // DIAGNOSTIC
  // cout << "Data_File_Manager::read_table_from_fits_file: "
  //      << "DIAGNOSTIC: About to allocate storage with npoints ("
  //      << npoints << ")" << endl;

  // Apparently one cannot use NEW and DELETE to allocate and deallocate 
  // arrays of char strings for use with fits_read_col.  This code works 
  // once, then dies during some subsequent use.  It is included for 
  // archival purposes.
  // char strnull[]="*";
  // char **cstring_array = NULL;
  // int npoints_alloc = npoints;
  // int nchars = 1000;
  // cstring_array = new char*[npoints_alloc];
  // for( int i=0; i<npoints_alloc; i++)
  //   cstring_array[i] = new char[nchars];

  // Allocate storage to read arrays of character strings.  WARNING: It 
  // appears that one MUST use MALLOC to allocate arrays of char strings for 
  // use with fits_read_col.  This code is extrememly delicate!
  char strnull[]="*";
  char **cstring_array = NULL;
  int npoints_alloc = npoints;
  int nchars = 1000;
  cstring_array = (char**) malloc( npoints_alloc * (sizeof *cstring_array));
  // cout << "DIAGNOSTIC: Allocated storage for " << npoints_alloc
  //      << " pointers" << endl;

  for( int i=0; i<npoints_alloc; i++) {
    // cout << "  -Allocate[" << i << "/" << npoints_alloc
    //      << "] with " << nchars << " chars" << endl;
    cstring_array[i] = (char*) malloc(nchars + 1);
  }

  // DIAGNOSTIC
  // cout << "Data_File_Manager::read_table_from_fits_file: "
  //      << "About to examine successive columns" << endl;

  // Loop: Read successive columns
  status = 0;
  for( int colnum=1; colnum<=nvars; colnum++) {

    // DIAGNOSTIC
    // cout << "Data_File_Manager::read_table_from_fits_file: "
    //      << "About to call fits_get_coltype for column["
    //      << colnum << "]" << endl;

    // Get column info
    int typecode;
    long repeat, width;
    if( fits_get_coltype(
          pFitsfile, colnum, &typecode, &repeat, &width, &status)) {
      cerr << "read_table_from_fits_file: ERROR" << endl
           << " -Couldn't find type for column[" << colnum << "/" << nvars
           << "] with status (" << status << ")" << endl;
      string sConfirm = "";
      sConfirm.append( "Couldn't find typecode for column in FITS table.\n");
      sConfirm.append( "Check format.  Restoring original data.");
      make_confirmation_window( sConfirm.c_str(), 1, 2);
      return 1;
    }

    // DIAGNOSTIC
    // cout << "Data_File_Manager::read_table_from_fits_file: "
    //      << "column[" << colnum-1 << "].type (" << typecode
    //      << "/" << TFLOAT << ")" << endl;

    // Read column info
    int frow = 1, felem = 1;
    int nelem = npoints;
    int anynull;
    if( typecode == TSHORT) {
      short shortnull = 0;
      // short *shortarray = new short[ npoints];
      short* shortarray;
      // shortarray = (short*) malloc( 100);
      shortarray = (short*) malloc( npoints * sizeof(short));
      fits_read_col(
        pFitsfile, typecode, colnum, frow, felem, nelem,
        &shortnull, shortarray, &anynull, &status);
      // blitz::Array<short,2>
      //   abuffer( shortarray, blitz::shape(1,npoints), blitz::duplicateData);
      // column_info[colnum-1].points(NPTS) = blitz::cast( abuffer( 1, NPTS), float());
      // for( int j=0; j<npoints; j++) column_info[colnum-1].points(j) = (float) abuffer(j);
      // for( int j=0; j<npoints; j++) column_info[colnum-1].points(j) = shortarray[j];
      for( int j=0; j<npoints; j++) column_info[colnum-1].points(j) = shortarray[j];
      // delete[] shortarray;
      free( shortarray);
    }
    if( typecode == TLONG) {
      long longnull = 0;
      long* longarray;
      longarray = (long*) malloc( npoints * sizeof(long));
      fits_read_col(
        pFitsfile, typecode, colnum, frow, felem, nelem,
        &longnull, longarray, &anynull, &status);
      for( int j=0; j<npoints; j++)
        column_info[colnum-1].points(j) = (float) longarray[j];
      free( longarray);
    }
    else if( typecode == TFLOAT) {
      float floatnull = 0;
      float* floatarray;
      floatarray = (float*) malloc( npoints * sizeof(float));
      fits_read_col(
        pFitsfile, typecode, colnum, frow, felem, nelem,
        &floatnull, floatarray, &anynull, &status);
      blitz::Array<float,2>
        abuffer( floatarray, blitz::shape(1,npoints), blitz::duplicateData);
      column_info[colnum-1].points(NPTS) = abuffer( 0, NPTS);
      free( floatarray);
    }
    else if( typecode == TDOUBLE) {
      double doublenull = 0;
      double* doublearray;
      doublearray = (double*) malloc( npoints * sizeof(double));
      fits_read_col(
        pFitsfile, typecode, colnum, frow, felem, nelem,
        &doublenull, doublearray, &anynull, &status);
      for( int j=0; j<npoints; j++)
        column_info[colnum-1].points(j) = (float) doublearray[j];
      free( doublearray);
    }
    else {

      // DIAGNOSTIC
      // cout << "Data_File_Manager::read_table_from_fits_file: "
      //      << "reading columnstring[" << colnum << "] with "
      //      << nelem << "/" << npoints << " elements" << endl;

      fits_read_col_str(
        pFitsfile, colnum, frow, felem, nelem,
        strnull, cstring_array, &anynull, &status);
      for( int j=0; j<npoints; j++) {
        // cout << "  -sToken[" << j << "] (" << cstring_array[j] << ")" << endl;
        string sToken = string( cstring_array[j]);
        column_info[colnum-1].points(j) =
          column_info[colnum-1].add_value( sToken);
      }
      column_info[colnum-1].hasASCII = 1;
    }

    // If this is selection information, load selection array
    if( readSelectionInfo_ != 0 && colnum == nvars)
      for( int j=0; j<npoints; j++)
        read_selected(j) = (int) column_info[colnum-1].points(j);
    
    // DIAGNOSTIC
    cout << "Data_File_Manager::read_table_from_fits_file: "
         << "Reading column " << colnum << endl;
    // for( int j=0; j<npoints; j++) {
    //   cout << " -VALUE[ " << colnum-1 << ", " << j
    //        << "] (" << points(colnum-1,j) << ")" << endl;
    // }
  }

  // Remember to deallocate space for the array of character strings used
  // during read operations.  WARNING: As noted above, it appears that one
  // MUST use MALLOC and FREE to allocate and deallocate arrays of char
  // strings for use with fits_read_col.  Like the memory allocation, this 
  // code is extremely delicate!
  for( int i=0; i<npoints_alloc; i++) {
    // cout << "  -Deallocate[" << i << "/" << npoints_alloc << "]" << endl;
    free( cstring_array[i]);
  }
  free( cstring_array);

  // As noted above, it appears one can't use NEW and DELETE to allocate 
  // and deallocate arrays of char strings for use with fits_read_col.  
  // This code is included for archival purposes.
  // for( int i=0; i<npoints_alloc; i++) delete[] cstring_array[i];
  // free( cstring_array);

  // Update number of rows and report success
  nDataRows_ = npoints;
  cout << " -Finished reading " << nvars
       << " columns" << endl;
  
  // If the current output buffer contains selection information, the
  // value of NVARS will be too high and it will be necessary to 
  // decrement NVARS and resize the current data buffers.  The selection
  // information is assumed to be in the second-to-last column, which 
  // must be recognized and deleted by the calling method.
  // XXX_PRG: For some reason, both of these lines are essential!  I'm
  // not quite sure why the resize operation is needed.
  // if( readSelectionInfo_ != 0) nvars = nvars-1;
  // for( int j=0; j<nvars; j++)
  //   (column_info[j].points).resizeAndPreserve( npoints);
  // nDataColumns_ = nvars;

  // Check for and remove the column of selection information
  nDataColumns_ = nvars;
  nDataColumns_ = remove_column_of_selection_info()-1;
  nvars = nDataColumns_;
  for( int j=0; j<nvars; j++)
    (column_info[j].points).resizeAndPreserve( npoints);

  // Close input file and terminate
  fits_close_file( pFitsfile, &status);
  return 0;
}

//***************************************************************************
// Data_File_Manager::findOutputFile() -- Query user to find the output file.
// Class Vp_File_Chooser is used in preference to the Vp_File_Chooser method 
// to obtain access to member functions such as directory() and to allow the 
// possibility of a derived class with additional controls in the 
// file_chooser window.  Returns 0 if successful.  
int Data_File_Manager::findOutputFile()
{
  // Generate query text and list file extensions, etc for this file type
  string title;
  string pattern;
  if( writeAllData_ != 0) title = "Write all data to file";
  else title = "Write selected data to file";
  if( outputFileType_ == 0) pattern = "*.{txt,lis,asc}\tAll Files (*)";
  else if( outputFileType_ == 1) pattern = "*.bin\tAll Files (*)";
  else if( outputFileType_ == 2) pattern = "*.{fit,fits}\tAll Files (*)";
  else pattern = "*.bin\tAll Files (*)";

  // Initialize output filespec.  NOTE: cOutFileSpec is defined as const 
  // char* for use with Vp_File_Chooser, which means it could be destroyed 
  // by the relevant destructors!
  const char *cOutFileSpec = sDirectory_.c_str();

  // Instantiate and show an Vp_File_Chooser widget.  NOTE: The pathname 
  // must be passed as a variable or the window will begin in some root 
  // directory.
  Vp_File_Chooser* file_chooser = 
    new Vp_File_Chooser( 
      cOutFileSpec, pattern.c_str(), Vp_File_Chooser::CREATE, title.c_str());
  file_chooser->directory( sDirectory_.c_str());
  file_chooser->fileType( outputFileType_);

  // Loop: Select succesive filespecs until a non-directory is obtained
  while( 1) {
    // if( cOutFileSpec != NULL) file_chooser->directory( cOutFileSpec);

    // Loop: wait until the selection is done, then extract the value.  NOTE: 
    // This usage of while and Fl::wait() seems strange.
    file_chooser->show();
    while( file_chooser->shown()) Fl::wait();
    cOutFileSpec = file_chooser->value();   

    // If no file was specified then quit
    if( cOutFileSpec == NULL) break;

    // If this is a new file, it can't be opened for read
    int isNewFile = 0;
    FILE* pFile = fopen( cOutFileSpec, "r");
    if( pFile == NULL) isNewFile= 1;
    else fclose( pFile);
    
    // Attempt to open an output stream to make sure the file can be opened 
    // for write.  If it can't, assume that cOutFileSpec was a directory and 
    // make it the working directory.  Otherwise close the output stream.
    // NOTE: If the file exists, it will be opened for append, then closed.
    // Otherwise this will create and close an empty file.
    #ifdef __WIN32__
      ofstream os;
      // os.open( cOutFileSpec, ios::out|ios::trunc);
      os.open( cOutFileSpec, ios::out|ios::app);
      if( os.fail()) {
        cerr << " -DIAGNOSTIC: This should trigger on error opening "
             << cOutFileSpec << "for write" << endl;
        file_chooser->directory( cOutFileSpec);
        directory( (string) cOutFileSpec);
        os.close();
        continue;
      }
      os.close();
      if( isNewFile != 0) break;
    #endif // __WIN32__

    // Presumably this test should be OS-independant
    if( isNewFile != 0) break;

    // OLD CODE: If this is a new file, it can't be opened for read, and 
    // we're done
    // FILE* pFile = fopen( cOutFileSpec, "r");
    // if( pFile == NULL) break;
    
    // OLD CODE:We're done examining the file, so close it
    // fclose( pFile);

    // If we got this far, the file must exist and be available to be
    // overwritten, so open a confirmation window and wait for the button 
    // handler to do something.
    string sConfirmText = "Data file already exists\n'";
    sConfirmText.append( cOutFileSpec);
    sConfirmText.append( "'\nOverwrite existing file?\n");
    int iConfirmResult = make_confirmation_window( sConfirmText.c_str(), 3, 3);

    // If this was a 'CANCEL' request, return without doing anything.  If 
    // this was a 'YES' request, move on.  Otherwise, make sure we're in
    // the right directory and try again.
    if( iConfirmResult < 0) return -1;
    if( iConfirmResult > 0) break;
  } 

  // Obtain file name using the FLTK member function.  This code doesn't work, 
  // but is retained as a comment for descriptive purposes.
  // char *cOutFileSpec = 
  //   Vp_File_Chooser( "write ASCII output to file", NULL, NULL, 0);

  // Get file type and content information
  outputFileType_ = file_chooser->fileType();
  delimiter_char_ = file_chooser->delimiter_char();
  if( file_chooser->writeSelectionInfo() != 0) writeSelectionInfo_ = 1;
  else writeSelectionInfo_ = 0;
  if( file_chooser->doCommentedLabels() != 0) doCommentedLabels_ = 1;
  else doCommentedLabels_ = 0;

  // Load outFileSpec
  int iResult = 0;
  if( cOutFileSpec == NULL) {
    outFileSpec = "";
    cout << "Data_File_Manager::findOutputFile: "
         << "closed with no output file specified" << endl;
    iResult = -1;
  }
  else{
    outFileSpec.assign( (string) cOutFileSpec);
    if( outputFileType_ == 0) 
      cout << "Data_File_Manager::findOutputFile: Writing ASCII data to <";
    else if( outputFileType_ == 1) 
      cout << "Data_File_Manager::findOutputFile: Writing binary data to <";
    else if( outputFileType_ == 2) 
      cout << "Data_File_Manager::findOutputFile: Writing FITS extension to <";
    else
      cout << "Data_File_Manager::findOutputFile: Writing binary data to <";
    cout << outFileSpec.c_str() << ">" << endl;
    iResult = 0;
  }

  // Make sure thepathname has been updated!
  directory( (string) file_chooser->directory());

  // Deallocate the Vp_File_Chooser object
  delete file_chooser;  // WARNING! This destroys cOutFileSpec!

  // Report result
  return iResult;
}

//***************************************************************************
// Data_File_Manager::save_data_file( outFileSpec) -- Write ASCII or binary 
// data file to disk.  Returns 0 if successful.
int Data_File_Manager::save_data_file( string outFileSpec)
{
  output_filespec( outFileSpec);
  return save_data_file();
}

//***************************************************************************
// Data_File_Manager::save_data_file() -- Write ASCII or binary data file to 
// disk.  Returns 0 if successful.
int Data_File_Manager::save_data_file()
{
  int result = 0;
  if( outputFileType_ == 0) result = write_ascii_file_with_headers();
  else if( outputFileType_ == 2) result = write_table_to_fits_file();
  else result = write_binary_file_with_headers();
  if( result == 0) {
    isSavedFile_ = 1;
    dataFileSpec = outFileSpec;
    // isAsciiData = (outputFileType_ == 0);
    isAsciiData = 1-outputFileType_;
  }
  return result;
}

//***************************************************************************
// Data_File_Manager::write_ascii_file_with_headers() -- Open and write an 
// ASCII data file.  File will consist of an ASCII header with column names 
// terminated by a newline, followed by successive lines of ASCII data.
// Returns 0 if successful.
int Data_File_Manager::write_ascii_file_with_headers()
{
  // Make sure a file name was specified, create and write the file
  if( outFileSpec.length() <= 0){
    cout << "Data_File_Manager::write_ascii_file_with_headers "
         << "reports that no file was specified" << endl;
    return -1;
  }
  else {
    blitz::Array<float,1> vars( nvars);
    blitz::Range NVARS( 0, nvars-1);
    
    // Open output stream and report any problems
    ofstream os;
    os.open( outFileSpec.c_str(), ios::out|ios::trunc);
    if( os.fail()) {
      cerr << " -ERROR opening " << outFileSpec.c_str() 
           << " for ASCII write" << endl;
      return -1;
    }
    
    // Write output file name (and additional information?) to the header
    os << "! File Name: " << outFileSpec.c_str() << endl;
    
    // Do not write out "line-number" column (header or data)
    // it gets created automatically when a file is read in
    int nvars_out = include_line_number?nvars-1:nvars;

    // Loop: Write comment character to include column labels to the header
    char first_char = ' ';
    if( doCommentedLabels_) first_char = '!';
    for( int i=0; i < nvars_out; i++ ) {
      if( i == 0) os << first_char << setw( 12) << column_info[ i].label;
      else os << delimiter_char_ << " " << setw( 13) << column_info[ i].label;
      // else os << " " << setw( 13) << column_info[ i].label;
    }
    if( writeSelectionInfo_ != 0) os << delimiter_char_ << " " << SELECTION_LABEL;
    os << endl;
    
    // Loop: Write successive ASCII records to the data block using the
    // "default" floatfield format.  This causes integers to be written as
    // integers, floating point as floating point, and numbers with large or 
    // small magnitude as scientific floats. 
    os.precision( 14); 
    os.unsetf( ios::scientific); // force floatfield to default
    int rows_written = 0;
    for( int irow = 0; irow < npoints; irow++) {
      if( writeAllData_ != 0 || selected( irow) > 0) {
        for( int jcol = 0; jcol < nvars_out; jcol++) {
          // if( jcol > 0) os << " ";
          if( jcol > 0) os << delimiter_char_ << " ";
          
          // Process numerical and ASCII values differently
          if( column_info[jcol].hasASCII == 0)
            os << column_info[jcol].points(irow);
          else
            os << column_info[jcol].ascii_value( (int) column_info[jcol].points(irow));
        }
        if( writeSelectionInfo_ != 0) os << delimiter_char_ << " " << selected( irow);
        os << endl;
        rows_written++;
      }
    }

    // Report results
    cout << "wrote " << rows_written << " rows of " << nvars 
         << " variables to ascii file " << outFileSpec.c_str() << endl;
  }
  return 0;
}

//***************************************************************************
// Data_File_Manager::write_binary_file_with_headers() -- Open and write a 
// binary data file.  File will consist of an ASCII header with column names 
// terminated by a newline, followed by a long block of binary data.  Returns
// 0 if successful.
int Data_File_Manager::write_binary_file_with_headers()
{
  // Make sure a file name was specified, create and write the file
  if( outFileSpec.length() <= 0){
    cout << "Data_File_Manager::write_binary_file_with_headers "
         << "reports that no file was specified" << endl;
    return -1;
  }
  else {

    // Do not write out "line-number" column (header or data)
    // it gets created automatically when a file is read in
    int nvars_out = include_line_number?nvars-1:nvars;

    blitz::Array<float,1> vars( nvars_out);
    blitz::Range NVARS( 0, nvars_out-1);
    
    // Open output stream and report any problems
    ofstream os;
    os.open( 
      outFileSpec.c_str(), 
      ios::out|ios::trunc|ios::binary);
      // fstream::out | fstream::trunc | fstream::binary);

    if( os.fail()) {
      cerr << " -ERROR opening " << outFileSpec.c_str() 
           << "for binary write" << endl;
      return -1;
    }

    // If ASCII values were present, write a Binary With ASCCII Values format
    // file beginning a tab-separated line that contains the type specifier,
    // BINARY_FILE_WITH_ASCII_VALUES_LABEL and number of columns followed by 
    // successice tab-separated lines of lines of column info.  Otherwise 
    // write an old-style file.  Note: Everything is tab-delimited
    if( n_ascii_columns() > 0) {

      // Define number of columns for output, including selection info
      int nColumns = nvars_out;
      if( writeSelectionInfo_ != 0) nColumns++;
              
      // Write type specifier followed by number of columns
      os << BINARY_FILE_WITH_ASCII_VALUES_LABEL.c_str() << '\t' << nColumns;
      os << endl;

      // Loop: Write one line for each column of data
      for( int i=0; i<nvars_out; i++) {
        os << (column_info[i].label).c_str() << '\t';

        // Write the data type (not used) and ASCII values, if any
        if( column_info[i].hasASCII <= 0) {
          os << "TFLOAT";
        }
        else {
          os << "TSTRING" << '\t';
          int nValues = n_ascii_values(i);
          for( int j=0; j<nValues; j++) {
            os << (column_info[i].ascii_value(j)).c_str();
            if( j < nValues-1) os << '\t';
          }
        }
        os << endl;
      }

      // If selection info are to be written, add an additional column
      if( writeSelectionInfo_ != 0)
        os << SELECTION_LABEL << '\t' << "TLONG" << endl;
    }
    else {

      // Loop: Write tab-delimited column labels to the header.  Each label 
      // is followed by a space to make sure the file can be read by early
      // versions of viewpoints that didn't look for tabs.
      for( int i=0; i < nvars_out; i++ ) {
        os << column_info[ i].label << " ";
        if( i<nvars-1) os << '\t';
      }
      if( writeSelectionInfo_ != 0) os << '\t' << " " << SELECTION_LABEL;
      os << endl;
    }

    // Loop: Write data and report any problems
    int nBlockSize = nvars*sizeof(float);
    int rows_written = 0;
    for( int i=0; i<npoints; i++) {
      if( writeAllData_ != 0 || selected( i) > 0) {
        for( int j=0; j<nvars; j++) vars(j) = column_info[j].points(i);
        os.write( (const char*) vars.data(), nBlockSize);
        if( writeSelectionInfo_ != 0) {
          float fselection = (float) (selected(i));
          os.write((const char*)&fselection, sizeof(float));
        }
        if( os.fail()) {
          cerr << "Error writing to" << outFileSpec.c_str() << endl;
          string sWarning = "";
          sWarning.append( "WARNING: Error writing to file\n");
          sWarning.append( outFileSpec);
          make_confirmation_window( sWarning.c_str(), 1);
          return 1;
        }
      }
      rows_written++;
    }
    
    // Report results
    cout << "wrote " << rows_written << " rows of " << nBlockSize 
         << " bytes to binary file " << outFileSpec.c_str() << endl;
  }
  return 0;
}

//***************************************************************************
// Data_File_Manager::write_table_to_fits_file() -- Create a FITS file and 
// write a table extension.  Returns 0 if successful.
int Data_File_Manager::write_table_to_fits_file()
{
  // Define and set maximum length for ASCII variables.  This should 
  // eventually be made a static class variable
  int max_length_ascii_variable = 20;
  
  // Define string to hold filespec for debudding purposes
  // string sFileSpec = "H:/cppdir/vp.fits240/bowser.fit";
  string sFileSpec;
  sFileSpec = outFileSpec;

  // Define pointer to the FITS file along with status buffer.
  fitsfile *pFitsfile;
  int status;

  // Make sure a file name was specified, create and write the file
  if( sFileSpec.length() <= 0){
    cout << "Data_File_Manager::write_table_to_fits_file "
         << "reports that no file was specified" << endl;
    return -1;
  }
  else {
    // The findOutputFile method tries to open the output file to see if it
    // exists and verify that it's a file rathar than a directory.  This 
    // will produce an empty file, which must be deleted because the FITS 
    // routines cannot handle empty files.  The following rigamarole is 
    // necessary to delete this file under Windows and account for cases
    // when it has been opened by some earlier write operation.  A better
    // alternatives might be to examine the FITS code to see why it leaves
    // open handles, or to modify the findOutputFile method to switch off
    // the chacking procedure for FITS files.

    // Attempt to delete file.  If this fails, open it, make sure it's 
    // closed, call fits_delete_file, and try again.
    if( 0 != unlink( sFileSpec.c_str())) {
      cout << "Data_File_Manager::write_table_to_fits_file: " << endl
           << "  WARNING: Couldn't unlink: " << sFileSpec.c_str()
           << " due to: " << strerror(errno) << endl;
      status = 0;
      fits_open_file( &pFitsfile, sFileSpec.c_str(), READWRITE, &status);
      cout << "  Called fits_open_file with status (" << status
           << ")" << endl;
      status = 0;
      fits_close_file( pFitsfile, &status);
      cout << "  Called fits_close_file with status (" << status
           << ")" << endl;
      status = 0;
      fits_delete_file( pFitsfile, &status);
      cout << "  Called fits_delete_file with status (" << status
           << ")" << endl;
      if( 0 != unlink( sFileSpec.c_str()))
        cout << "  WARNING: Second unlink: " << sFileSpec.c_str()
             << " failed with: " << strerror(errno) << endl;
      else
        cout << "Data_File_Manager::write_table_to_fits_file: " << endl
             << "  Second unlink of " << sFileSpec.c_str()
             << " succeeded" << endl;
    }
    else
        cout << "Data_File_Manager::write_table_to_fits_file: " << endl
             << "  First unlink of " << sFileSpec.c_str()
             << " succeeded" << endl;

    status=0;
    fits_open_file( &pFitsfile, sFileSpec.c_str(), READWRITE, &status);
    cout << "DIAGNOSTIC: Finished initial attempt to open FITS file with status ("
         << status << ") vs (" << FILE_NOT_OPENED << ")" << endl;

    // If file already exists, destroy, recreate, and open it
    if( status == FILE_NOT_OPENED) {    
      remove( sFileSpec.c_str());
      status = 0;
      fits_create_file( &pFitsfile, sFileSpec.c_str(), &status);
      cout << "DIAGNOSTIC: Finished attempt to create FITS file with ("
           << status << ") vs (0)" << endl;
      status=0;
      fits_open_file( &pFitsfile, sFileSpec.c_str(), READWRITE, &status);
    }
    
    // If an error occured, close file and quit
    if( status == 0) {
      cout << "Data_File_Manager::write_table_to_fits_file: "
           << "Opened (" << sFileSpec.c_str()
           << ") as FITS file for output." << endl;
    }
    else {
      if( status != FILE_NOT_OPENED) fits_close_file( pFitsfile, &status);
      cerr << "Data_File_Manager::write_table_to_fits_file: ERROR, "
         << "couldn't open ( "<< sFileSpec.c_str()
         << " for output with status (" << status
         << ")" << endl;
      return -1;
    }
  }

  // Specify parameters of output file
  int nrows = npoints;
  int tfields = nvars;
  if( writeSelectionInfo_ != 0) tfields = tfields+1;
  cout << "DIAGNOSTIC: writeSelectionInfo_ (" << writeSelectionInfo_
       << "), tfields (" << tfields << "/" << nvars << ")" << endl;

  // Generate ascii format string, which must be less that 10 chars long!
  char tform_ascii[10] = "";
  (void) sprintf( tform_ascii, "a%i", max_length_ascii_variable);
  
  // Define, allocate, and load arrays to store field information  
  char **ttype;
  char **tform;
  char **tunit;
  ttype = (char**) malloc( tfields * (sizeof *ttype));
  tform = (char**) malloc( tfields * (sizeof *tform));
  tunit = (char**) malloc( tfields * (sizeof *tunit));
  for( int i=0; i<tfields; i++) {
    if( i<nvars) {
      ttype[i] = (char*) malloc( (column_info[i].label).size() + 1);
      tform[i] = (char*) malloc( 10);
      tunit[i] = (char*) malloc( 1);
      strcpy( ttype[i], (column_info[i].label).c_str());
      if( column_info[i].hasASCII <= 0) strcpy( tform[i], "E14.6");
      else strcpy( tform[i], tform_ascii);
      strcpy( tunit[i], "\0");
    }
    else {
      ttype[i] = (char*) malloc( 18);
      tform[i] = (char*) malloc( 3);
      tunit[i] = (char*) malloc( 1);
      strcpy( ttype[i], SELECTION_LABEL.c_str());
      strcpy( tform[i], "I8");
      strcpy( tunit[i], "\0");
    }
  }
  char extname[] = "VP_OUTPUT_ASCII";
  
  // Append a new empty ASCII table onto the FITS file
  if(
    fits_create_tbl( 
      pFitsfile, ASCII_TBL, nrows, tfields, ttype, tform, tunit,
      extname, &status)) {
    fits_close_file( pFitsfile, &status);
    cerr << "Data_File_Manager::write_table_to_fits_file: ERROR, "
         << "couldn't append table extension with status (" << status
         << ")" << endl;
    return -1;
  }

  // Deallocate arrays with field information
  for( int i=0; i<tfields; i++) {
    free(ttype[i]);
    free(tform[i]);
    free(tunit[i]);
  }
  free(ttype);
  free(tform);
  free(tunit);

  // Set position for first row and column.  Note that 'firstelem' will be
  // ignored for ASCII tables
  int firstrow  = 1;
  int firstelem = 1;

  // Allocate space for output arrays
  float* floatarray;
  long* longarray;
  floatarray = (float*) malloc( npoints * sizeof(float));
  longarray = (long*) malloc( npoints * sizeof(long));

  // Allocate storage to read arrays of character strings.  WARNING: It 
  // appears that one MUST use MALLOC to allocate arrays of char strings 
  // for use with fits_read_col.  This code is extrememly delicate!
  char **cstring_array = NULL;
  int npoints_alloc = npoints;
  int nchars = max_length_ascii_variable+1;
  cstring_array = (char**) malloc( npoints_alloc * (sizeof *cstring_array));
  for( int i=0; i<npoints_alloc; i++)
    cstring_array[i] = (char*) malloc(nchars + 1);

  // Loop: Write attributes to their respective columns
  int ifield = 0;
  for( int i=0; i<tfields; i++) {
    ifield = i+1;
    if( i<nvars) {
      
      // Load and write non-ASCII data as float, otherwise look up and write
      // ASCII values
      if( column_info[i].hasASCII <= 0) {
        for( int j=0; j<npoints; j++) floatarray[j] = column_info[i].points(j);
        fits_write_col(
          pFitsfile, TFLOAT, ifield, firstrow, firstelem, npoints,
          floatarray, &status);
      }
      else {
        for( int j=0; j<npoints; j++)
          strcpy( cstring_array[j], (column_info[i].ascii_value(j)).c_str());
        fits_write_col(
          pFitsfile, TSTRING, ifield, firstrow, firstelem, npoints,
          cstring_array, &status);
      }
    }
    else {
      cout << "DIAGNOSTIC: Saving selection information" << endl;
      for( int j=0; j<npoints; j++) longarray[j] = selected( j);
      fits_write_col(
        pFitsfile, TLONG, ifield, firstrow, firstelem, npoints,
        longarray, &status);
    }
  }

  // Deallocate space
  free( floatarray);
  free( longarray);

  // Remember to deallocate space for the array of character strings used
  // during read operations.  WARNING: As noted above, it appears that one
  // MUST use MALLOC and FREE to allocate and deallocate arrays of char
  // strings for use with fits_read_col.  Like the memory allocation, this 
  // code is extremely delicate!
  for( int i=0; i<npoints_alloc; i++) free( cstring_array[i]);
  free( cstring_array);

  // Report success to console
  cout << "Data_File_Manager::write_table_to_fits_file: "
       << "finished writing (" << npoints << "x" << ifield
       << ") array to table extension in" << endl
       << "  FITS file (" << sFileSpec.c_str()
       << ")" << endl; 

  // Close file and report success
  fits_close_file( pFitsfile, &status);
  cout << "Data_File_Manager::write_table_to_fits_file: "
       << "closed file with status (" << status << ")" << endl;
  return 0;
}

//***************************************************************************
// Data_File_Manager::edit_column_info( *o) -- Static function to wrap the
// callback function, edit_column_info, that builds and manages the 
// 'Tools|Edit Column Labels' window.
void Data_File_Manager::edit_column_info( Fl_Widget *o)
{
  // DIAGNOSTIC
  cout << endl << "DIAGNOSTIC: Called Data_File_Manager::edit_column_info" << endl;

  // Old attempts to trace back the right number of generations
  // edit_column_info_i( o);
  // ( (Data_File_Manager*)
  //  (o->parent()->parent()->user_data()))->edit_column_info_i( o);
  // ( (Data_File_Manager*)
  //  (o->parent()->parent()))->edit_column_info_i( o);

  // Be sure to trace back the right number of generations
  ((Data_File_Manager*)(o->parent()))->edit_column_info_i( o);
}

//***************************************************************************
// Data_File_Manager::edit_column_info_i( *o) -- Callback function that
// builds and manages the 'Tools|Edit Column Labels' window window.
void Data_File_Manager::edit_column_info_i( Fl_Widget *o)
{
  if( edit_labels_window != NULL) edit_labels_window->hide();

  // Create the 'Tools|Edit Column Labels' window
  Fl::scheme( "plastic");  // optional
  edit_labels_window = new Fl_Window( 250, 305, "Edit Column Labels");
  edit_labels_window->begin();
  edit_labels_window->selection_color( FL_BLUE);
  edit_labels_window->labelsize( 10);
  
  // Write warning in box at top of window
  Fl_Box* warningBox = new Fl_Box( 5, 5, 240, 20);
  warningBox->label( "Warning: this will reset axes \nselections, and scaling");
  warningBox->align( FL_ALIGN_INSIDE|FL_ALIGN_LEFT);

  // Set an invisible box to control resize behavior
  Fl_Box *box = new Fl_Box( 5, 35, 240, 220);
  box->box( FL_NO_BOX);
  // box->box( FL_ROUNDED_BOX);
  edit_labels_window->resizable( box);

  // Define Fl_Check_Browser widget
  edit_labels_widget = new Fl_Check_Browser( 5, 35, 240, 220, "");
  edit_labels_widget->labelsize( 14);
  edit_labels_widget->textsize( 12);
  
  // Load column labels into browser
  refresh_edit_column_info();

  // Invoke callback function to delete labels
  Fl_Button* delete_button = new Fl_Button( 10, 270, 100, 25, "&Delete labels");
  delete_button->callback( (Fl_Callback*) delete_labels, edit_labels_widget);

  // Invoke callback function to quit and close window
  Fl_Button* quit = new Fl_Button( 160, 270, 70, 25, "&Quit");
  quit->callback( (Fl_Callback*) close_edit_labels_window, edit_labels_window);

  // Done creating the 'Tools|Edit Column_Labels' window.  
  edit_labels_window->end();
  // edit_labels_window->set_modal();   // This window shouldn't be modal
  edit_labels_window->show();
}

//***************************************************************************
// Data_File_Manager::refresh_edit_column_info() -- Make sure edit window
// exists, then refresh list of column labels
void Data_File_Manager::refresh_edit_column_info()
{
  if( edit_labels_window == NULL) return;
  edit_labels_widget->clear();
  for( int i=0; i<nvars; i++)
    edit_labels_widget->add( (column_info[ i].label).c_str());
}

//***************************************************************************
// Data_File_Manager::delete_labels( *o, *user_data) -- Callback function to 
// delete columns and their associated labels.  NOTE: Since data and labels
// are stored in separate structures, it is important to be careful that
// their contents remain consistent!
void Data_File_Manager::delete_labels( Fl_Widget *o, void* user_data)
{
  // DIAGNOSTIC
  cout << "Data_File_Manager::delete_labels: checked "
       << edit_labels_widget->nchecked() << "/"
       << edit_labels_widget->nitems() << " items" << endl;
  for( int i=0; i<nvars; i++) {
    cout << "Label[ " << i << "]: (" << column_info[i].label << ") ";
    if( edit_labels_widget->checked(i+1)) cout << "CHECKED";
    cout << endl;
  }

  // If no boxes were checked then quit
  int nChecked = edit_labels_widget->nchecked();
  int nRemain = nvars - nChecked;
  if( nChecked <= 0) return;
  if( nRemain <=1) {
    make_confirmation_window(
      "WARNING: Attempted to delete too many columns", 1);
    return;
  }

  // Check the dimensions of the data array and vector of Column_Info
  // objects to make sure we began with one more column label ('-nothing')
  // than the number of variables.
  int nColumnInfos = column_info.size();
  if( nColumnInfos != nvars+1) {
     cerr << "WARNING: Data_File_Manager::delete_labels was called with " 
          << nColumnInfos
          << " columns and a final label of (" 
          << (column_info[nColumnInfos].label).c_str()
          << ") but only " << nvars << " attributes" << endl;
  }
  
  // Move and resize data and column labels.  Do this inside the same loop 
  // to reduce the chance of doing it wrong.  NOTE: What should be done with
  // the array of ranked points used to perform scaling and normalization?
  blitz::Range NPOINTS( 0, npoints-1);
  int ivar = 0;
  for( int i=0; i<nvars; i++) {
    if( edit_labels_widget->checked(i+1) <= 0) {
      column_info[ivar].isRanked = 0;

      Column_Info column_info_buf;  // Why is this necessary?
      column_info_buf = column_info[i];
      column_info[ivar] = column_info_buf;
      ivar++;
    }
  }
  nvars = ivar;
  for( int i=0; i<nvars; i++) {
    (column_info[i].points).resizeAndPreserve(npoints);
    (column_info[i].ranked_points).resizeAndPreserve(npoints);
  }
  
  column_info.resize( nvars);
  Column_Info column_info_buf;
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);

  // Check the dimensions of the data array and vector of Column_Info
  // objects to make sure we finished with one more column label 
  // ('-nothing') than the number of variables.
  nColumnInfos = column_info.size();
  if( nColumnInfos != nvars+1) {
     cerr << "WARNING: Data_File_Manager::delete_labels finished with " 
          << nColumnInfos
          << " columns and a final label of (" 
          << (column_info[nColumnInfos].label).c_str()
          << ") but only " << nvars << " attributes" << endl;
  }
  
  // Clear and update menu
  edit_labels_widget->clear();
  for( int i=0; i<nvars; i++)
    edit_labels_widget->add( (column_info[ i].label).c_str());

  // Set flag so the idle callback, cb_manage_plot_window_array, in MAIN 
  // will know to do a Restore Panels operation!
  needs_restore_panels_ = 1;

  // ANOTHER DIAGNOSTIC
  // cout << "Data_File_Manager::delete_labels: finished with "
  //      << edit_labels_widget->nitems() << " items" << endl;
  // for( int i=0; i<nvars; i++) {
  //   cout << "Label[ " << i << "]: (" << column_info[i].label << ") ";
  //   cout << endl;
  // }

  // DIAGNOSTIC
  cout << "Data_File_Manager::delete_labels: finished with "
       << "needs_restore_panels (" << needs_restore_panels_ << ")" << endl;
}

//***************************************************************************
// Data_File_Manager::close_edit_labels_window( *o, *user_data) -- Callback 
// function to close the Edit Column Labels window.  It is assumed that a 
// pointer to the window will be passed as USER_DATA.  WARNING: No error 
// checking is done on USER_DATA!
void Data_File_Manager::close_edit_labels_window( Fl_Widget *o, void* user_data)
{
  ((Fl_Window*) user_data)->hide();
}

//***************************************************************************
// Data_File_Manager::remove_trivial_columns -- Examine an array of data and 
// remove columns for which all values are identical.  Part of the read 
// process.
void Data_File_Manager::remove_trivial_columns()
{
  blitz::Range NPTS( 0, npoints-1);
  int nvars_save = nvars;
  int current=0;

  // Define buffers to record removed columns
  int iRemoved = 0;
  vector <int> removed_columns;

  // Loop: Examine the data array column by colums and remove any columns for 
  // which all values are identical.
  while( current < nvars-1) {
    if( blitz::all( column_info[current].points(NPTS) == column_info[current].points(0))) {
      cout << "skipping trivial column " 
           << column_info[ current].label << endl;
      for( int j=current; j<nvars-1; j++) column_info[ j] = column_info[ j+1];
      removed_columns.push_back( iRemoved);
      nvars--;
      assert( nvars>0);
    }
    else {
      current++;
    }
    iRemoved++;
  }

  // Finish resizing array and column labels and report results
  if( nvars != nvars_save) {
      
    // Report what columns were removed
    cout << "Removed " << nvars_save - nvars << " columns:";
    for( unsigned int i=0; i<removed_columns.size(); i++) {
      int nLineLength = 8;
      nLineLength += 1+(column_info[ i].label).length();
      if( nLineLength > 80) {
        cout << endl << "   ";
        nLineLength = 2 + (column_info[ i].label).length();
      }
      cout << " " << column_info[ i].label;
    }
    cout << endl;
    
    // Resize array and report results
    // XXX need to trim column_info to size nvars+1 
    for( int i=0; i<nvars; i++)
      (column_info[i].points).resizeAndPreserve(npoints);
    column_info[ nvars].label = string( "-nothing-");
    cout << "new data array has " << nvars
         << " columns." << endl;
  }
}

//***************************************************************************
// Data_File_Manager::resize_global_arrays -- Resize various all the global 
// arrays used store raw, sorted, and selected data.
void Data_File_Manager::resize_global_arrays()
{
  blitz::Range NPTS( 0, npoints-1);
  
  // If line numbers are to be included as another field (column) of data 
  // array, create them as the last column.
  if( include_line_number) {
    for (int i=0; i<npoints; i++) {
      column_info[nvars-1].points(i) = (float)(i+1);
    }
  }
  
  // Resize and reinitialize list of ranked points to reflect the fact that 
  // no ranking has been done.
  for( int i=0; i<nvars; i++) {
    (column_info[i].ranked_points).resize(npoints);
    column_info[i].isRanked = 0;
  }
  
  // Resize and reinitialize selection related arrays and flags.
  inside_footprint.resize( npoints);
  newly_selected.resize( npoints);
  selected.resize( npoints);
  previously_selected.resize( npoints);
  saved_selection.resize(npoints);
  Plot_Window::indices_selected.resize(NBRUSHES,npoints);
  reset_selection_arrays();
}

//***************************************************************************
// Data_File_Manager::create_default_data( nvars_in) -- Load data arrays with 
// default data consisting of dummy data.
void Data_File_Manager::create_default_data( int nvars_in)
{
  // Protect against screwy values of nvars_in
  if( nvars_in < 2) return;
  nvars = nvars_in;
  // if( nvars > MAXVARS) nvars = MAXVARS;
  if( nvars > maxvars_) nvars = maxvars_;

  // Loop: Initialize and load the column labels, including the final label 
  // that says 'nothing'.
  column_info.erase( column_info.begin(), column_info.end());
  Column_Info column_info_buf;
  for( int i=0; i<nvars; i++) {
    ostringstream buf;
    buf << "default_"
        << setw( 3) << setfill( '0')
        << i
        << setfill( ' ') << " ";
    column_info_buf.label = string( buf.str());
    column_info.push_back( column_info_buf);
  }
  column_info_buf.label = string( "-nothing-");
  column_info.push_back( column_info_buf);

  // Report column labels
  cout << " -column_labels:";
  int nLineLength = 17;
  for( unsigned int i=0; i < column_info.size(); i++ ) {
    nLineLength += 1+(column_info[ i].label).length();
    if( nLineLength > 80) {
      cout << endl << "   ";
      nLineLength = 4 + (column_info[ i].label).length();
    }
    cout << " " << column_info[ i].label;
  }
  cout << endl;
  cout << " -Generated default header with " << nvars
       << " fields" << endl;

  // Resize data array to avoid the madening frustration of segmentation 
  // errors!  Important!
  npoints = 3;
  for( int i=0; i<nvars; i++)
    (column_info[i].points).resize(npoints);

  // Loop: load each variable with 0 and 1.  These two loops are kept separate
  // for clarity and to facilitate changes
  for( int i=0; i<nvars; i++) {
    column_info[i].points( 0) = 0.0;
    column_info[i].points( 1) = 0.5;
    column_info[i].points( 2) = 1.0;
  }

  // Resize global arrays
  resize_global_arrays();

  // Report results
  cout << "Generated default data with " << npoints 
       << " points and " << nvars << " variables" << endl;
}


//***************************************************************************
// Data_File_Manager::ascii_value( jcol, ival) -- Get ASCII value ival for
// column jcol.
string Data_File_Manager::ascii_value( int jcol, int ival)
{
  if( column_info[jcol].hasASCII == 0) return string( "NUMERIC_VALUE_VP");
  if( 0>jcol || jcol >= nvars) return string( "BAD_COLUMN_INDEX_VP");
  return column_info[jcol].ascii_value(ival);
}

//***************************************************************************
// Data_File_Manager::ascii_value_index( jcol, sToken) -- Index of sToken in
// column jcol.  String is passed by value for efficiency.
int Data_File_Manager::ascii_value_index( int jcol, string &sToken)
{
  if( column_info[jcol].hasASCII == 0) return -1;
  if( 0>jcol || jcol >= nvars) return -1;
  map<string,int>::iterator iter = (column_info[jcol].ascii_values_).find(sToken);
  if( iter != (column_info[jcol].ascii_values_).end()) return iter->second;
  return -1;
}

//***************************************************************************
// Data_File_Manager::directory() -- Get pathname.
string Data_File_Manager::directory()
{
  return sDirectory_; 
}
     
//***************************************************************************
// Data_File_Manager::directory( sDirectory_in) -- Make sure that any old 
// pathname is deallocated, then set the pathname.
void Data_File_Manager::directory( string sDirectory_in)
{
  sDirectory_.erase( sDirectory_.begin(), sDirectory_.end());
  // sDirectory_.append( sDirectory_);
  sDirectory_ = sDirectory_in;
}

//***************************************************************************
// Data_File_Manager::input_filespec() -- Get input filespec.
string Data_File_Manager::input_filespec()
{
  return inFileSpec;
}

//***************************************************************************
// Data_File_Manager::input_filespec( outFileSpecIn) -- Set input_filespec.
void Data_File_Manager::input_filespec( string inFileSpecIn)
{
  inFileSpec = inFileSpecIn;
}

//***************************************************************************
// Data_File_Manager::inputFileType() -- Get input file type.
int Data_File_Manager::inputFileType()
{
  return inputFileType_;
}

//***************************************************************************
// Data_File_Manager::inputFileType( i) -- Set input file type.
void Data_File_Manager::inputFileType( int i)
{
  inputFileType_ = i;
  if( inputFileType_ < 0) inputFileType_ = 0;
  if( inputFileType_ > 2) inputFileType_ = 2;
}

//***************************************************************************
// Data_File_Manager::maxpoints() -- Set maximum number of points (rows) and
// make sure it's reasonable.
void Data_File_Manager::maxpoints( int i)
{
  if( i < npoints) {
    if( make_confirmation_window(
          "This will delete some points.\n  Do you wish to continue?") <= 0)
    return;
  }
  
  maxpoints_ = i;
  if( maxpoints_ < 2) maxpoints_ = 2;

  // If necessary, shrink the current buffer  
  if( npoints > maxpoints_) {
    npoints = maxpoints_;
    for( int i=0; i<nvars; i++)
      (column_info[i].points).resizeAndPreserve(npoints);
      
    // Selection arrays should be resized as well
  }
}

//***************************************************************************
// Data_File_Manager::maxvars() -- Set maximum number of variables (columns)
// and make sure it's reasonable.
void Data_File_Manager::maxvars( int i)
{
  if( i < nvars) {
    if( make_confirmation_window(
          "This will delete some columns.\n  Do you wish to continue?") <= 0)
    return;
  }

  maxvars_ = i;
  if( maxvars_ < 2) maxvars_ = 2;
  
  // If necessary, nuke columns
  if( nvars > maxvars_) {
    nvars = maxvars_;
    vector<Column_Info>::iterator pTarget = column_info.begin();
    for( int i=0; i<=nvars; i++) pTarget++;
    column_info.erase( pTarget, column_info.end());
  }
}

//***************************************************************************
// Data_File_Manager::n_ascii_columns() -- Get number of columns that 
// contain ASCII values
int Data_File_Manager::n_ascii_columns()
{
  int result = 0;
  for( unsigned int i=0; i<column_info.size(); i++) {
    if( column_info[i].hasASCII >0) result++;
  }
  return result;
}

//***************************************************************************
// Data_File_Manager::n_vars() -- Get the number of variables (columns of
// data) accounting for the fact that the last column should contain the 
// dummy label '-nothing-'
int Data_File_Manager::n_vars()
{
  return column_info.size() - 1;
}

//***************************************************************************
// Data_File_Manager::n_points() -- Get the number of data points (rows of
// data).
int Data_File_Manager::n_points()
{
  if( column_info.size() <= 1) return 0;
  return (column_info[0].points).rows();
}

//***************************************************************************
// Data_File_Manager::output_filespec() -- Get output filespec.
string Data_File_Manager::output_filespec()
{
  return outFileSpec;
}

//***************************************************************************
// Data_File_Manager::output_filespec( outFileSpecIn) -- Set output_filespec.
void Data_File_Manager::output_filespec( string outFileSpecIn)
{
  outFileSpec = outFileSpecIn;
}

//***************************************************************************
// Data_File_Manager::outputFileType() -- Get output file type.
int Data_File_Manager::outputFileType()
{
  return outputFileType_;
}

//***************************************************************************
// Data_File_Manager::outputFileType( i) -- Set output file type.
void Data_File_Manager::outputFileType( int i)
{
  outputFileType_ = i;
  if( outputFileType_ < 0) outputFileType_ = 0;
  if( outputFileType_ > 1) outputFileType_ = 1;
}
