// Digigurdy-Baz

// AUTHOR: Basil Lalli
// DESCRIPTION: Digigurdy-Baz is a fork of the Digigurdy code by John Dingley.  See his page:
//   https://digigurdy.com/
//   https://hackaday.io/project/165251-the-digi-gurdy-and-diginerdygurdy
// REPOSITORY: https://github.com/bazmonk/digigurdy-baz

#include <Adafruit_GFX.h>
// https://www.pjrc.com/teensy/td_libs_Bounce.html
#include <Bounce.h>
#include <EEPROM.h>
#include <ADC.h>

// These are found in the digigurdy-baz repository
#include "notes.h"           // Note and note string variables
#include "bitmaps.h"         // Pretty pictures
#include "eeprom_values.h"   // Save-slot memory addresses
#include "default_tunings.h" // Preset tunings.
#include "config.h"          // Configuration variables

#include "common.h"

// These are all about the display
#include "display.h"         // Intializes our display object
#include "startup_screens.h" // Startup-related screens.
#include "play_screens.h"    // The screens mid-play (notes and staffs)
#include "tuning_screens.h"  // The tuning and sub-menu screens.

// Right now not using the std namespace is just impacting strings.  That's ok...
using namespace MIDI_NAMESPACE;

// ##################
// GLOBAL SETUP STUFF
// ##################

// Declare MidiInterface object using that serial interface
MidiInterface<SerialMIDI<HardwareSerial>> *myMIDI;

ADC* adc;

// Declare the "keybox" and buttons.
HurdyGurdy *mygurdy;
ToggleButton *bigbutton;
GurdyCrank *mycrank;

// As musical keys, these are referred to in the mygurdy object above.
// This declaration of them is specifically for their use as navigational
// buttons in the menu screens.  ok = O, back = X.
KeyboxButton *myAButton;
KeyboxButton *myXButton;
KeyboxButton *my1Button;
KeyboxButton *my2Button;
KeyboxButton *my3Button;
KeyboxButton *my4Button;
KeyboxButton *my5Button;
KeyboxButton *my6Button;

// For legacy button-combo support:
KeyboxButton *myAltTposeButton;
KeyboxButton *myAltTposeUp;
KeyboxButton *myAltTposeDown;
KeyboxButton *myBButton;

// Note that there aren't special classes for melody, drone, even the keyclick.
// They are differentiated in the main loop():
// * A melody string is one that changes with the keybox offset.
// * A drone/trompette is one that doesn't change.
// * The keyclick "string" is just a drone that comes on and off at particular times.
// * The buzz "string" is also just a drone that comes on/off at other particular times.
GurdyString *mystring;
GurdyString *mylowstring;
GurdyString *mykeyclick;
GurdyString *mytromp;
GurdyString *mydrone;
GurdyString *mybuzz;

// These are the dedicated transpose/capo buttons
GurdyButton *tpose_up;
GurdyButton *tpose_down;
GurdyButton *capo;

// These are the "extra" buttons, new on the rev3.0 gurdies
GurdyButton *ex1Button;
GurdyButton *ex2Button;
GurdyButton *ex3Button;

// This defines the +/- one octave transpose range.
const int max_tpose = 12;
int tpose_offset;

// This defines the 0, +2, +4 capo range.
const int max_capo = 4;
int capo_offset;

// The offset is given when we update the buttons each cycle.
// Buttons should only be updated once per cycle.  So we need this in the main loop()
// to refer to it several times.
int myoffset;

// this tracks the drone/trompette mute status.  Starts all-on.
// 0 = all on
// 1 = all off
// 2 = drone on, tromp off
// 3 = drone off, tromp on
int drone_mode = 0;

// similarly to drone_mode...
// 0 = all on
// 1 = high on, low off
// 2 = high off, low on
int mel_mode = 0;

int play_screen_type = 0;
uint8_t scene_signal_type = 0;

