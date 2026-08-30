
var KEY_BRAND_TEXT = 0;
var KEY_WEATHER_LABEL = 1;
var KEY_SHOW_INFO = 2;
var KEY_BIG_TIME = 3;
var KEY_WEATHER_TEMP = 4;
var KEY_TIME_24H = 5;
var KEY_ANIMATIONS = 6;
var KEY_FRUIT_PUNS = 7;
var KEY_TEMP_UNIT = 8;
var KEY_BUZZ = 9;

var CONFIG_URL = 'https://h3nry.xyz/soy/index.html';

var WEATHER_REFRESH_MS = 45 * 60 * 1000;

function wmoLabel(code) {
  if (code === 0) return 'Clear';
  if (code === 1) return 'Mostly Clear';
  if (code === 2) return 'Part Cloudy';
  if (code === 3) return 'Cloudy';
  if (code === 45 || code === 48) return 'Fog';
  if (code >= 51 && code <= 57) return 'Drizzle';
  if (code >= 61 && code <= 67) return 'Rain';
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) return 'Snow';
  if (code >= 80 && code <= 82) return 'Showers';
  if (code >= 95) return 'Storm';
  return 'Mystery';
}

function cacheGet(k) {
  try { return window.localStorage.getItem(k); } catch (e) { return null; }
}
function cacheSet(k, v) {
  try { window.localStorage.setItem(k, v); } catch (e) { }
}

function sendWeather(label, temp) {
  cacheSet('soyface-weather', label);
  if (temp !== null && !isNaN(temp)) {
    cacheSet('soyface-temp', String(Math.round(temp)));
  }
  var msg = {};
  msg[KEY_WEATHER_LABEL] = label;
  if (temp !== null && !isNaN(temp)) {
    msg[KEY_WEATHER_TEMP] = Math.round(temp);
  }
  Pebble.sendAppMessage(
    msg,
    function () { console.log('weather: ' + label + ' ' + temp); },
    function (e) { console.log('weather send failed: ' + e.error.message); }
  );
}

function httpGet(url, onOk, onFail) {
  var finished = false;
  function fail(why) {
    if (!finished) { finished = true; console.log('http fail: ' + why); onFail(); }
  }
  function ok(text) {
    if (!finished) {
      finished = true;
      try { onOk(JSON.parse(text)); } catch (e) { fail('json'); }
    }
  }
  try {
    var x = new XMLHttpRequest();
    x.open('GET', url, true);
    x.onreadystatechange = function () {
      if (x.readyState === 4) {
        if (x.status === 200) ok(x.responseText);
        else fail('status ' + x.status);
      }
    };
    setTimeout(function () { fail('timeout'); }, 12000);
    x.send();
  } catch (e) {
    fail(e && e.message ? e.message : 'xhr');
  }
}

function fetchWeather() {
  if (!navigator.geolocation) {
    sendWeather('Mystery', null);
    return;
  }
  navigator.geolocation.getCurrentPosition(
    function (pos) {
      var unit = cacheGet('soyface-temp-unit') === '0' ? 'fahrenheit'
        : 'celsius';
      var url =
        'https://api.open-meteo.com/v1/forecast?latitude=' +
        pos.coords.latitude.toFixed(3) +
        '&longitude=' + pos.coords.longitude.toFixed(3) +
        '&current_weather=true' +
        '&temperature_unit=' + unit;
      httpGet(
        url,
        function (j) {
          var cur = j.current_weather || {};
          sendWeather(wmoLabel(cur.weathercode), cur.temperature);
        },
        function () { sendWeather('Mystery', null); }
      );
    },
    function () {
      console.log('geolocation denied');
      sendWeather('Mystery', null);
    },
    { timeout: 15000, maximumAge: 600000 }
  );
}

Pebble.addEventListener('ready', function () {
  var last = cacheGet('soyface-weather');
  if (last) sendWeather(last, parseFloat(cacheGet('soyface-temp')));
  fetchWeather();
  setInterval(fetchWeather, WEATHER_REFRESH_MS);
});

