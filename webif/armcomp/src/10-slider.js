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


class SliderElement extends HTMLElement {
  constructor() {
    super();


  }
  connectedCallback() {
    this.classList.add("k-leaf");

    this.min = this.hasAttribute("min") ? parseFloat(this.getAttribute("min")) : 0;
    this.max = this.hasAttribute("max") ? parseFloat(this.getAttribute("max")) : 1;
    this.openMin = this.hasAttribute("open-min") || this.hasAttribute("open");
    this.openMax = this.hasAttribute("open-max") || this.hasAttribute("open");

    this.attachShadow({ mode: 'open' });

    const container = document.createElement("div");
    container.style.width = "100%";
    container.style.height = "100%";


    this.lhs = document.createElement("div");
    this.lhs.setAttribute("part", "lhs");
    container.appendChild(this.lhs);

    this.shadowRoot.appendChild(container);

    container.owner = this;
    container.addEventListener("mousedown", this);
    container.addEventListener("touchstart", this);
    this.addEventListener("keydown", this);
    this.addEventListener("msg", this);

    this.activeButton = null;
    this.activeTouch = null;
    this.lastPos = 0;
    this.sliderChanged = false;
    this.sliderClickValue = null;

    this.setAttribute("tabindex", "0");
    this.value = null;
    this.setValue(this.hasAttribute("value") ? parseFloat(this.getAttribute("value")) : this.min);
    this.immediate = this.hasAttribute("immediate");
  }

  fireCallbacks(isImmediate) {
    if (isImmediate == this.immediate) {
      this.dispatchEvent(new Event("change"));
    }

  }

  setValue(v, isImmediate = true) {
    if ((v < this.min) && !this.openMin) v = this.min;
    if ((v > this.max) && !this.openMax) v = this.max;

    if (this.value == v) return;
    this.value = v;

    const v_clamped = Math.min(Math.max(this.min, v), this.max);
    const v_rel = v_clamped / (this.max - this.min);
    this.lhs.style.setProperty("--pixval", this.getPixelScale() * v_rel + "px");
    this.lhs.style.setProperty("--raw", v);
    this.lhs.style.setProperty("--clamped", v_clamped);
    this.lhs.style.setProperty("--relative", v_rel);

    this.fireCallbacks(isImmediate);
  }

  slideBy(delta) {
    this.setValue(this.value + delta);
  }

  isSliding() {
    return this.activeButton != null || this.activeTouch != null;
  }


  handleEvent(ev) {
    switch (ev.type) {
      case "touchstart":
        if (this.activeTouch != null) {
          ev.preventDefault();
          return;
        }
        if (this.isSliding()) return;
        this.focus();

        this.activeTouch = ev.changedTouches[0].identifier;
        this.lastPos = this.decodeScrollPosition(ev.changedTouches[0]);
        this.sliderChanged = false;
        this.sliderClickValue = this.decodeScrollValue(ev.changedTouches[0]);
        window.addEventListener("touchmove", this);
        window.addEventListener("touchend", this);
        ev.preventDefault();
        break;
      case "touchmove":
        for (const touch of ev.touches) {
          if (touch.identifier != this.activeTouch) continue;
          const p1 = this.decodeScrollPosition(touch);
          const dv = this.decodeScrollDelta(p1 - this.lastPos);
          if (dv != 0) {
            this.sliderChanged = true;
            this.slideBy(dv);
          }
          this.lastPos = p1;
          break;
        }
        break;
      case "touchend":
        if (ev.touches.length < 1) {
          window.removeEventListener("touchmove", this);
          window.removeEventListener("touchend", this);
          this.activeTouch = null;
          if (!this.sliderChanged) this.setValue(this.sliderClickValue, true);
          this.fireCallbacks(false);
        }
        break;
      case "mousedown":
        if (this.isSliding()) return;
        this.focus();
        this.activeButton = ev.button;
        this.lastPos = this.decodeScrollPosition(ev);
        this.sliderChanged = false;
        this.sliderClickValue = this.decodeScrollValue(ev);
        window.addEventListener("mousemove", this);
        window.addEventListener("mouseup", this);
        ev.preventDefault();
        break;
      case "mousemove":
        const p1 = this.decodeScrollPosition(ev);
        const dv = this.decodeScrollDelta(p1 - this.lastPos);
        if (dv != 0) {
          this.sliderChanged = true;
          this.slideBy(dv);
        }
        this.lastPos = p1;
        ev.preventDefault();
        break;
      case "mouseup":
        if (this.activeButton == ev.button) {
          window.removeEventListener("mousemove", this);
          window.removeEventListener("mouseup", this);
          this.activeButton = null;
          if (!this.sliderChanged) this.setValue(this.sliderClickValue, true);
          this.fireCallbacks(false);
          ev.preventDefault();
        }
        break;


    }
  }
}

class VSliderElement extends SliderElement {
  constructor() {
    super();
  }

  decodeScrollPosition(data) {
    return data.screenY;
  }
  decodeScrollDelta(delta) {
    return -(delta) * (this.max - this.min) / (this.clientHeight);
  }
  decodeScrollValue(data) {
    const y = data.pageY - this.getBoundingClientRect().y - window.pageYOffset;
    return (this.clientHeight - y) * (this.max - this.min) / (this.clientHeight);
  }
  getPixelScale() {
    return this.clientHeight;
  }
}

customElements.define("k-vslider", VSliderElement)