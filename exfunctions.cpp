#include "exfunctions.h"

/// @defgroup ex EX Button Functions
/// These are the various functions that the EX buttons can be configured to run when pressed.
/// @note One EX button use isn't listed here: bringing up the pause screen.
/// @{

/// @brief Cycles through muting the melody strings.
/// @details
/// * Applies immediately if playing.
/// * Cycles through one or the other or no melody strings.
/// * Does not include an option to mute all melody strings (why have that?).
void cycle_mel_mute() {
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
    draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type, false);
  } else {
    print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
  };
};

/// @brief Cycles through muting the drone and trompette strings.
/// @details
/// * Applies immediately if playing.
/// * Cycles through all 4 mute/unmute possibilities between the two strings.
void cycle_drone_tromp_mute() {
  if (drone_mode == 0) {
    drone_mode = 1; // 1 == both off
    mydrone->setMute(true);
    mytromp->setMute(true);
    mybuzz->setMute(true);
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
    mybuzz->setMute(true);
    if (mydrone->isPlaying()) {
      mydrone->soundOff();
      mydrone->soundOn();
    };
  } else if (drone_mode == 2) {
    drone_mode = 3; // 3 == drone off, tromp on
    mydrone->setMute(true);
    mytromp->setMute(false);
    mybuzz->setMute(false);
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
    mybuzz->setMute(false);
    if (mydrone->isPlaying()) {
      mydrone->soundOff();
      mydrone->soundOn();
    };
  };
  if (mystring->isPlaying()) {
    draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type, false);
  } else {
    print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
  };
};

/// @brief Toggles muting the drone string.
/// @details * Applies immediately if playing.
void cycle_drone_mute() {
  if (drone_mode == 0 || drone_mode == 2) {
    if (drone_mode == 0) {
      drone_mode = 3;
    } else {
      drone_mode = 1;
    };
    mydrone->setMute(true);
    if (mydrone->isPlaying()) {
      mydrone->soundOff();
      mydrone->soundOn();
    };
  } else if (drone_mode == 1 || drone_mode == 3) {
    if (drone_mode == 1) {
      drone_mode = 2;
    } else {
      drone_mode = 0; // 0 = On
    };
    mydrone->setMute(false);
    if (mydrone->isPlaying()) {
      mydrone->soundOff();
      mydrone->soundOn();
    };
  };
  if (mystring->isPlaying()) {
    draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type, false);
  } else {
    print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
  };
};

/// @brief Toggles muting the trompette string.
/// @details * Applies immediately if playing.
void cycle_tromp_mute() {
  if (t_mode == 0) {
    t_mode = 1;
    mytromp->setMute(true);
    mybuzz->setMute(true);
    if (mytromp->isPlaying()) {
      mytromp->soundOff();
      mytromp->soundOn();
    };
  } else if (t_mode == 1) {
    t_mode = 0;
    mytromp->setMute(false);
    mybuzz->setMute(false);
    if (mytromp->isPlaying()) {
      mytromp->soundOff();
      mytromp->soundOn();
    };
  };
  if (mystring->isPlaying()) {
    draw_play_screen(mystring->getOpenNote() + tpose_offset + myoffset, play_screen_type, false);
  } else {
    print_display(mystring->getOpenNote(), mylowstring->getOpenNote(), mydrone->getOpenNote(), mytromp->getOpenNote(), tpose_offset, capo_offset, myoffset, mystring->getMute(), mylowstring->getMute(), mydrone->getMute(), mytromp->getMute());
  };
};

/// @brief Turns the volume down by 10 if possible on all strings.
/// @version *New in 2.3.3*
void turn_volume_down() {
  
  vol_down();
};

/// @brief Turns the volume up by 10 if possible on all strings.
/// @version *New in 2.3.3*
void turn_volume_up() {

  vol_up();
};

/// @brief Cycles the capo, and adjusts any playing notes.
/// @version *New in  2.3.7*
void ex_cycle_capo(bool playing) {
  cycle_capo(playing);
};

/// @brief Increases the transpose by 1 semitone, and adjusts any playing notes.
/// @version *New in  2.3.7*
void ex_tpose_up(bool playing) {
  tpose_up_1(playing);
};

/// @brief Decreases the transpose by 1 semitone, and adjusts any playing notes.
/// @version *New in  2.3.7*
void ex_tpose_down(bool playing) {
  tpose_down_1(playing);
};

/// @brief Toggles the transpose up or down a number of steps
/// @param playing 
/// @param toggled 
/// @version *New in 2.5.7*
void ex_tpose_toggle(bool playing, int steps) {
  if (tpose_offset != steps) {
    tpose_up_x(playing, steps);
  } else {
    tpose_up_x(playing, 0);
  };
};

/// @}