// Teensy and Arduino units start by running setup() once after powering up.
// Here we establish how the "gurdy" is setup, what strings to use, and we also
// start the MIDI communication.
void setup() {

  start_display();
  startup_screen_sqeuence();

  // Un-comment to print yourself debugging messages to the Teensyduino
  // serial console.
  Serial.begin(115200);
  delay(500);
  Serial.println("Hello.");

  adc = new ADC();

  // Create the serial interface object
  SerialMIDI<HardwareSerial> mySerialMIDI(Serial1);
  myMIDI = new MidiInterface<SerialMIDI<HardwareSerial>>((SerialMIDI<HardwareSerial>&)mySerialMIDI);
  myMIDI->begin();

  mycrank = new GurdyCrank(15, A2, adc, LED_PIN);

  // The keybox arrangement is decided by pin_array, which is up in the CONFIG SECTION
  // of this file.  Make adjustments there.
  mygurdy = new HurdyGurdy(pin_array, num_keys);
  bigbutton = new ToggleButton(39);

  // These indices are defined in config.h
  myXButton = mygurdy->keybox[X_INDEX];
  myAButton = mygurdy->keybox[A_INDEX];
  myBButton = mygurdy->keybox[B_INDEX];

  my1Button = mygurdy->keybox[BUTTON_1_INDEX];
  my2Button = mygurdy->keybox[BUTTON_2_INDEX];
  my3Button = mygurdy->keybox[BUTTON_3_INDEX];
  my4Button = mygurdy->keybox[BUTTON_4_INDEX];
  my5Button = mygurdy->keybox[BUTTON_5_INDEX];
  my6Button = mygurdy->keybox[BUTTON_6_INDEX];

  myAltTposeUp = mygurdy->keybox[TPOSE_UP_INDEX];
  myAltTposeDown = mygurdy->keybox[TPOSE_DN_INDEX];


  // Which channel is which doesn't really matter, but I'm sticking with
  // John's channels so his videos on setting it up with a tablet/phone still work.
  //
  // This starting tuning here is replaced immediately upon startup with either a
  // preset, a saved tuning, or a created one.  The menu shouldn't let you actually
  // end up with this, but I have to initialize them with something, so might as well
  // be a working tuning.
  mystring = new GurdyString(1, Note(g4), myMIDI);
  mylowstring = new GurdyString(2, Note(g3), myMIDI);
  mytromp = new GurdyString(3, Note(c3), myMIDI);
  mydrone = new GurdyString(4, Note(c2), myMIDI);
  mybuzz = new GurdyString(5,Note(c3), myMIDI);
  mykeyclick = new GurdyString(6, Note(b5), myMIDI);


  tpose_up = new GurdyButton(22);   // A.k.a. the button formerly known as octave-up
  tpose_down = new GurdyButton(21); // A.k.a. the button formerly known as octave-down

  capo = new GurdyButton(23); // The capo button

  ex1Button = new GurdyButton(41);
  ex2Button = new GurdyButton(17);
  ex3Button = new GurdyButton(14);

  tpose_offset = 0;
  capo_offset = 0;

  scene_signal_type = EEPROM.read(EEPROM_SCENE_SIGNALLING);
};

void signal_scene_change(int scene_idx) {
  if (scene_signal_type == 1) {
    // Signal as a Program Control message on Channel 1
    mystring->setProgram(scene_idx);
  } else {
    // 0 = Do nothing
    // While this should be 0, if there is bad data we'll just ignore it and do nothing.
  }
};

// ##############
// MENU FUNCTIONS
// ##############
//
// Functions here drive the main behavior of the digigurdy.
//
// First-time hackers of this code: the loop() at the end of this is the main()
// of ardruino/teensy programs.  It runs in an endless loop.  The welcome_screen()
// is the first screen that comes after startup if you're trying to track down the flow.
// The pause_screen() is what comes up when X+A is pressed.  The rest of the functions/screens
// get called from them.  Hope that helps you find the part you care about!

// load_preset_tunings accepts an int between 1-4 and sets the appropriate preset.
// See the default_tunings.h file to modify what the presets actually are.
void load_preset_tunings(int preset) {
  const int *tunings;
  if (preset == 1) { tunings = PRESET1; };
  if (preset == 2) { tunings = PRESET2; };
  if (preset == 3) { tunings = PRESET3; };
  if (preset == 4) { tunings = PRESET4; };

  mystring->setOpenNote(tunings[0]);
  mylowstring->setOpenNote(tunings[1]);
  mydrone->setOpenNote(tunings[2]);
  mytromp->setOpenNote(tunings[3]);
  mybuzz->setOpenNote(tunings[4]);
  tpose_offset = tunings[5];
  capo_offset = tunings[6];
}

// load_saved_tunings requires one argument: the "save slot" which
// should be one of the EEPROM_SLOT[1-4] values in eeprom_values.h.
void load_saved_tunings(int slot) {
  byte value;

  // Notes
  value = EEPROM.read(slot + EEPROM_HI_MEL);
  mystring->setOpenNote(value);
  value = EEPROM.read(slot + EEPROM_LO_MEL);
  mylowstring->setOpenNote(value);
  value = EEPROM.read(slot + EEPROM_DRONE);
  mydrone->setOpenNote(value);
  value = EEPROM.read(slot + EEPROM_TROMP);
  mytromp->setOpenNote(value);
  value = EEPROM.read(slot + EEPROM_BUZZ);
  mybuzz->setOpenNote(value);
  value = EEPROM.read(slot + EEPROM_TPOSE);
  tpose_offset = value - 12;
  value = EEPROM.read(slot + EEPROM_CAPO);
  capo_offset = value;

  // Volumes
  value = EEPROM.read(slot + EEPROM_HI_MEL_VOL);
  mystring->setVolume(value);
  value = EEPROM.read(slot + EEPROM_LOW_MEL_VOL);
  mylowstring->setVolume(value);
  value = EEPROM.read(slot + EEPROM_DRONE_VOL);
  mydrone->setVolume(value);
  value = EEPROM.read(slot + EEPROM_TROMP_VOL);
  mytromp->setVolume(value);
  value = EEPROM.read(slot + EEPROM_BUZZ_VOL);
  mybuzz->setVolume(value);
  value = EEPROM.read(slot + EEPROM_KEYCLICK_VOL);
  mykeyclick->setVolume(value);

};

