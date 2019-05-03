#ifndef _____PLWINDOW0_____
#define _____PLWINDOW0_____
#include "WXWidgets.hpp"

struct PlaylistWindow: wxDialog {
struct App& app;
struct wxTextCtrl* tfFilter;
struct wxListBox* lcList;
struct wxButton *btnClose, *btnPlay;

std::string input = "";
long lastInputTime=0;

PlaylistWindow (App& app);
void onItemClick ();
void OnListKeyDown (wxKeyEvent& e);
void OnListKeyChar (wxKeyEvent& e);
void OnContextMenu (wxContextMenuEvent& e);
void OnCloseRequest ();
void OnFilterTextChange (wxCommandEvent& e);
void OnFilterTextEnter (wxCommandEvent& e);
void updateList ();
};

#endif