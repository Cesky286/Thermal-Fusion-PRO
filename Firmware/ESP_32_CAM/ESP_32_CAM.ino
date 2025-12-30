#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include "esp_http_server.h"

const char* ssid = "PRO-THERMAL-CAM";
const char* password = "masterofespressif";
#define FLASH_GPIO_NUM 4

// AI Thinker Pinout
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

WiFiUDP udp;
SemaphoreHandle_t dataMutex;
unsigned long lastDataTime = 0;

struct __attribute__((packed)) ThermalPacket {
  float minTemp;
  float maxTemp;
  uint32_t frameId;
  uint8_t battery;
  int8_t rssi;
  uint8_t pixels[768]; 
} thermalData;

const char index_html[] = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no, viewport-fit=cover">
  <title>ESP32 THERMAL PRO</title>
  <style>
    :root { --accent: #007aff; --panel: #1a1a1a; }
    body { background: #000; color: #fff; margin: 0; font-family: sans-serif; display: flex; flex-direction: column; align-items: center; min-height: 100vh; overflow-x: hidden; }
    #container { display: flex; width: 100vw; max-width: 690px; background: #000; border-bottom: 2px solid var(--accent); flex-direction: row; align-items: stretch; }
    #view-port { position: relative; width: calc(100% - 50px); aspect-ratio: 4/3; overflow: hidden; touch-action: none; background: #111; }
    #videoImg { position: absolute; top:0; left:0; width: 100%; height: 100%; object-fit: fill; }
    #dstCanvas { position: absolute; top:0; left:0; width: 100%; height: 100%; opacity: 0.6; pointer-events: none; }
    #scale-container { width: 50px; background: #000; display: flex; flex-direction: column; align-items: center; padding: 10px 0; border-left: 1px solid #333; box-sizing: border-box; }
    .scale-label { font-size: 11px; font-weight: bold; width: 100%; text-align: center; color: #fff; height: 20px; }
    #scaleBar { flex: 1; width: 15px; border: 1px solid #444; border-radius: 2px; margin: 5px 0; }
    #status-bar { position: absolute; bottom: 0; left: 0; right: 0; background: rgba(0,0,0,0.85); display: flex; justify-content: space-around; padding: 6px; font-size: 11px; color: #00ffcc; border-top: 1px solid #222; z-index: 10; }
    #tapMarker { position: absolute; width: 26px; height: 26px; transform: translate(-50%, -50%); border: 2px solid #ffcc00; border-radius: 50%; display: none; pointer-events: none; }
    #tapVal { position: absolute; background: #ffcc00; color: #000; padding: 3px 8px; border-radius: 6px; font-weight: bold; font-size: 14px; transform: translate(15px, -35px); display: none; }
    #controls { width: 100%; max-width: 690px; background: var(--panel); padding: 12px; display: flex; flex-direction: column; gap: 8px; box-sizing: border-box; overflow-y: auto; }
    .row { display: flex; gap: 10px; align-items: center; justify-content: space-between; }
    button { background: #333; color: #fff; border: 1px solid #444; padding: 12px; border-radius: 10px; flex: 1; cursor: pointer; font-weight: bold; }
    button.active { background: var(--accent); border-color: #fff; }
    .label { font-size: 9px; color: #888; width: 60px; text-transform: uppercase; }
    input[type=range] { flex: 1; accent-color: var(--accent); }
    input[type=number] { background: #222; color: #fff; border: 1px solid #444; padding: 8px; border-radius: 6px; width: 45px; text-align: center; }
  </style>
</head><body>
  <div id="container">
    <div id="view-port" onpointerdown="handleTap(event)">
      <img id="videoImg" crossorigin="anonymous">
      <canvas id="dstCanvas" width="640" height="480"></canvas>
      <div id="status-bar">
        <div>SIG: <span id="rssiVal">--</span>dB</div>
        <div>BAT: <span id="battVal">--</span>%</div>
        <div>FPS: <span id="fpsVal">0</span></div>
      </div>
      <div id="tapMarker"></div><div id="tapVal"></div>
    </div>
    <div id="scale-container">
      <div id="scale-max" class="scale-label">--</div>
      <canvas id="scaleBar"></canvas>
      <div id="scale-min" class="scale-label">--</div>
    </div>
  </div>
  <div id="controls">
    <div class="row"><button onclick="toggleFreeze()" id="btnFreeze">FREEZE</button><button onclick="takeSnapshot()" id="btnSave">SAVE JPG</button></div>
    <div class="row"><button onclick="setPalette('iron')" id="btnIron" class="active">IRON</button><button onclick="setPalette('rain')" id="btnRain">RAINBOW</button><button onclick="state.iso=!state.iso; this.classList.toggle('active')" id="btnIso">ISO</button></div>
    <div class="row"><span class="label">FUSION</span><input type="range" min="0" max="100" value="60" oninput="state.opacity=this.value/100; document.getElementById('dstCanvas').style.opacity=state.opacity"></div>
    <div class="row"><span class="label">PARALLAX</span><input type="range" min="-50" max="50" value="0" oninput="state.offX=parseInt(this.value)"> <input type="range" min="-50" max="50" value="0" oninput="state.offY=parseInt(this.value)"></div>
    <div class="row"><span class="label">EMISSIVITY</span><input type="range" min="0.1" max="1.0" step="0.05" value="0.95" oninput="state.emi=parseFloat(this.value); document.getElementById('emiShow').innerText=state.emi"> <div id="emiShow" style="width:30px; font-size:11px">0.95</div></div>
    <div class="row"><span class="label">SCALE</span><button onclick="setScaleMode(true)" id="btnAuto" class="active">AUTO</button><button onclick="setScaleMode(false)" id="btnManual">MANUAL</button>
      <div id="manInputs" style="display:none; gap:5px"><input type="number" id="minIn" value="20" onchange="updateMan()"> <input type="number" id="maxIn" value="40" onchange="updateMan()"></div>
    </div>
  </div>
  <canvas id="srcCanvas" width="32" height="24" style="display:none"></canvas>
<script>
  let state = { freeze:false, isSaving:false, opacity:0.6, palette:'iron', pixels:null, offX:0, offY:0, iso:false, emi:0.95, autoScale:true, manMin:20, manMax:40, fMin:20, fMax:40, selIdx:null };
  let frameCount=0, lastFpsTime=Date.now(), lastFrameId=-1;
  const video=document.getElementById('videoImg'), dstCtx=document.getElementById('dstCanvas').getContext('2d'), srcCtx=document.getElementById('srcCanvas').getContext('2d'), scaleCtx=document.getElementById('scaleBar').getContext('2d');

  function refreshVideo() { if(!state.freeze && !state.isSaving) video.src = "/capture?t=" + Date.now(); }
  video.onload = () => { if(!state.freeze && !state.isSaving) setTimeout(refreshVideo, 15); };

  function handleTap(e) {
    const rect=document.getElementById('view-port').getBoundingClientRect();
    const xR=(e.clientX-rect.left)/rect.width, yR=(e.clientY-rect.top)/rect.height;
    state.selIdx=(23-Math.floor(yR*24))*32 + Math.floor(xR*32);
    const tm=document.getElementById('tapMarker'), tv=document.getElementById('tapVal');
    tm.style.left=tv.style.left=(xR*100)+"%"; tm.style.top=tv.style.top=(yR*100)+"%";
    tm.style.display=tv.style.display='block';
  }

  async function fetchThermal() {
    if(state.freeze || state.isSaving) { setTimeout(fetchThermal, 100); return; }
    try {
      let r=await fetch('/thermal'), buf=await r.arrayBuffer(), dv=new DataView(buf);
      let fId=dv.getUint32(8, true);
      if(fId !== lastFrameId) {
        state.fMin=dv.getFloat32(0, true)/state.emi; state.fMax=dv.getFloat32(4, true)/state.emi;
        document.getElementById('battVal').innerText=dv.getUint8(12);
        document.getElementById('rssiVal').innerText=dv.getInt8(13);
        state.pixels=new Uint8Array(buf, 14);
        renderThermal(); drawScale(scaleCtx);
        lastFrameId=fId; frameCount++;
      }
      if(state.selIdx!==null) {
        let t=state.fMin+(state.pixels[state.selIdx]/255)*(state.fMax-state.fMin);
        document.getElementById('tapVal').innerText = t.toFixed(1) + "\u00B0C";
      }
      if(Date.now()-lastFpsTime>1000) { document.getElementById('fpsVal').innerText = frameCount; frameCount=0; lastFpsTime=Date.now(); }
      setTimeout(fetchThermal, 30);
    } catch(e) { setTimeout(fetchThermal, 500); }
  }

  function getPaletteColor(pct, pName) {
    const p={ iron:[[0,0,10],[0,0,120],[130,0,130],[255,100,0],[255,255,255]], rain:[[0,0,255],[0,255,255],[0,255,0],[255,255,0],[255,0,0]] };
    let arr=p[pName], idx=pct*(arr.length-1), iL=Math.floor(idx), f=idx-iL;
    let c1=arr[iL], c2=arr[Math.min(iL+1, arr.length-1)];
    return `rgb(${Math.floor(c1[0]+f*(c2[0]-c1[0]))},${Math.floor(c1[1]+f*(c2[1]-c1[1]))},${Math.floor(c1[2]+f*(c2[2]-c1[2]))})`;
  }

  function renderThermal() {
    srcCtx.clearRect(0,0,32,24);
    let low=state.autoScale?state.fMin:state.manMin, high=state.autoScale?state.fMax:state.manMax;
    document.getElementById('scale-max').innerText=high.toFixed(0);
    document.getElementById('scale-min').innerText=low.toFixed(0);
    for(let i=0; i<768; i++) {
      let t=state.fMin+(state.pixels[i]/255)*(state.fMax-state.fMin);
      let pct=Math.max(0, Math.min(1, (t-low)/(high-low)));
      if(state.iso && pct<0.75) srcCtx.fillStyle="rgba(0,0,0,0)";
      else srcCtx.fillStyle=getPaletteColor(pct, state.palette);
      srcCtx.fillRect(i%32, 23-Math.floor(i/32), 1, 1);
    }
    dstCtx.clearRect(0,0,640,480);
    dstCtx.drawImage(document.getElementById('srcCanvas'), state.offX, state.offY, 640, 480);
  }

  function drawScale(ctx) {
    const w = ctx.canvas.clientWidth, h = ctx.canvas.clientHeight;
    if(ctx.canvas.width != w || ctx.canvas.height != h) { ctx.canvas.width=w; ctx.canvas.height=h; }
    for(let y=0; y<h; y++) { ctx.fillStyle=getPaletteColor(1-(y/h), state.palette); ctx.fillRect(0, y, w, 1); }
  }

  function setScaleMode(a) { state.autoScale=a; document.getElementById('btnAuto').classList.toggle('active', a); document.getElementById('btnManual').classList.toggle('active', !a); document.getElementById('manInputs').style.display=a?"none":"flex"; }
  function updateMan() { state.manMin=parseFloat(document.getElementById('minIn').value); state.manMax=parseFloat(document.getElementById('maxIn').value); }
  function setPalette(p) { state.palette=p; document.getElementById('btnIron').classList.toggle('active', p=='iron'); document.getElementById('btnRain').classList.toggle('active', p=='rain'); }
  function toggleFreeze() { state.freeze=!state.freeze; document.getElementById('btnFreeze').classList.toggle('active', state.freeze); if(!state.freeze){refreshVideo(); fetchThermal();} }

  function takeSnapshot() {
    state.isSaving=true; document.getElementById('btnSave').innerText="LOCKING...";
    setTimeout(() => {
      const c=document.createElement('canvas'); c.width=710; c.height=480;
      const ctx=c.getContext('2d');
      ctx.fillStyle="black"; ctx.fillRect(0,0,710,480);
      ctx.drawImage(video, 0,0, 640,480);
      ctx.globalAlpha=state.opacity; ctx.drawImage(document.getElementById('dstCanvas'), 0,0, 640,480);
      ctx.globalAlpha=1.0; 
      const snapX=660; 
      for(let y=0; y<400; y++) { ctx.fillStyle=getPaletteColor(1-(y/400), state.palette); ctx.fillRect(snapX, y+40, 15, 1); }
      ctx.fillStyle="white"; ctx.font="bold 14px Arial";
      ctx.fillText(document.getElementById('scale-max').innerText+"°C", snapX-5, 30);
      ctx.fillText(document.getElementById('scale-min').innerText+"°C", snapX-5, 465);
      ctx.font="11px monospace"; ctx.fillText("ESP32 THERMAL | EMI: "+state.emi, 10, 20);
      if(state.selIdx) ctx.fillText("POINT: " + document.getElementById('tapVal').innerText, 10, 35);
      const link=document.createElement('a'); link.download='ESP32_'+Date.now()+'.jpg'; link.href=c.toDataURL('image/jpeg', 0.95); link.click();
      setTimeout(() => { state.isSaving=false; document.getElementById('btnSave').innerText="SAVE JPG"; refreshVideo(); fetchThermal(); }, 1000);
    }, 200);
  }
  window.onload=()=>{refreshVideo(); fetchThermal();};
</script></body></html>
)rawliteral";

void udpTask(void *pvParameters) {
  uint8_t buffer[sizeof(ThermalPacket) + 32];
  while(true) {
    int pSize = udp.parsePacket();
    if (pSize >= sizeof(ThermalPacket)) {
      udp.read(buffer, sizeof(buffer));
      if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10))) {
        memcpy(&thermalData, buffer, sizeof(ThermalPacket));
        lastDataTime = millis();
        xSemaphoreGive(dataMutex);
      }
    }
    vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(115200);
  dataMutex = xSemaphoreCreateMutex();
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0; config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM; config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM; config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; config.jpeg_quality = 12; config.fb_count = 2;
  esp_camera_init(&config);
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1); s->set_hmirror(s, 0);
  WiFi.softAP(ssid, password);
  MDNS.begin("thermalcam");
  udp.begin(4444);
  xTaskCreatePinnedToCore(udpTask, "UDP", 4096, NULL, 1, NULL, 0);
  httpd_config_t h_config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;
  if (httpd_start(&server, &h_config) == ESP_OK) {
    httpd_uri_t uri_idx = {"/", HTTP_GET, [](httpd_req_t *r){return httpd_resp_send(r, index_html, HTTPD_RESP_USE_STRLEN);}, NULL};
    httpd_uri_t uri_cap = {"/capture", HTTP_GET, [](httpd_req_t *r){
      camera_fb_t * fb = esp_camera_fb_get();
      httpd_resp_set_type(r, "image/jpeg");
      httpd_resp_send(r, (const char *)fb->buf, fb->len);
      esp_camera_fb_return(fb); return ESP_OK;
    }, NULL};
    httpd_uri_t uri_thr = {"/thermal", HTTP_GET, [](httpd_req_t *r){
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      httpd_resp_set_type(r, "application/octet-stream");
      httpd_resp_send(r, (const char*)&thermalData, sizeof(thermalData));
      xSemaphoreGive(dataMutex); return ESP_OK;
    }, NULL};
    httpd_register_uri_handler(server, &uri_idx);
    httpd_register_uri_handler(server, &uri_cap);
    httpd_register_uri_handler(server, &uri_thr);
  }
}
void loop() { delay(1000); }