// save_tunings accepts one argument, which should be one of the
// EEPROM_SLOT[1-4] values defined in eeprom_values.h.
void save_tunings(int slot) {

  EEPROM.write(slot + EEPROM_HI_MEL, mystring->getOpenNote());
  EEPROM.write(slot + EEPROM_LO_MEL, mylowstring->getOpenNote());
  EEPROM.write(slot + EEPROM_DRONE, mydrone->getOpenNote());
  EEPROM.write(slot + EEPROM_TROMP, mytromp->getOpenNote());
  EEPROM.write(slot + EEPROM_BUZZ, mybuzz->getOpenNote());
  EEPROM.write(slot + EEPROM_TPOSE, tpose_offset + 12);
  EEPROM.write(slot + EEPROM_CAPO, capo_offset);
  EEPROM.write(slot + EEPROM_HI_MEL_VOL, mystring->getVolume());
  EEPROM.write(slot + EEPROM_LOW_MEL_VOL, mylowstring->getVolume());
  EEPROM.write(slot + EEPROM_DRONE_VOL, mydrone->getVolume());
  EEPROM.write(slot + EEPROM_TROMP_VOL, mytromp->getVolume());
  EEPROM.write(slot + EEPROM_BUZZ_VOL, mybuzz->getVolume());
  EEPROM.write(slot + EEPROM_KEYCLICK_VOL, mykeyclick->getVolume());

};

// This clears the EEPROM and overwrites it all with zeroes
void clear_eeprom() {
  // Not much to say here... write 0 everywhere:
  for (int i = 0 ; i < EEPROM.length() ; i++ )
    EEPROM.write(i, 0);
};

// This screen is for viewing a save slot's settings.
// It returns true if the user wants to use it, false
// otherwise.
// Accepts integer slot: the save slot number.
bool view_slot_screen(int slot_num) {
  int slot;
  if (slot_num == 1) { slot = EEPROM_SLOT1; };
  if (slot_num == 2) { slot = EEPROM_SLOT2; };
  if (slot_num == 3) { slot = EEPROM_SLOT3; };
  if (slot_num == 4) { slot = EEPROM_SLOT4; };

  String disp_str = " -Saved Slot Tuning- \n"
                    " Hi Melody: " + LongNoteNum[EEPROM.read(slot + EEPROM_HI_MEL)] + "  \n"
                    " Lo Melody: " + LongNoteNum[EEPROM.read(slot + EEPROM_LO_MEL)] + "  \n"
                    " Drone:     " + LongNoteNum[EEPROM.read(slot + EEPROM_DRONE)] + "  \n"
                    " Trompette: " + LongNoteNum[EEPROM.read(slot + EEPROM_TROMP)] + "  \n"
                    " Tpose: ";

  if (EEPROM.read(slot + EEPROM_TPOSE) > 12) { disp_str += "+"; };
  disp_str = disp_str + ((EEPROM.read(slot + EEPROM_TPOSE))-12) + "  Capo: ";

  if (EEPROM.read(slot + EEPROM_CAPO) > 0) { disp_str += "+"; };
  disp_str = disp_str + (EEPROM.read(slot + EEPROM_CAPO)) + "\n"
             " A or 1) Accept \n"
             " X or 2) Go Back  \n";

  print_screen(disp_str);

  bool done = false;
  while (!done) {

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    myAButton->update();
    myXButton->update();

    if (my1Button->wasPressed() || myAButton->wasPressed()) {
      load_saved_tunings(slot);
      signal_scene_change(slot_num + 3); // Zero indexed and first 4 are reserved for presets
      done = true;

    } else if (my2Button->wasPressed() || myXButton->wasPressed()) {
      return false;
    };
  };
  return true;
};

// This screen previews a preset's settings.
// Returns true if users wants to use it, false otherwise.
// Accept integer preset_num: the preset number.
bool view_preset_screen(int preset) {
  const int *tunings;
  if (preset == 1) { tunings = PRESET1; };
  if (preset == 2) { tunings = PRESET2; };
  if (preset == 3) { tunings = PRESET3; };
  if (preset == 4) { tunings = PRESET4; };

  String disp_str = " ---Preset Tuning--- \n"
                    " Hi Melody: " + LongNoteNum[tunings[0]] + "  \n"
                    " Lo Melody: " + LongNoteNum[tunings[1]] + "  \n"
                    " Drone:     " + LongNoteNum[tunings[2]] + "  \n"
                    " Trompette: " + LongNoteNum[tunings[3]] + "  \n"
                    " Tpose: ";

  if (tunings[5] > 12) { disp_str += "+"; };
  disp_str = disp_str + tunings[5] + "  Capo: ";
  if (tunings[6] > 0) { disp_str += "+"; };
  disp_str = disp_str + tunings[6] + "\n"
             " A or 1) Accept  \n"
             " X or 2) Go Back  \n";

  print_screen(disp_str);

  bool done = false;
  while (!done) {

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    myAButton->update();
    myXButton->update();

    if (my1Button->wasPressed() || myAButton->wasPressed()) {
      load_preset_tunings(preset);
      signal_scene_change(preset - 1); // Zero indexed
      done = true;

    } else if (my2Button->wasPressed() || myXButton->wasPressed()) {
      return false;
    };
  };
  return true;
};

