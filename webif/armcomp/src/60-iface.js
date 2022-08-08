/* Copyright 2022 Peter Wagener <mail@peterwagener.net>

This file is part of Freyr2.

Freyr2 is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Freyr2 is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Freyr2. If not, see <https://www.gnu.org/licenses/>.
*/



currentPreset = {};
brightnessValue = 0;
cellCommands = {};

function activatePreset(preset) {
  currentPreset = {};
  if (preset.animation) {
    const anim = animations[preset.animation];
    currentPreset.animation = preset.animation;
    for (const [k, v] of Object.entries(anim.sliders)) {
      currentPreset[k] = preset.hasOwnProperty(k) ? preset[k] : v.default;
    }
    if (anim.enums) for (const [k, v] of Object.entries(anim.enums)) {
      currentPreset[k] = preset.hasOwnProperty(k) ? preset[k] : v.default;
    }
  } else {
    currentPreset.animation = preset.ident;
    currentPreset.suffix = preset.suffix;
    for (const [k, v] of Object.entries(preset.sliders)) {
      currentPreset[k] = v.default;
    }
    if (preset.enums) for (const [k, v] of Object.entries(preset.enums)) {
      if (v.default) currentPreset[k] = v.default;
    }
  }
}

function emitCommand(cmd) {
  console.log("command", cmd);
  var xhttp = new XMLHttpRequest();
  xhttp.open("POST", "/", true);
  xhttp.send(cmd);
}

function applySettings() {
  if (Object.keys(activeCells).length < 1) return;
  const sel = buildSelector();
  if (sel.length < 1) return;

  if (currentPreset.animation) {
    let command = currentPreset.animation;
    if (currentPreset.suffix) command += " " + currentPreset.suffix;
    const anim = animations[currentPreset.animation];
    for (const key of Object.keys(anim.sliders)) command += " " + key + " " + currentPreset[key];
    if (anim.enums) for (const [key, data] of Object.entries(anim.enums)) {
      if (data.command) command += " " + data.command;
      command += " " + currentPreset[key];
    }
    for (const cell of Object.keys(activeCells)) cellCommands[cell] = command;
    emitCommand("display " + command + sel);
  } else if (currentPreset.mode) {
    switch (currentPreset.mode) {
      case "brightness":
        emitCommand("brightness " + brightnessValue + sel);
        break;
    }
  }
}

function fullscreen() {
  var elem = document.documentElement;
  if (elem.requestFullscreen) {
    elem.requestFullscreen();
  } else if (elem.webkitRequestFullscreen) { /* Safari */
    elem.webkitRequestFullscreen();
  } else if (elem.msRequestFullscreen) { /* IE11 */
    elem.msRequestFullscreen();
  }
}
function refreshPage() {
  window.location.replace(window.location.pathname + window.location.search + window.location.hash);
}

function updateBattery() {
  navigator.getBattery().then((bat) => {
    document.getElementById("battery").innerText = Math.floor(bat.level * 100) + "%";
    setTimeout(updateBattery, 60000);
  });
}

function init() {
  const storedPresets = localStorage.getItem("presets");
  const storedThemes = localStorage.getItem("themes");
  const storedHue = localStorage.getItem("hue");
  if (storedPresets) presets = JSON.parse(storedPresets);
  if (storedThemes) themes = JSON.parse(storedThemes);
  if (storedHue) document.documentElement.style.setProperty("--hue", storedHue);

  buildScreens();
  updateScreen();
  updateBattery();
}
