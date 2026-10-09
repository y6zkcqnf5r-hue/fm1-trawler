// SPDX-License-Identifier: GPL-3.0-only
// The computer keyboard of the virtual FM-1 (spec §4.5), by KeyboardEvent.code, i.e. by physical
// position whatever the layout (Z is W on an AZERTY keyboard). A plain script: the page loads it with
// <script src>, tests/host/emu_test.mjs imports it; both read globalThis.TRAWLER_KEYMAP.
//
// The musical keyboard is a piano drawn from F, as the FM-1's own keys (key 0 = F3):
//   Z X C V B N M        F3 .. E4 (white)    S D F H J   F#3 G#3 A#3 C#4 D#4
//   Q W E R T Y U I O    F4 .. G5 (white)    2 3 4 6 7 9 F#4 G#4 A#4 C#5 D#5 F#5
// The digits are also meant for KNOB1-4 -/+ (spec §4.5): Q-17 settles it when the knobs arrive.
// Buttons: - and = OCT- / OCT+, Shift SEL. The rest of spec §4.5 arrives with P0-06.
(function () {
  const B = { OCTDN: 0, OCTUP: 1, SEL: 3 };   // B_* of firmware/src/app/plat.h
  const keys = {};
  const row = (codes, first) => codes.forEach((c, i) => (keys[c] = first + i));
  // chromatic from F3, white and black codes interleaved (no black key between B and C, E and F)
  row(["KeyZ", "KeyS", "KeyX", "KeyD", "KeyC", "KeyF", "KeyV", "KeyB", "KeyH", "KeyN", "KeyJ", "KeyM"], 0);
  row(["KeyQ", "Digit2", "KeyW", "Digit3", "KeyE", "Digit4", "KeyR", "KeyT", "Digit6", "KeyY", "Digit7", "KeyU",
       "KeyI", "Digit9", "KeyO"], 12);
  globalThis.TRAWLER_KEYMAP = {
    keys,                                     // code -> key index, 0 = F3 .. 26 = G5
    buttons: { Minus: B.OCTDN, Equal: B.OCTUP, ShiftLeft: B.SEL, ShiftRight: B.SEL },   // code -> B_*
  };
})();