// This screen asks the user to select a save slot for
// preview and optionally choosing.
bool load_saved_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " --Load Saved Slot-- \n"
                      " Select for Preview: \n"
                      "                     \n"
                      "     1) Slot 1       \n"
                      "     2) Slot 2       \n"
                      "     3) Slot 3       \n"
                      "     4) Slot 4       \n"
                      "X or 5) Go Back      \n";

    print_screen(disp_str);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    my4Button->update();
    my5Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {
      if (view_slot_screen(1)) { done = true; };

    } else if (my2Button->wasPressed()) {
      if (view_slot_screen(2)) { done = true; };

    } else if (my3Button->wasPressed()) {
      if (view_slot_screen(3)) { done = true; };

    } else if (my4Button->wasPressed()) {
      if (view_slot_screen(4)) { done = true; };

    } else if (my5Button->wasPressed() || myXButton->wasPressed()) {
      return false;
    };
  };
  return true;
};

// This screen lets the user choose a preset.
bool load_preset_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " ---Load A Preset--- \n"
                      " Select a Preset:    \n"
                      "                     \n"
                      " 1) " + PRESET1_NAME + "\n"
                      " 2) " + PRESET2_NAME + "\n"
                      " 3) " + PRESET3_NAME + "\n"
                      " 4) " + PRESET4_NAME + "\n"
                      " X or 5) Go Back     \n";

    print_screen(disp_str);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    my4Button->update();
    my5Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {
      if (view_preset_screen(1)) {done = true;};

    } else if (my2Button->wasPressed()) {
      if (view_preset_screen(2)) {done = true;};

    } else if (my3Button->wasPressed()) {
      if (view_preset_screen(3)) {done = true;};
    } else if (my4Button->wasPressed()) {
      if (view_preset_screen(4)) {done = true;};
    } else if (my5Button->wasPressed() || myXButton->wasPressed()) {
      return false;
    };
  };
  return true;
};

// This screen lets you choose how the gurdy indicates the turning to the controller
void scene_options_screen() {
  bool done = false;
  while (!done) {

    String disp_str = " -Scene Signalliing- \n"
                      " Select an Option:   \n"
                      "                     \n"
                      " 1) None             \n"
                      " 2) Prog Chg, Ch. 1  \n"
                      "                     \n"
                      " X) Go Back          \n";

    print_screen(disp_str);

    // Check the buttons
    my1Button->update();
    my2Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {
      scene_signal_type = 0;
      EEPROM.write(EEPROM_SCENE_SIGNALLING, scene_signal_type);
      done = true;
    } else if (my2Button->wasPressed()) {
      scene_signal_type = 1;
      EEPROM.write(EEPROM_SCENE_SIGNALLING, scene_signal_type);
      done = true;
    } else if (myXButton->wasPressed()) {
      done = true;
    }
  }
}

// This screen lets you choose what kind of display you want.
void playing_options_screen() {
  bool done = false;
  while (!done) {

    String disp_str = " -Choose Play Screen-\n"
                      " Select an Option:   \n"
                      "                     \n"
                      " 1) Big Note + Staff \n"
                      "                     \n"
                      " 2) Big Note         \n"
                      "                     \n"
                      " X) Go Back          \n";

    print_screen(disp_str);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {

      play_screen_type = 0;
      EEPROM.write(EEPROM_DISPLY_TYPE, 0);

      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      String disp_str = ""
      "  DISPLAY  \n"
      "  \n"
      "  SAVED \n";

      display.print(disp_str);
      display.display();
      delay(750);
      done = true;

    } else if (my2Button->wasPressed()) {

      play_screen_type = 1;
      EEPROM.write(EEPROM_DISPLY_TYPE, 1);

      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      String disp_str = ""
      "  DISPLAY  \n"
      "  \n"
      "   SAVED  \n";

      display.print(disp_str);
      display.display();
      delay(750);
      done = true;
    } else if (myXButton->wasPressed()) {
      done = true;
    };
  };
};

// This screen is for other setup options.  Currently, that's
// just an option to clear the EEPROM.
void options_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " ------Options------ \n"
                      " Select an Option:   \n"
                      "                     \n"
                      " 1) Clear EEPROM     \n"
                      " 2) Playing Screen   \n"
                      " 3) Scene Control    \n"
                      "                     \n"
                      " X) Go Back          \n";

    print_screen(disp_str);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {

      clear_eeprom();

      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      String disp_str = ""
      "  EEPROM  \n"
      "  \n"
      "  CLEARED \n";

      display.print(disp_str);
      display.display();
      delay(750);
      done = true;

    } else if (my2Button->wasPressed()) {
      playing_options_screen();
      done = true;
    } else if (my3Button->wasPressed()) {
      scene_options_screen();
      done = true;
    } else if (myXButton->wasPressed()) {
      done = true;
    };
  };
};

