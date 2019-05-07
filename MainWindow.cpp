#include "cpprintf.hpp"
#include "stringUtils.hpp"
#include "MainWindow.hpp"
#include "App.hpp"
#include "PlaylistWindow.hpp"
#include "LevelsWindow.hpp"
#include "ItemInfoDlg.hpp"
#include "Encoder.hpp"
#include "Caster.hpp"
#include "CastStreamDlg.hpp"
#include "UniversalSpeech.h"
#include "WXWidgets.hpp"
#include <wx/thread.h>
#include <wx/progdlg.h>
#include <wx/aboutdlg.h>
#include <wx/accel.h>
#include <wx/settings.h>
#include <wx/gbsizer.h>
#include <wx/timer.h>
#include <wx/scrolbar.h>
#include <wx/slider.h>
#include "bass.h"
#include "bass_fx.h"
#include "bassmidi.h"
#include<cmath>
using namespace std;

float eqFreqs[] = {
80, 180, 400,  825, 1700, 3500,  7500
}, eqBandwidths[] = {
2, 2, 2, 2, 2, 2, 2
};

extern void encAddAll ();

MainWindow::MainWindow (App& app):
wxFrame(nullptr, wxID_ANY,
U(APP_DISPLAY_NAME),
wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE),
app(app)
{
println("Initializing main window GUI...");
auto panel = new wxPanel(this);
btnPlay = new wxButton(panel, IDM_PLAYPAUSE, U(translate("Play")) );
btnPrev = new wxButton(panel, IDM_PREVTRACK, U(translate("Previous")) );
btnNext = new wxButton(panel, IDM_NEXTTRACK, U(translate("Next")) );
btnOptions = new wxButton(panel, wxID_ANY, U(translate("Options")) );
auto lblPosition = new wxStaticText(panel, wxID_ANY, U(translate("Position")), wxPoint(-2, -2), wxSize(1, 1) );
slPosition = new wxSlider(panel, wxID_ANY, 0, 0, 60, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblVolume = new wxStaticText(panel, wxID_ANY, U(translate("Volume")), wxPoint(-2, -2), wxSize(1, 1) );
slVolume = new wxSlider(panel, wxID_ANY, app.streamVol * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);
auto lblRate = new wxStaticText(panel, wxID_ANY, U(translate("Speed")), wxPoint(-2, -2), wxSize(1, 1) );
slRate = new wxSlider(panel, wxID_ANY, 50, 0, 100,  wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);
auto lblPitch = new wxStaticText(panel, wxID_ANY, U(translate("Pitch")), wxPoint(-2, -2), wxSize(1, 1) );
slPitch = new wxSlider(panel, wxID_ANY, 36, 0, 72,  wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);
slPitch->SetPageSize(12);
for (int i=0; i<7; i++) {
auto lblEqualizer = new wxStaticText(panel, wxID_ANY, U(format("%s %gHz", translate("Equalizer"), eqFreqs[i] )), wxPoint(-2, -2), wxSize(1, 1) );
slEqualizer[i] = new wxSlider(panel, wxID_ANY, 60, 0, 120,  wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL | wxSL_INVERSE);
}

status = CreateStatusBar(4);

auto bagSizer = new wxGridBagSizer();
bagSizer->Add(btnPlay, wxGBPosition(0, 0), wxGBSpan(1, 3), 0);
bagSizer->Add(btnPrev, wxGBPosition(0, 3), wxGBSpan(1, 3), 0);
bagSizer->Add(btnNext, wxGBPosition(0, 6), wxGBSpan(1, 3), 0);
bagSizer->Add(btnOptions, wxGBPosition(0, 9), wxGBSpan(1, 3), 0);
bagSizer->Add(slPosition, wxGBPosition(1, 0), wxGBSpan(1, 12), 0);
bagSizer->Add(slVolume, wxGBPosition(2, 9), wxGBSpan(1, 1), 0);
bagSizer->Add(slRate, wxGBPosition(2, 10), wxGBSpan(1, 1), 0);
bagSizer->Add(slPitch, wxGBPosition(2, 11), wxGBSpan(1, 1), 0);
for (int i=0; i<7; i++) bagSizer->Add(slEqualizer[i], wxGBPosition(2, i+1), wxGBSpan(1, 1), 0);
panel->SetSizer(bagSizer);
auto panelSizer = new wxBoxSizer(wxVERTICAL);
panelSizer->Add(panel, 1, wxEXPAND);

int sizes[] = { -1, -1, 30, 30, 30 };
status->SetFieldsCount(5, sizes);
status->SetStatusText(U("0:00:00 / 0:00:00."), 0);
status->SetStatusText(U("123/456 voices."), 1);
status->SetStatusText(U(format("%d%%.", (int)(app.streamVol*100) )), 2);
status->SetStatusText(U("+0."), 3);
status->SetStatusText(U("100%."), 4);

auto menubar = new wxMenuBar();
auto fileMenu = new wxMenu();
auto mediaMenu = new wxMenu();
auto windowMenu = new wxMenu();
auto openMenu = new wxMenu();
auto appendMenu = new wxMenu();
openMenu->Append(IDM_OPENFILE, U(translate("OpenFile")));
openMenu->Append(IDM_OPENDIR, U(translate("OpenDirectory")));
openMenu->Append(IDM_OPENURL, U(translate("OpenURL")));
appendMenu->Append(IDM_APPENDFILE, U(translate("AppendFile")));
appendMenu->Append(IDM_APPENDDIR, U(translate("AppendDirectory")));
appendMenu->Append(IDM_APPENDURL, U(translate("AppendURL")));
fileMenu->AppendSubMenu(openMenu, U(translate("OpenSubMenu")));
fileMenu->AppendSubMenu(appendMenu, U(translate("AppendSubMenu")));
fileMenu->Append(IDM_SHOWINFO, U(translate("FileProperties")));
fileMenu->Append(IDM_SAVE, U(translate("SaveFileAs")));
fileMenu->Append(IDM_SAVEPLAYLIST, U(translate("SavePlaylistAs")));
fileMenu->Append(wxID_EXIT, U(translate("Exit")));
mediaMenu->Append(IDM_PLAYPAUSE, U(translate("PlayPause")));
mediaMenu->Append(IDM_PREVTRACK, U(translate("PreviousTrack")));
mediaMenu->Append(IDM_NEXTTRACK, U(translate("NextTrack")));
mediaMenu->Append(IDM_CASTSTREAM, U(translate("CastStream")));
mediaMenu->AppendCheckItem(IDM_LOOP, U(translate("PlayLoop")));
windowMenu->AppendCheckItem(IDM_SHOWPLAYLIST, U(translate("Playlist")));
windowMenu->AppendCheckItem(IDM_SHOWLEVELS, U(translate("Levels")));
windowMenu->Append(wxID_ANY, U(translate("MIDIPane")));
menubar->Append(fileMenu, U(translate("File")));
menubar->Append(mediaMenu, U(translate("Media")));
menubar->Append(windowMenu, U(translate("Window")));
SetMenuBar(menubar);

#define R(I,K) RegisterHotKey(I, 0, K);
R(IDM_PLAYPAUSE, 0xA9)
R(IDM_PLAYPAUSE, 0xB2)
R(IDM_PLAYPAUSE, 0xB3)
R(IDM_NEXTTRACK, 0xA7)
R(IDM_NEXTTRACK, 0xB0)
R(IDM_PREVTRACK, 0xA6)
R(IDM_PREVTRACK, 0xB1)
#undef R

refreshTimer = new wxTimer(this, 99);
otherTimer = new wxTimer(this, 98);
refreshTimer->Start(250);
OnTrackChanged();

Bind(wxEVT_TIMER, &MainWindow::OnTrackUpdate, this);
panel->Bind(wxEVT_CHAR_HOOK, &MainWindow::OnCharHook, this);
slPosition->Bind(wxEVT_SCROLL_CHANGED, &MainWindow::OnSeekPosition, this);
slVolume->Bind(wxEVT_SCROLL_CHANGED, &MainWindow::OnVolChange, this);
slRate->Bind(wxEVT_SCROLL_CHANGED, &MainWindow::OnRateChange, this);
slPitch->Bind(wxEVT_SCROLL_CHANGED, &MainWindow::OnPitchChange, this);
for (int i=0; i<7; i++) slEqualizer[i]->Bind(wxEVT_SCROLL_CHANGED, [this,i](auto& e){ OnEqualizerChange(e,i); });
btnPlay->Bind(wxEVT_BUTTON, MainWindow::OnPlayPause, this);
btnNext->Bind(wxEVT_BUTTON, MainWindow::OnNextTrack, this);
btnPrev->Bind(wxEVT_BUTTON, MainWindow::OnPrevTrack, this);
Bind(wxEVT_MENU, &MainWindow::OnOpenFile, this, IDM_OPENFILE);
Bind(wxEVT_MENU, &MainWindow::OnAppendFile, this, IDM_APPENDFILE);
Bind(wxEVT_MENU, &MainWindow::OnOpenDir, this, IDM_OPENDIR);
Bind(wxEVT_MENU, &MainWindow::OnAppendDir, this, IDM_APPENDDIR);
Bind(wxEVT_MENU, &MainWindow::OnOpenURL, this, IDM_OPENURL);
Bind(wxEVT_MENU, &MainWindow::OnAppendURL, this, IDM_APPENDURL);
Bind(wxEVT_MENU, &MainWindow::OnPlayPause, this, IDM_PLAYPAUSE);
Bind(wxEVT_MENU, &MainWindow::OnNextTrack, this, IDM_NEXTTRACK);
Bind(wxEVT_MENU, &MainWindow::OnPrevTrack, this, IDM_PREVTRACK);
Bind(wxEVT_MENU, &MainWindow::OnLoopChange, this, IDM_LOOP);
Bind(wxEVT_MENU, &MainWindow::OnSaveDlg, this, IDM_SAVE);
Bind(wxEVT_MENU, &MainWindow::OnSavePlaylistDlg, this, IDM_SAVEPLAYLIST);
Bind(wxEVT_MENU, &MainWindow::OnShowPlaylist, this, IDM_SHOWPLAYLIST);
Bind(wxEVT_MENU, &MainWindow::OnShowLevels, this, IDM_SHOWLEVELS);
Bind(wxEVT_MENU, &MainWindow::OnShowItemInfo, this, IDM_SHOWINFO);
Bind(wxEVT_MENU, &MainWindow::OnCastStreamDlg, this, IDM_CASTSTREAM);
Bind(wxEVT_HOTKEY, &MainWindow::OnPlayPauseHK, this, IDM_PLAYPAUSE);
Bind(wxEVT_HOTKEY, &MainWindow::OnNextTrackHK, this, IDM_NEXTTRACK);
Bind(wxEVT_HOTKEY, &MainWindow::OnPrevTrackHK, this, IDM_PREVTRACK);
//Bind(wxEVT_MENU, [&](wxCommandEvent& e){ app.OnAction(e.GetId()); });
Bind(wxEVT_CLOSE_WINDOW, &MainWindow::OnClose, this);
//Bind(wxEVT_ACTIVATE_APP, [](wxActivateEvent& e){ println("Activate app %s", e.GetActive()); e.Skip(); });
//Bind(wxEVT_ACTIVATE, [&](wxActivateEvent& e){ active=e.GetActive();  e.Skip(); });
//Bind(wxEVT_ICONIZE, [&](wxIconizeEvent& e){ active=!e.IsIconized();  e.Skip(); });
Bind(wxEVT_THREAD, &MainWindow::OnProgress, this);
btnPlay->SetFocus();
SetSizerAndFit(panelSizer);
SetSize(wxSize(500, 300));

auto sz = GetSize();
println("Size = %d x %d", sz.x, sz.y);

vector<wxAcceleratorEntry> entries = {
/*{ wxACCEL_NORMAL, WXK_PAGEUP, ACTION_HISTORY_PREV  },
{ wxACCEL_NORMAL, WXK_PAGEDOWN, ACTION_HISTORY_NEXT },
{ wxACCEL_CTRL, WXK_PAGEUP, ACTION_HISTORY_FIRST },
{ wxACCEL_CTRL, WXK_PAGEDOWN, ACTION_HISTORY_LAST },
{ wxACCEL_SHIFT, WXK_PAGEUP, ACTION_HISTORY_FIRST },
{ wxACCEL_SHIFT, WXK_PAGEDOWN, ACTION_HISTORY_LAST },
{ wxACCEL_ALT, WXK_LEFT, ACTION_VIEW_PREV },
{ wxACCEL_ALT, WXK_RIGHT, ACTION_VIEW_NEXT },
{ wxACCEL_ALT, '0', ACTION_VIEW_GOTO+9 },
{ wxACCEL_ALT, '1', ACTION_VIEW_GOTO+0 },
{ wxACCEL_ALT, '2', ACTION_VIEW_GOTO+1 },
{ wxACCEL_ALT, '3', ACTION_VIEW_GOTO+2 },
{ wxACCEL_ALT, '4', ACTION_VIEW_GOTO+3 },
{ wxACCEL_ALT, '5', ACTION_VIEW_GOTO+4 },
{ wxACCEL_ALT, '6', ACTION_VIEW_GOTO+5 },
{ wxACCEL_ALT, '7', ACTION_VIEW_GOTO+6 },
{ wxACCEL_ALT, '8', ACTION_VIEW_GOTO+7 },
{ wxACCEL_ALT, '9', ACTION_VIEW_GOTO+8 },
{ wxACCEL_CTRL, WXK_SPACE, ACTION_COPY_CUR_HISTORY },
{ wxACCEL_CTRL, WXK_RETURN, ACTION_ACTIVATE_CURHISTORY__LINK },
{ wxACCEL_NORMAL, WXK_F6, ACTION_VOL_SELECT },
{ wxACCEL_NORMAL, WXK_F8, ACTION_VOL_PLUS },
{ wxACCEL_NORMAL, WXK_F7, ACTION_VOL_MINUS },
{ wxACCEL_CTRL | wxACCEL_SHIFT, WXK_DELETE, ACTION_DEBUG }*/
};
wxAcceleratorTable table(entries.size(), &entries[0]);
SetAcceleratorTable(table);
SetFocus();
btnPlay->SetFocus();
println("Initialized main window GUI");
}

int MainWindow::popupMenu (const vector<string>& items, int selection) {
wxMenu menu;
for (int i=0, n=items.size(); i<n; i++) menu.Append(i+1, U(items[i]), wxEmptyString, selection<0? wxITEM_NORMAL : wxITEM_RADIO);
if (selection>=0) menu.Check(selection+1, true);
int result = GetPopupMenuSelectionFromUser(menu);
return result==wxID_NONE? -1 : result -1;
}

void MainWindow::openProgress (const std::string& title, const std::string& message, int maxValue) {
RunEDT([=](){
progressCancelled=false;
progressDialog = new wxProgressDialog(U(title), U(message), maxValue, this, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME);
});
}

void MainWindow::closeProgress () {
progressDialog->Destroy();
progressDialog=nullptr;
wxWakeUpIdle();
}

void MainWindow::updateProgress (int value) {
wxThreadEvent e(wxEVT_THREAD, 9999);
e.SetInt(value);
wxQueueEvent(this, e.Clone());
}

void MainWindow::setProgressMax (int value) {
wxThreadEvent e(wxEVT_THREAD, 9997);
e.SetInt(value);
wxQueueEvent(this, e.Clone());
}

void MainWindow::setProgressText (const std::string& msg) {
wxThreadEvent e(wxEVT_THREAD, 9998);
e.SetString(U(msg));
wxQueueEvent(this, e.Clone());
}

void MainWindow::setProgressTitle (const std::string& msg) {
wxThreadEvent e(wxEVT_THREAD, 9996);
e.SetString(U(msg));
wxQueueEvent(this, e.Clone());
}

void MainWindow::OnProgress (wxThreadEvent& e) {
switch(e.GetId()) {
case 9999: {
int value = e.GetInt();
if (progressDialog) progressCancelled = !progressDialog->Update(value);
}break;
case 9998: {
wxString value = e.GetString();
if (progressDialog) progressCancelled = !progressDialog->Update(progressDialog->GetValue(), value);
}break;
case 9997: {
int value = e.GetInt();
if (progressDialog) progressDialog->SetRange(value);
}break;
case 9996: {
wxString value = e.GetString();
if (progressDialog) progressDialog->SetTitle(value);
}break;
}}

bool MainWindow::isProgressCancelled () { return progressCancelled; }

void MainWindow::setTimeout (int ms, const function<void()>& func) {
timerFunc = func;
otherTimer->StartOnce(ms);
}

void MainWindow::showAboutBox (wxWindow* parent) {
string name = APP_DISPLAY_NAME;
wxAboutDialogInfo info;
info.SetCopyright("Copyright (C) 2019, QuentinC");
info.SetName(name);
info.SetVersion(format("%1.%2.%3", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD));
info.SetWebSite("https://quentinc.net/");
wxAboutBox(info, parent);
}

void MainWindow::OnOpenFile (wxCommandEvent& e) {
OnOpenFileDlg(false);
}

void MainWindow::OnAppendFile (wxCommandEvent& e) {
OnOpenFileDlg(true);
}

static void openFileUpdatePreview (wxFileDialog* fd) {
static string curFile = string();
string newFile = UFN(fd->GetCurrentlySelectedFilename());
if (newFile.size() && newFile!=curFile) {
curFile=newFile;
App& app = wxGetApp();
if (app.curPreviewStream) BASS_ChannelStop(app.curPreviewStream);
app.win->setTimeout(1000, [=,&app]()mutable{
BASS_SetDevice(app.previewDevice);
app.curPreviewStream = app.loadFileOrURL(curFile, false, false);
if (app.curPreviewStream) {
BASS_ChannelSetAttribute(app.curPreviewStream, BASS_ATTRIB_VOL, app.previewVol);
BASS_ChannelPlay(app.curPreviewStream, false);
}});
}}

static wxWindow* createOpenFilePanel (wxWindow* parent) {
wxWindow* panel = new wxPanel(parent);
panel->Bind(wxEVT_UPDATE_UI, [=](auto& e){ 
        wxFileDialog* fd = wxStaticCast(parent, wxFileDialog);
openFileUpdatePreview(fd);
});
return panel;
}

static wxWindow* createSaveFilePanel (wxWindow* parent) {
App& app = wxGetApp();
wxWindow* panel = new wxPanel(parent);
auto sizer = new wxBoxSizer(wxHORIZONTAL);
auto btnFormat = new wxButton(panel, wxID_ANY, U(translate("FormatOptionsBtn")));
sizer->Add(btnFormat);
panel->Bind(wxEVT_UPDATE_UI, [=](auto& e){ 
        wxFileDialog* fd = wxStaticCast(parent, wxFileDialog);
int filterIndex = fd->GetCurrentlySelectedFilterIndex();
bool enable = true;
if (filterIndex>=0 && filterIndex<Encoder::encoders.size()) {
auto& encoder = *Encoder::encoders[filterIndex];
enable = encoder.hasFormatDialog();
}
btnFormat->Enable(enable);
});
btnFormat->Bind(wxEVT_BUTTON, [=](auto& e){
        wxFileDialog* fd = wxStaticCast(parent, wxFileDialog);
int filterIndex = fd->GetCurrentlySelectedFilterIndex();
if (filterIndex<0 || filterIndex>=Encoder::encoders.size()) return;
auto& encoder = *Encoder::encoders[filterIndex];
if (encoder.hasFormatDialog()) encoder.showFormatDialog(fd->GetParent());
else Beep(1000, 150);
btnFormat->SetFocus();
});
return panel;
}

void MainWindow::OnOpenFileDlg (bool append) {
wxFileDialog fd(this, U(translate("OpenFileDlg")), wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
fd.SetExtraControlCreator(createOpenFilePanel);
if (wxID_OK==fd.ShowModal()) {
wxArrayString files;
fd.GetPaths(files);
if (!append) app.playlist.clear();
for (auto& file: files) app.openFileOrURL(UFN(file));
}
if (otherTimer->IsRunning()) otherTimer->Stop();
if (app.curPreviewStream) BASS_ChannelStop(app.curPreviewStream);
app.curPreviewStream=0;
}

void MainWindow::OnOpenDir (wxCommandEvent& e) {
OnOpenDirDlg(false);
}

void MainWindow::OnAppendDir (wxCommandEvent& e) {
OnOpenDirDlg(true);
}

void MainWindow::OnOpenDirDlg (bool append) {
wxDirDialog dd(this, U(translate("OpenDirDlg")), wxEmptyString, wxDD_DIR_MUST_EXIST | wxRESIZE_BORDER);
if (dd.ShowModal()==wxID_OK) {
if (!append) app.playlist.clear();
app.openFileOrURL(UFN(dd.GetPath()));
}}

void MainWindow::OnOpenURL (wxCommandEvent& e) {
OnOpenURLDlg(false);
}

void MainWindow::OnAppendURL (wxCommandEvent& e) {
OnOpenURLDlg(true);
}

void MainWindow::OnOpenURLDlg (bool append) {
wxTextEntryDialog ted(this, U(translate("OpenURLDlg")), U(translate("OpenURLDlgPrompt")), wxEmptyString);
if (wxID_OK==ted.ShowModal()) {
if (!append) app.playlist.clear();
app.openFileOrURL(U(ted.GetValue()));
}}

void MainWindow::OnSavePlaylistDlg (wxCommandEvent& e) {
vector<shared_ptr<PlaylistFormat>> usableFormats;
vector<string> filters;
int filterIndex = 0;
for (auto& format: Playlist::formats) {
if (!format->checkWrite(format->extension + "." + format->extension)) continue;
if (app.playlist.format.get() == format.get()) filterIndex = usableFormats.size();
usableFormats.push_back(format);
filters.push_back(::format("%s (*.%s)", format->name, format->extension));
filters.push_back("*." + format->extension);
}
wxFileDialog fd(this, U(translate("SavePlaylistDlg")), wxEmptyString, UFN(app.playlist.file), U(join(filters, "|")), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
fd.SetFilterIndex(filterIndex);
if (wxID_OK!=fd.ShowModal()) return;
string file = UFN(fd.GetPath());
filterIndex = fd.GetFilterIndex();
auto& format = usableFormats[filterIndex];
if (!format->checkWrite(file) || !format->save(app.playlist, file)) {
//todo: error message
return;
}
app.playlist.file = file;
app.playlist.format = format;
}

void MainWindow::OnSaveDlg (wxCommandEvent& e) {
vector<string> filters;
if (!Encoder::encoders.size()) encAddAll();
for (auto& encoder: Encoder::encoders) {
filters.push_back(::format("%s (*.%s)", encoder->name, encoder->extension));
filters.push_back("*." + encoder->extension);
}
wxFileDialog fd(this, U(translate("SaveFileDlg")), wxEmptyString, wxEmptyString, U(join(filters, "|")), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
fd.SetExtraControlCreator(createSaveFilePanel);
if (wxID_OK!=fd.ShowModal()) return;
string file = UFN(fd.GetPath());
int filterIndex = fd.GetFilterIndex();
auto& encoder = *Encoder::encoders[filterIndex];
app.saveEncode(app.playlist.current(), file, encoder);
}

void MainWindow::OnShowItemInfo (wxCommandEvent& e) {
ItemInfoDlg dlg(app, this, app.playlist.current(), app.curStream);
dlg.ShowModal();
}

void MainWindow::OnShowLevels (wxCommandEvent& e) {
if (!levelsWindow) levelsWindow = new LevelsWindow(app);
if (levelsWindow->IsVisible()) levelsWindow->Hide();
else {
levelsWindow->Show();
levelsWindow->cbStreamDevice->SetFocus();
}
GetMenuBar()->Check(IDM_SHOWLEVELS, levelsWindow->IsVisible());
}

void MainWindow::OnShowPlaylist (wxCommandEvent& e) {
if (!playlistWindow) playlistWindow = new PlaylistWindow(app);
if (playlistWindow->IsVisible()) playlistWindow->Hide();
else {
playlistWindow->lcList->SetSelection(wxNOT_FOUND);
playlistWindow->lcList->SetFocus();
playlistWindow->Show();
}
GetMenuBar()->Check(IDM_SHOWPLAYLIST, playlistWindow->IsVisible());
}

void MainWindow::OnPlayPause () { 
auto state = BASS_ChannelIsActive(app.curStream);
if (state==BASS_ACTIVE_PLAYING || state==BASS_ACTIVE_STALLED) {
BASS_ChannelPause(app.curStream);
btnPlay->SetLabel(U(translate("Play")));
} else {
BASS_ChannelPlay(app.curStream, false);
btnPlay->SetLabel(U(translate("Pause")));
}}

void MainWindow::OnNextTrack () {
app.playNext();
}

void MainWindow::OnPrevTrack () {
app.playNext(-1);
}

void MainWindow::OnVolChange (wxScrollEvent& e) {
int vol = slVolume->GetValue();
changeVol(vol/100.0f, false, true);
}

void MainWindow::changeVol (float vol, bool update, bool update2) {
app.streamVol = vol;
status->SetStatusText(U(format("%g%%.", 100.0f * vol)), 2);
BASS_ChannelSetAttribute(app.curStream, BASS_ATTRIB_VOL, vol);
if (update) slVolume->SetValue(vol * 100);
if (update2 && levelsWindow) levelsWindow->slStreamVol->SetValue(vol * 100);
}

void MainWindow::OnPitchChange (wxScrollEvent& e) {
int pitch = slPitch->GetValue() -36;
changePitch(pitch, false);
}

void MainWindow::changePitch (int pitch, bool update) {
status->SetStatusText(U(format("%+d.", pitch)), 3);
BASS_ChannelSetAttribute(app.curStream, BASS_ATTRIB_TEMPO_PITCH, pitch);
}

void MainWindow::OnRateChange (wxScrollEvent& e) {
double val = (slRate->GetValue() -50) /25.0;
double ratio = pow(2, val);
changeRate(ratio, false);
}

void MainWindow::changeRate (double ratio, bool update) {
status->SetStatusText(U(format("%g%%.", round(100 * ratio))), 4);
BASS_ChannelSetAttribute(app.curStream, BASS_ATTRIB_TEMPO, (ratio * 100) -100);
}

void MainWindow::OnEqualizerChange (wxScrollEvent& e, int index) {
float gain = (slEqualizer[index]->GetValue() -60) /4.0;
changeEqualizer(index, gain, false);
}

void MainWindow::changeEqualizer (int index, float gain, bool update) {
BASS_BFX_PEAKEQ p = { index, eqBandwidths[index], 0, eqFreqs[index], gain, -1 }; 
BASS_FXSetParameters(app.curStreamEqFX, &p);
}

void MainWindow::OnSeekPosition (wxScrollEvent& e) {
int pos = slPosition->GetValue();
seekPosition(pos, false);
}

void MainWindow::seekPosition (double pos, bool update) {
BASS_ChannelSetPosition(app.curStream, BASS_ChannelSeconds2Bytes(app.curStream, pos), BASS_POS_BYTE);
}

void MainWindow::OnLoopChange () {
app.loop = !app.loop;
DWORD stream = app.curStream;
DWORD src = BASS_FX_TempoGetSource(stream);
DWORD flags = app.loop? BASS_SAMPLE_LOOP : 0;
BASS_ChannelFlags(stream, flags, BASS_SAMPLE_LOOP);
GetMenuBar() ->Check(IDM_LOOP, app.loop);
speechSay(U(translate(app.loop? "LoopOn" : "LoopOff")).wc_str(), true);
}

void MainWindow::OnCastStreamDlg (wxCommandEvent& e) {
CastStreamDlg dlg(app, this);
if (wxID_OK==dlg.ShowModal()) {
app.startCaster(
*Caster::casters[ dlg.cbServerType->GetSelection() ],
*Encoder::encoders[ dlg.cbFormat->GetSelection() ],
U(dlg.tfServer->GetValue()),
U(dlg.tfPort->GetValue()),
U(dlg.tfUser->GetValue()),
U(dlg.tfPass->GetValue()),
U(dlg.tfMount->GetValue())
);
}}

void MainWindow::OnTrackChanged () {
if (!app.curStream || app.playlist.curIndex<0) return;
DWORD stream = app.curStream;
auto& item = app.playlist.current();
auto byteLen = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
int secLen = item.length = BASS_ChannelBytes2Seconds(stream, byteLen);
slPosition->SetRange(0, secLen);
slPosition->SetLineSize(5);
slPosition->SetPageSize(30);
btnPlay->SetLabel(U(translate("Pause")));
if (playlistWindow) {
playlistWindow->updateList();
}
string sWinTitle = format("%d. %s - %s", app.playlist.curIndex+1, item.title, APP_DISPLAY_NAME);
SetTitle(U(sWinTitle));
}

void MainWindow::OnTrackUpdate (wxTimerEvent& e) {
if (e.GetId()==98) { timerFunc(); return; }
DWORD stream = app.curStream;
auto bytePos = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
auto byteLen = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
int secPos = BASS_ChannelBytes2Seconds(stream, bytePos);
int secLen = BASS_ChannelBytes2Seconds(stream, byteLen);
auto lenStr = formatTime(secPos) + " / " + formatTime(secLen) + ".";
status->SetStatusText(U(lenStr), 0);
slPosition->SetValue(secPos);

if (app.curStreamType==BASS_CTYPE_STREAM_MIDI || (app.curStreamType&BASS_CTYPE_MUSIC_MOD)) {
float voices = -1;
BASS_ChannelGetAttribute(BASS_FX_TempoGetSource(app.curStream), app.curStreamType==BASS_CTYPE_STREAM_MIDI? BASS_ATTRIB_MIDI_VOICES_ACTIVE : BASS_ATTRIB_MUSIC_ACTIVE, &voices);
app.curStreamVoicesMax = std::max<int>(app.curStreamVoicesMax, voices);
status->SetStatusText(U(format(translate("statvoices"), static_cast<int>(voices), app.curStreamVoicesMax)), 1);
}
else status->SetStatusText("", 1);
}

static inline void slide (wxSlider* sl, int delta) {
sl->SetValue(sl->GetValue() + delta);
}

void MainWindow::OnPrevNextTrackHK (wxKeyEvent& e, bool next) {
static long lastTime = 0;
static bool intercepted = false;
long time = e.GetTimestamp();
intercepted = (time-lastTime)<1000;
lastTime = time;
if (intercepted) seekPosition(  (next? 2 : -2) + BASS_ChannelBytes2Seconds(app.curStream, BASS_ChannelGetPosition(app.curStream, BASS_POS_BYTE)), true);
else setTimeout(510, [=]()mutable{
if (!intercepted) {
if (next) OnNextTrack();
else OnPrevTrack();
}});
}

void MainWindow::OnCharHook (wxKeyEvent& e) {
auto focus = wxWindow::FindFocus();
auto key = e.GetKeyCode(), mod = e.GetModifiers();
void* vnullptr = nullptr;
auto& nullscrev = *reinterpret_cast<wxScrollEvent*>(vnullptr);
if (mod==0) switch(key) {
case 'C': OnPlayPause(); break;
case 'Y': OnPrevTrack(); break;
case 'B': OnNextTrack(); break;
case 'Q': slide(slRate, 1); OnRateChange(nullscrev); break;
case 'A': slide(slRate, -1); OnRateChange(nullscrev); break;
case 'W': slide(slPitch, 1); OnPitchChange(nullscrev); break;
case 'S': slide(slPitch, -1); OnPitchChange(nullscrev); break;
case 'X': seekPosition(0, true); break;
case 'V': BASS_ChannelPause(app.curStream); seekPosition(0, true); break;
case 'P': OnLoopChange(); break;

#define E(K,J,N) case J: slide(slEqualizer[N], 1); OnEqualizerChange(nullscrev, N); break; case K: slide(slEqualizer[N], -1); OnEqualizerChange(nullscrev, N); break;
E('D', 'E', 0)
E('F', 'R', 1)
E('G', 'T', 2)
E('H', 'Z', 3)
E('J', 'U', 4)
E('K', 'I', 5)
E('L', 'O', 6)
#undef E
}
if (focus==btnPlay || focus==btnNext ||  focus==btnPrev) switch(key){
case WXK_UP: 
if (mod==0) slide(slVolume, 1); OnVolChange(nullscrev); 
return;
case WXK_DOWN: 
if (mod==0) slide(slVolume, -1); OnVolChange(nullscrev); 
return;
case WXK_HOME:
if (mod==0) {
auto pos = BASS_ChannelBytes2Seconds(app.curStream, BASS_ChannelGetPosition(app.curStream, BASS_POS_BYTE));
if (pos<1) OnPrevTrack();
else seekPosition(0, true);
}
break;
case WXK_END:
if (mod==0) OnNextTrack();
break;

#define SEEK(N) seekPosition( N + BASS_ChannelBytes2Seconds(app.curStream, BASS_ChannelGetPosition(app.curStream, BASS_POS_BYTE)), true)
case WXK_LEFT:
if (mod==0) SEEK(-5);
else if (mod==wxMOD_CONTROL) SEEK(-30);
return;
case WXK_RIGHT:
if (mod==0) SEEK(5);
else if (mod==wxMOD_CONTROL) SEEK(30);
return;
case WXK_PAGEUP:
if (mod==0) SEEK(-30);
else if (mod==wxMOD_CONTROL) SEEK(-300);
break;
case WXK_PAGEDOWN:
if (mod==0) SEEK(30);
else if (mod==wxMOD_CONTROL) SEEK(300);
break;
#undef SEEK
}
if (key>='A' && key<='Z' && (mod==0 || mod==wxMOD_SHIFT)) return;
e.Skip();
}

void MainWindow::OnClose (wxCloseEvent& e) {
//if (e.CanVeto() && app.config.get("general.confirmOnQuit", true) && wxNO==wxMessageBox(U(translate("confirmquit")), GetTitle(), wxICON_EXCLAMATION | wxYES_NO)) e.Veto();
//else 
app.OnQuit();
e.Skip();
}