Pebble.addEventListener('showConfiguration', function () {
  var seed = {};
  var brand = cacheGet('soyface-brand');
  var info = cacheGet('soyface-show-info');
  var big = cacheGet('soyface-big-time');
  var t24 = cacheGet('soyface-time-24h');
  var anim = cacheGet('soyface-animations');
  var puns = cacheGet('soyface-fruit-puns');
  var tunit = cacheGet('soyface-temp-unit');
  var buzz = cacheGet('soyface-buzz');
  if (brand !== null) seed.brand = brand;
  if (info !== null) seed.showInfo = info === '1';
  if (big !== null) seed.bigTime = big === '1';
  if (t24 !== null) seed.time24 = t24 === '1';
  if (anim !== null) seed.animations = anim !== '0';
  if (puns !== null) seed.fruitPuns = puns !== '0';
  if (tunit !== null) seed.tempC = tunit === '1';
  if (buzz !== null) seed.buzz = buzz === '1';
  var url = CONFIG_URL +
    '?settings=' + encodeURIComponent(JSON.stringify(seed)) +
    '&returnTo=' + encodeURIComponent('pebblejs://close#');
  try {
    Pebble.openURL(url);
  } catch (e) {
    console.log('openURL failed: ' + (e && e.message ? e.message : e));
  }
});
// wait what does this do, i dont know man.
Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  var raw = e.response.match(/^\{/) ? e.response
    : decodeURIComponent(e.response);
  var opts;
  try {
    opts = JSON.parse(raw);
  } catch (err) {
    console.log('config parse failed');
    return;
  }
  var msg = {};
  if (typeof opts.brandText === 'string' && opts.brandText.length) {
    msg[KEY_BRAND_TEXT] = opts.brandText;
    cacheSet('soyface-brand', opts.brandText);
  }
  if (typeof opts.bigTime === 'boolean') {
    msg[KEY_BIG_TIME] = opts.bigTime ? 1 : 0;
    cacheSet('soyface-big-time', opts.bigTime ? '1' : '0');
  }
  if (typeof opts.showInfo === 'boolean') {
    msg[KEY_SHOW_INFO] = opts.showInfo ? 1 : 0;
    cacheSet('soyface-show-info', opts.showInfo ? '1' : '0');
  }
  if (typeof opts.time24 === 'boolean') {
    msg[KEY_TIME_24H] = opts.time24 ? 1 : 0;
    cacheSet('soyface-time-24h', opts.time24 ? '1' : '0');
  }
  if (typeof opts.animations === 'boolean') {
    msg[KEY_ANIMATIONS] = opts.animations ? 1 : 0;
    cacheSet('soyface-animations', opts.animations ? '1' : '0');
  }
  if (typeof opts.fruitPuns === 'boolean') {
    msg[KEY_FRUIT_PUNS] = opts.fruitPuns ? 1 : 0;
    cacheSet('soyface-fruit-puns', opts.fruitPuns ? '1' : '0');
  }
  var refetch = false;
  if (typeof opts.tempC === 'boolean') {
    refetch = (cacheGet('soyface-temp-unit') === '0') !== opts.tempC;
    msg[KEY_TEMP_UNIT] = opts.tempC ? 1 : 0;
    cacheSet('soyface-temp-unit', opts.tempC ? '1' : '0');
  }
  if (typeof opts.buzz === 'boolean') {
    msg[KEY_BUZZ] = opts.buzz ? 1 : 0;
    cacheSet('soyface-buzz', opts.buzz ? '1' : '0');
  }
  var hasKeys = false;
  for (var k in msg) { if (msg.hasOwnProperty(k)) { hasKeys = true; break; } }
  if (!hasKeys) return;
  Pebble.sendAppMessage(
    msg,
    function () {
      console.log('settings pushed');
      if (refetch) { fetchWeather(); }
    },
    function (ev) { console.log('settings push failed: ' + ev.error.message); }
  );
});