// This is the first screen after the credits n' stuff.
void welcome_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " -----DigiGurdy----- \n"
                      " Select an Option:   \n"
                      "                     \n"
                      " 1) Load Preset      \n"
                      " 2) Load Save Slot   \n"
                      " 3) New Tuning Setup \n"
                      " 4) Other Options    \n"
                      "                     \n";

    print_screen(disp_str);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    my4Button->update();

    if (my1Button->wasPressed()) {
      if (load_preset_screen()) {done = true;};

    } else if (my2Button->wasPressed()) {
      if (load_saved_screen()) {done = true;};

    } else if (my3Button->wasPressed()) {
      if (tuning()) {done = true;};

    } else if (my4Button->wasPressed()) {
      options_screen();
      // Not done = true here, we'd want to get prompted again.
    };
  };
};

// This screen prompts which kind of tuning to load.
bool load_tuning_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " ----Load Tuning---- \n"
                      "                     \n"
                      " 1) Preset Tuning    \n"
                      "                     \n"
                      " 2) Saved Tuning     \n\n"
                      " X or 3) Go Back     \n";

    print_screen(disp_str);

    delay(250);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {
      if (load_preset_screen()) {
        done = true;
      };

    } else if (my2Button->wasPressed()) {
      if (load_saved_screen()) {
        done = true;
      };

    } else if (my3Button->wasPressed() || myXButton->wasPressed()) {
      return false;
    };
  };

  return true;
};

// Checks if save slot is occupied and prompts user to overwrite if necessary.
// Returns true if slot is empty or OK to overwrite.
bool check_save_tuning(int slot) {

  if (EEPROM.read(slot) == 0) {
    return true;
  } else {

    String disp_str = " ----Save Tuning---- \n"
                      "                     \n"
                      " Save slot is full,  \n"
                      "    Save anyway?     \n"
                      "                     \n"
                      " 1) Overwrite        \n"
                      "                     \n"
                      " X or 2) Go Back     \n";

    print_screen(disp_str);

    bool done = false;
    while (!done) {

      my1Button->update();
      my2Button->update();
      myXButton->update();

      if (my1Button->wasPressed()) {
        return true;

      } else if (my2Button->wasPressed() || myXButton->wasPressed()) {
        return false;
      };
    };
    return false;
  };
};

// This is the screen for saving to a save slot
void save_tuning_screen() {

  bool done = false;
  int slot = 0;
  while (!done) {

    String disp_str = " ----Save Tuning---- \n"
                      " Choose a Save Slot: \n"
                      "                     \n"
                      "      1) Slot 1      \n"
                      "      2) Slot 2      \n"
                      "      3) Slot 3      \n"
                      "      4) Slot 4      \n"
                      "      X) Go Back     \n";

    print_screen(disp_str);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    my4Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {
      if (check_save_tuning(EEPROM_SLOT1)) {
        save_tunings(EEPROM_SLOT1);
        slot = 1;
        done = true;
      };

    } else if (my2Button->wasPressed()) {
      if (check_save_tuning(EEPROM_SLOT2)) {
        save_tunings(EEPROM_SLOT2);
        slot = 2;
        done = true;
      };

    } else if (my3Button->wasPressed()) {
      if (check_save_tuning(EEPROM_SLOT3)) {
        save_tunings(EEPROM_SLOT3);
        slot = 3;
        done = true;
      };

    } else if (my4Button->wasPressed()) {
      if (check_save_tuning(EEPROM_SLOT4)) {
        save_tunings(EEPROM_SLOT4);
        slot = 4;
        done = true;
      };

    } else if (myXButton->wasPressed()) {
      // Just return.
      return;
    };
  };

  // Display a confirmation message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("\n  Slot ");
  display.print(slot);
  display.print("\n  Saved!");
  display.display();
  delay(500);
};

void redetect_crank_screen() {

  // In case the user has already tried removing it and it's freaking out, let's kill the sound.
  mystring->soundKill();
  mylowstring->soundKill();
  mykeyclick->soundKill();
  mytromp->soundKill();
  mydrone->soundKill();
  mybuzz->soundKill();

  bool done = false;
  while (!done) {

    String disp_str = "                     \n"
                      "                     \n"
                      " Remove/Attach Crank \n"
                      " And press 1...      \n";

    print_screen(disp_str);

    my1Button->update();

    if (my1Button->wasPressed()) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println(" DigiGurdy");
      display.setTextSize(1);
      display.println(" --------------------");
      display.setTextSize(2);
      display.println("   Crank  ");

      if (mycrank->isDetected()) {
        display.println(" Detected ");
      } else {
        display.println("   Absent ");
      };

      display.display();
      delay(1000);

      done = true;
    };
  };
};

void options_about_screen() {

  about_screen();

  while (true) {
    myXButton->update();

    if(myXButton->wasPressed()) {
      break;
    };
  };

  String disp_str = "      DigiGurdy      "
                    "---------------------"
                    "Special Thanks:      "
                    "                     "
                    "John Dingley         "
                    "David Jacobs         "
                    "lune36400            "
                    "SalusaSecondus       ";

  print_screen(disp_str);

  while (true) {
    myXButton->update();

    if(myXButton->wasPressed()) {
      break;
    };
  };
};

