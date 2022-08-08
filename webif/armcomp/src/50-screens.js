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

screenStack = ["root"];
sliderTitle = null;

screens = {
  root: {
    builtin: true,
    buttons: {
      DPLY: () => { pushScreen('display'); },
      BRGT: () => {
        setSlider({
          title: "brightness",
          min: 0,
          max: 1,
          getter: () => { return brightnessValue; },
          setter: (v) => { brightnessValue = v; }
        });
        currentPreset = { mode: "brightness" };
      },
      THME: () => { pushScreen('themes'); },
      PRST: () => { pushScreen('presets'); },
      STHM: () => {
        const ident = prompt("theme name");
        if (!ident) return;
        if (themes.hasOwnProperty(ident) && !confirm("theme " + ident + " exists already. Overwrite?")) return;
        let theme = {};
        for (const [cell, cmd] of Object.entries(cellCommands)) {
          if (activeCells.hasOwnProperty(cell)) {
            theme[cell] = cmd;
          }
        }
        if (Object.keys(theme).length > 0) {
          themes[ident] = theme;
          localStorage.setItem("themes", JSON.stringify(themes));
          buildScreens();
          updateScreen();
        }
      },
      CUE: () => { },
      HUE: () => {
        setSlider({
          title: "hue",
          min: 0,
          max:360,
          getter: () => { return parseFloat(getComputedStyle(document.documentElement).getPropertyValue("--hue")); },
          setter: (v) => { console.log(v); document.documentElement.style.setProperty("--hue", v); localStorage.setItem("hue", "" + v); }
        })
      }
    }
  }
}

function addScreen(ident) {
  let res = { buttons: {} };
  screens[ident] = res;
  return res;
}


function addDisplayScreen(ident, preset) {

  const isPreset = preset.hasOwnProperty("animation");

  const idRoot = isPreset ? "presets" : "display";
  let root = screens[idRoot];

  let anim = isPreset ? animations[preset.animation] : preset;
  {
    const idAnimScreen = idRoot + '.' + ident;
    let animScreen = addScreen(idAnimScreen);
    root.buttons[preset.button] = () => { activatePreset(preset); pushScreen(idAnimScreen); };

    for (const [idSlider, slider] of Object.entries(anim.sliders)) {
      const idSliderScreen = idAnimScreen + ".slider-" + idSlider;
      let sliderScreen = addScreen(idSliderScreen);
      animScreen.isAnimation = true;
      animScreen.buttons.PRST = () => {
        const id = prompt("preset name");
        if (!id) return;
        if (presets.hasOwnProperty(id) && !confirm("preset " + id + " exists already. Overwrite?")) return;
        currentPreset.button = id;
        presets[id] = clone(currentPreset);
        localStorage.setItem("presets", JSON.stringify(presets));
        buildScreens();
        updateScreen();
      }

      animScreen.buttons[idSlider] = () => {
        setSlider({
          title: idSlider,
          min: slider.min,
          max: slider.max,
          getter: () => { return currentPreset[idSlider]; },
          setter: (v) => { currentPreset[idSlider] = v; }
        });
      };
    }

    if (anim.enums) {
      for(const [idEnum,data] of Object.entries(anim.enums)) {
        for (const option of data.options) {
          animScreen.buttons[option] = () => {
            currentPreset[idEnum] = option;
          }
        }
      }
    }
  }
}

function makeButton(name, callback) {
  let elButtons = document.getElementById("buttons");
  let el = document.createElement("li");
  let lbl = document.createElement("p");
  lbl.innerText = name;
  el.appendChild(lbl);
  el.onclick = callback;
  elButtons.appendChild(el);
  return el;
}

function updateBreadcrumbs() {
  const idScr = screenStack[screenStack.length - 1];
  let trail = idScr;
  if (sliderTitle) trail += ":" + sliderTitle;
  let el = document.getElementById("breadcrumbs");

  el.innerText = trail;
}
function updateScreen() {
  const idScr = screenStack[screenStack.length - 1];
  const scr = screens[idScr];

  updateBreadcrumbs();

  let elButtons = document.getElementById("buttons");

  elButtons.innerHTML = "";

  if (screenStack.length > 1) {
    makeButton("HOME", () => { screenStack = ["root"]; updateScreen(); });
    makeButton("BACK", popScreen);
  }

  if (scr.buttons) {
    for (const [ident, callback] of Object.entries(scr.buttons)) {
      makeButton(ident, callback);
    }
  }

  setSlider(screen.slider);

}

function pushScreen(ident) {
  if (!screens.hasOwnProperty(ident)) {
    console.log("pushScreen: invalid screen", ident);
    return;
  }

  screenStack.push(ident);

  updateScreen();

}

function popScreen() {
  if (screenStack.length < 2) {
    console.log("popScreen: attempted to pop root");
    return;
  }

  screenStack.pop();
  updateScreen();
}

function setSlider(slider) {
  let elSlider = document.getElementById("slider");
  elSlider.onchange = null;
  sliderTitle = null;
  if (!slider) {
    elSlider.style.display = "none";
  } else {
    elSlider.style.display = "block";
    if (slider.title) sliderTitle = slider.title;
    elSlider.min = slider.min;
    elSlider.max = slider.max;
    elSlider.setValue(slider.getter());
    elSlider.onchange = (ev) => { slider.setter(ev.currentTarget.value); }
  }

  updateBreadcrumbs();
}

function buildScreens() {
  currentPreset = {};
  sliderTitle = null;
  screenStack = ["root"];
  { // clear auto-generated screens
    let del = [];
    for (const [key, scr] of Object.entries(screens)) {
      if (!scr.builtin) del.push(key);
    }
    for (const key of del) delete screens[key];
  }

  addScreen("display");
  addScreen("presets");
  let scrThemes = addScreen("themes");

  for (const [id, anim] of Object.entries(animations)) addDisplayScreen(id, anim);
  for (const [id, anim] of Object.entries(presets)) addDisplayScreen(id, anim);

  for (const [id, cmds] of Object.entries(themes)) {
    scrThemes.buttons[id] = () => {
      let groupedCommands = {};
      for (const [cell, cmd] of Object.entries(cmds)) {
        if (activeCells.hasOwnProperty(cell)) {
          if (!groupedCommands[cmd]) groupedCommands[cmd] = cmd;
          groupedCommands[cmd] += " on " + cell;
          cellCommands[cell] = cmd;
        }
      }
      for (const cmd of Object.values(groupedCommands)) emitCommand("display " + cmd);
    };
  }
}
