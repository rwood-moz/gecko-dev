/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This test makes sure that the URL bar is focused when entering the private window.

"use strict";

function checkUrlbarFocus(win) {
  let urlbar = win.gURLBar;
  is(win.document.activeElement, urlbar.inputField, "URL Bar should be focused");
  is(urlbar.value, "", "URL Bar should be empty");
}

function openNewPrivateWindow() {
  let deferred = Promise.defer();
  whenNewWindowLoaded({private: true}, win => {
    executeSoon(() => deferred.resolve(win));
  });
  return deferred.promise;
}

add_task(function* () {
  let win = yield openNewPrivateWindow();
  checkUrlbarFocus(win);
  win.close();
});

add_task(function* () {
  Services.prefs.setCharPref("browser.newtab.url", "about:blank");
  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("browser.newtab.url");
  });

  let win = yield openNewPrivateWindow();
  checkUrlbarFocus(win);
  win.close();

  Services.prefs.clearUserPref("browser.newtab.url");
});