bool other_options_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " ---Other Options--- \n"
                      "                     \n"
                      " 1) Remove/Attach    \n"
                      "      Crank          \n\n"
                      " 2) About DigiGurdy  \n\n"
                      " X or 3) Go Back     \n";

    print_screen(disp_str);

    my1Button->update();
    my2Button->update();
    my3Button->update();
    myXButton->update();

    if (my1Button->wasPressed()) {
      redetect_crank_screen();
      done = true;

    } else if (my2Button->wasPressed()) {
      options_about_screen();

    } else if (my3Button->wasPressed() || myXButton->wasPressed()) {
      return false;
    };
  };

  return true;
};

// This is the screen that X+A gets you.
void pause_screen() {

  bool done = false;
  while (!done) {

    String disp_str = " ----Pause  Menu---- \n"
                      " 1) Load    2) Save  \n"
                      " 3) Tuning  4) Other \n\n"
                      " X, 5 or ex1) Go Back\n\n";

    if (drone_mode == 0) {
      disp_str += "A) Drone:On ,Trmp:On \n";
    } else if (drone_mode == 1) {
      disp_str += "A) Drone:Off,Trmp:Off\n";
    } else if (drone_mode == 2) {
      disp_str += "A) Drone:On, Trmp:Off\n";
    } else if (drone_mode == 3) {
      disp_str += "A) Drone:Off,Trmp:On \n";
    };

    if (mel_mode == 0) {
      disp_str += "B) High:On ,  Low:On \n";
    } else if (mel_mode == 1) {
      disp_str += "B) High:On ,  Low:Off\n";
    } else if (mel_mode == 2) {
      disp_str += "B) High:Off,  Low:On \n";
    };

    print_screen(disp_str);

    delay(150);

    // Check the 1 and 2 buttons
    my1Button->update();
    my2Button->update();
    my3Button->update();
    my4Button->update();
    my5Button->update();
    myXButton->update();
    myAButton->update();
    myBButton->update();
    ex1Button->update();

    if (my1Button->wasPressed()) {
      if (load_tuning_screen()) {
        done = true;
      };

    } else if (my2Button->wasPressed()) {
      save_tuning_screen();
      done = true;

    } else if (my3Button->wasPressed()) {
      if (tuning()) {
        done = true;
      };

    } else if (my4Button->wasPressed()) {
      if (other_options_screen()) {
        done = true;
      };

    } else if (my5Button->wasPressed() || myXButton->wasPressed() || ex1Button->wasPressed()) {
      done = true;

    } else if (myAButton->wasPressed()) {
      if (drone_mode == 0) {
        drone_mode = 1; // 1 == both off
        mydrone->setMute(true);
        mytromp->setMute(true);
      } else if (drone_mode == 1) {
        drone_mode = 2; // 2 == drone on, tromp off
        mydrone->setMute(false);
        mytromp->setMute(true);
      } else if (drone_mode == 2) {
        drone_mode = 3; // 3 == drone off, tromp on
        mydrone->setMute(true);
        mytromp->setMute(false);
      } else if (drone_mode == 3) {
        drone_mode = 0; // 0 == both on
        mydrone->setMute(false);
        mytromp->setMute(false);
      };
    } else if (myBButton->wasPressed()) {
      if (mel_mode == 0) {
        mel_mode = 1; // 1 == high on, low off
        mystring->setMute(false);
        mylowstring->setMute(true);
      } else if (mel_mode == 1) {
        mel_mode = 2; // 2 == high off, low on
        mystring->setMute(true);
        mylowstring->setMute(false);
      } else if (mel_mode == 2) {
        mel_mode = 0; // 0 == high on, low on
        mystring->setMute(false);
        mylowstring->setMute(false);
      };
    };
  };

  // Crank On! for half a sec.
  display.clearDisplay();
  display.drawBitmap(0, 0, crank_on_logo, 128, 64, 1);
  display.display();
  delay(500);
};

// ###########
//  MAIN LOOP
// ###########

bool first_loop = true;

int test_count = 0;
int start_time = millis();

int stopped_playing_time = 0;
bool note_display_off = true;

