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

activeCells = {};

function selToggle(id) {
  let el = document.getElementById("cellIndicator_" + id);
  if (activeCells[id]) {
    delete activeCells[id];
    el.style.opacity = 0.01;
  } else {
    activeCells[id] = 1;
    el.style.opacity = 0.405;
  }
}

function selToggleAll() {
  if (Object.keys(activeCells).length < 1) {
    for (const cell of Object.keys(allCells)) selToggle(cell);
  } else {
    let tmp = [];
    for (const cell of Object.keys(activeCells)) tmp.push(cell);
    for (const cell of tmp) selToggle(cell);
  }
}

function buildSelector() {
  let sel = "";
  for (const cell of Object.keys(activeCells)) sel += " on " + cell;
  return sel;
}