// The loop() function is repeatedly run by the Teensy unit after setup() completes.
// This is the main logic of the program and defines how the strings, keys, click, buzz,
// and buttons acutally behave during play.
void loop() {
  // loop() actually runs too fast and gets ahead of hardware calls if it's allowed to run freely.
  // This was noticeable in older versions when the crank got "stuck" and would not buzz and took
  // oddly long to stop playing when the crank stopped moving.
  //
  // A microsecond delay here lets everything keep up with itself.  The exactly delay is set at
  // the top in the config section.
  //delayMicroseconds(LOOP_DELAY);

  if (first_loop) {
    welcome_screen();

    // This might have been reset above, so grab it now.
    play_screen_type = EEPROM.read(EEPROM_DISPLY_TYPE);

    // Crank On! for half a sec.
    display.clearDisplay();
    display.drawBitmap(0, 0, crank_on_logo, 128, 64, 1);
    display.display();
    delay(750);

    print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, 0, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    first_loop = false;
  };

  // Update the keys, buttons, and crank status (which includes the buzz knob)
  myoffset = mygurdy->getMaxOffset();  // This covers the keybox buttons.
  bigbutton->update();
  mycrank->update();
  tpose_up->update();
  tpose_down->update();
  capo->update();

  myAButton->update();
  myXButton->update();

  ex1Button->update();
  ex2Button->update();
  ex3Button->update();

  // If the "X" and "O" buttons are both down, or if the first extra button is pressed,
  // trigger the tuning menu
  if ((myAButton->beingPressed() && myXButton->beingPressed()) || ex1Button->wasPressed()) {

    // Turn off the sound :-)
    mystring->soundOff();
    mylowstring->soundOff();
    mykeyclick->soundOff();  // Not sure if this is necessary... but it feels right.
    mytromp->soundOff();
    mydrone->soundOff();
    mybuzz->soundOff();

    // If I don't do this, it comes on afterwards.
    bigbutton->setToggle(false);

    pause_screen();
    print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(),
                 tpose_offset, capo_offset, 0, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
  };

  // Check for a capo shift.
  if (capo->wasPressed() ||
      (myXButton->beingPressed() && myBButton->wasPressed())) {

    capo_offset += 2;

    if (capo_offset > max_capo ) {
      capo_offset = 0;
    };

    if (mycrank->isSpinning() || bigbutton->toggleOn()) {
      mytromp->soundOff();
      mydrone->soundOff();
      mystring->soundOff();
      mylowstring->soundOff();
      mykeyclick->soundOff();

      mystring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mylowstring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mykeyclick->soundOn(tpose_offset);
      mytromp->soundOn(tpose_offset + capo_offset);
      mydrone->soundOn(tpose_offset + capo_offset);

      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);
    } else {
      print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(),
                 tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    };
  };

  // As long as we're in playing mode--acutally playing or not--
  // check for a tpose shift.
  if ((tpose_up->wasPressed() ||
      (my1Button->beingPressed() && myAltTposeUp->wasPressed())) && (tpose_offset < max_tpose)) {
    tpose_offset += 1;

    if (mycrank->isSpinning() || bigbutton->toggleOn()) {
      mytromp->soundOff();
      mydrone->soundOff();
      mystring->soundOff();
      mylowstring->soundOff();
      mykeyclick->soundOff();

      mystring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mylowstring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mykeyclick->soundOn(tpose_offset);
      mytromp->soundOn(tpose_offset + capo_offset);
      mydrone->soundOn(tpose_offset + capo_offset);

      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);
    } else {
      print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(),
                   tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    };
  };
  if ((tpose_down->wasPressed() ||
      (my1Button->beingPressed() && myAltTposeDown->wasPressed())) && (max_tpose + tpose_offset > 0)) {
    tpose_offset -= 1;

    if (mycrank->isSpinning() || bigbutton->toggleOn()) {
      mytromp->soundOff();
      mydrone->soundOff();
      mystring->soundOff();
      mylowstring->soundOff();
      mykeyclick->soundOff();

      mystring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mylowstring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mykeyclick->soundOn(tpose_offset);
      mytromp->soundOn(tpose_offset + capo_offset);
      mydrone->soundOn(tpose_offset + capo_offset);

      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);
    } else {
      print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(),
                   tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    };
  };

  // If ex2 is pressed during play, cycle through the melody string on/off options.
  if (ex2Button->wasPressed()) {
    if (mel_mode == 0) {
      mel_mode = 1; // 1 == high on, low off
      mystring->setMute(false);
      mylowstring->setMute(true);
      if (mylowstring->isPlaying()) {
        mylowstring->soundOff();
        mylowstring->soundOn();
      };
    } else if (mel_mode == 1) {
      mel_mode = 2; // 2 == high off, low on
      mystring->setMute(true);
      mylowstring->setMute(false);
      if (mystring->isPlaying()) {
        mystring->soundOff();
        mystring->soundOn();
        mylowstring->soundOff();
        mylowstring->soundOn();
      };
    } else if (mel_mode == 2) {
      mel_mode = 0; // 0 == high on, low on
      mystring->setMute(false);
      mylowstring->setMute(false);
      if (mystring->isPlaying()) {
        mystring->soundOff();
        mystring->soundOn();
      };
    };
    if (mystring->isPlaying()) {
      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);
    } else {
      print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    };
  };

  // If ex3 is pressed during play, cycle thought the drone/trompette on/off options.
  if (ex3Button->wasPressed()) {
    if (drone_mode == 0) {
      drone_mode = 1; // 1 == both off
      mydrone->setMute(true);
      mytromp->setMute(true);
      if (mydrone->isPlaying()) {
        mydrone->soundOff();
        mydrone->soundOn();
        mytromp->soundOff();
        mytromp->soundOn();
      };
    } else if (drone_mode == 1) {
      drone_mode = 2; // 2 == drone on, tromp off
      mydrone->setMute(false);
      mytromp->setMute(true);
      if (mydrone->isPlaying()) {
        mydrone->soundOff();
        mydrone->soundOn();
      };
    } else if (drone_mode == 2) {
      drone_mode = 3; // 3 == drone off, tromp on
      mydrone->setMute(true);
      mytromp->setMute(false);
      if (mydrone->isPlaying()) {
        mydrone->soundOff();
        mydrone->soundOn();
        mytromp->soundOff();
        mytromp->soundOn();
      };
    } else if (drone_mode == 3) {
      drone_mode = 0; // 0 == both on
      mydrone->setMute(false);
      mytromp->setMute(false);
      if (mydrone->isPlaying()) {
        mydrone->soundOff();
        mydrone->soundOn();
      };
    };
    if (mystring->isPlaying()) {
      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);
    } else {
      print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    };
  };

  // NOTE:
  // We don't actually do anything if nothing changed this cycle.  Strings stay on/off automatically,
  // and the click sound goes away because of the sound in the soundfont, not the length of the
  // MIDI note itself.  We just turn that on and off like the other strings.

  // If the big button is toggled on or the crank is active (i.e., if we're making noise):
  if (bigbutton->toggleOn() || mycrank->isSpinning()) {

    // Turn on the strings without a click if:
    // * We just hit the button and we weren't cranking, OR
    // * We just started cranking and we hadn't hit the button.
    if (bigbutton->wasPressed() && !mycrank->isSpinning()) {
      mystring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mylowstring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mytromp->soundOn(tpose_offset + capo_offset);
      mydrone->soundOn(tpose_offset + capo_offset);
      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);

    } else if (mycrank->startedSpinning() && !bigbutton->toggleOn()) {
      mystring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mylowstring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mytromp->soundOn(tpose_offset + capo_offset);
      mydrone->soundOn(tpose_offset + capo_offset);
      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);

    // Turn off the previous notes and turn on the new one with a click if new key this cycle.
    // NOTE: I'm not touching the drone/trompette.  Just leave it on if it's a key change.
    } else if (mygurdy->higherKeyPressed() || mygurdy->lowerKeyPressed()) {
      mystring->soundOff();
      mylowstring->soundOff();
      mykeyclick->soundOff();

      mystring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mylowstring->soundOn(myoffset + tpose_offset, MELODY_VIBRATO);
      mykeyclick->soundOn(tpose_offset);
      draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type);
    };

    // Whenever we're playing, check for buzz.
    if (mycrank->startedBuzzing()) {
      mybuzz->soundOn(tpose_offset + capo_offset);
    };

    if (mycrank->stoppedBuzzing()) {
      mybuzz->soundOff();
    };

  // If the toggle came off and the crank is off, turn off sound.
  } else if (bigbutton->wasReleased() && !mycrank->isSpinning()) {
    mystring->soundOff();
    mylowstring->soundOff();
    mykeyclick->soundOff();  // Not sure if this is necessary... but it feels right.
    mytromp->soundOff();
    mydrone->soundOff();
    mybuzz->soundOff();

    // Send a CC 123 (all notes off) to be sure.  This causes turning off sound via the big
    // button to basically be a MIDI kill button.
    mystring->soundKill();
    mylowstring->soundKill();
    mykeyclick->soundKill();
    mytromp->soundKill();
    mydrone->soundKill();
    mybuzz->soundKill();

    // Mark the time we stopped playing and trip the turn-off-the-display flag
    stopped_playing_time = millis();
    note_display_off = false;

  // If the crank stops and the toggle was off, turn off sound.
  } else if (mycrank->stoppedSpinning() && !bigbutton->toggleOn()) {
    mystring->soundOff();
    mylowstring->soundOff();
    mykeyclick->soundOff();
    mytromp->soundOff();
    mydrone->soundOff();
    mybuzz->soundOff();

    stopped_playing_time = millis();
    note_display_off = false;
  };

  // Once the sound stops, track the time until a fifth of a second has passed, then go back to the
  // non-playing display.  This just makes the display looks a bit nicer even if the sound jitters a
  // little.  It's subtle but I like the touch.
  if (!note_display_off && !bigbutton->toggleOn() && !mycrank->isSpinning()) {
    if ((millis() - stopped_playing_time) > 200) {
      note_display_off = true;
      print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
    };
  };

  // Apparently we need to do this to discard incoming data.
  while (myMIDI->read()) {
  };
  while (usbMIDI.read()) {
  };

  test_count +=1;
  if (test_count > 100000) {
    test_count = 0;
    Serial.print("100,000 loop()s took: ");
    Serial.print(millis() - start_time);
    Serial.print("ms.  Avg Velocity: ");
    Serial.print(mycrank->getVAvg());
    Serial.print("rpm. Transitions: ");
    Serial.print(mycrank->getCount());
    Serial.print(", est. rev: ");
    Serial.println(mycrank->getRev());
    start_time = millis();
  }
};
