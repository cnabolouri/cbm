#include "web_ui.h"
#include "../config.h"
#include <math.h>

WebUI::WebUI(SDManager& sdRef, Recorder& recorderRef, LiveData& liveRef)
  : sd(sdRef), recorder(recorderRef), live(liveRef), server(80) {}

static String badgeClass(const String& alert) {
  if (alert == "High") return "background:#b42318;color:#fff;padding:4px 8px;border-radius:999px;";
  if (alert == "Watch") return "background:#9a6700;color:#fff;padding:4px 8px;border-radius:999px;";
  return "background:#238636;color:#fff;padding:4px 8px;border-radius:999px;";
}

static String htmlEscapeLocal(const String& s) {
  String out;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

void WebUI::begin() {
  initWiFiAP();
  initRoutes();
  server.begin();
  Serial.println("Web server started");
}

void WebUI::handleClient() {
  server.handleClient();
}

void WebUI::setRecordingCallbacks(std::function<bool()> startCb, std::function<void()> stopCb) {
  startRecordingCb = startCb;
  stopRecordingCb = stopCb;
}

void WebUI::setStatusMessageSource(String* statusMsg) {
  statusMessagePtr = statusMsg;
}

void WebUI::setVibrationFFTaxisRef(VibrationAxis* axisPtr) {
  vibrationFFTaxisPtr = axisPtr;
}

void WebUI::setManualOverrideRef(bool* overridePtrIn) {
  manualOverridePtr = overridePtrIn;
}

void WebUI::setThermalRangeRefs(ThermalRangeMode* modePtr, float* minPtr, float* maxPtr) {
  thermalRangeModePtr = modePtr;
  thermalFixedMinPtr = minPtr;
  thermalFixedMaxPtr = maxPtr;
}

void WebUI::setThermalZoomRef(float* zoomPtr) {
  thermalZoomPtr = zoomPtr;
}

void WebUI::setThermalCenterRefs(float* cxPtr, float* cyPtr) {
  thermalCenterXPtr = cxPtr;
  thermalCenterYPtr = cyPtr;
}

void WebUI::setThermalPaletteRef(ThermalPalette* palettePtr) {
  thermalPalettePtr = palettePtr;
}

void WebUI::setThermalHotspotRefs(ThermalHotspotMode* modePtr, int* xPtr, int* yPtr) {
  thermalHotspotModePtr = modePtr;
  thermalHotspotXPtr = xPtr;
  thermalHotspotYPtr = yPtr;
}

void WebUI::setThermalThresholdRefs(ThermalThresholdMode* modePtr, float* thresholdPtr) {
  thermalThresholdModePtr = modePtr;
  thermalThresholdFPtr = thresholdPtr;
}

void WebUI::setThermalPointerCallback(std::function<void(int,int)> cb) {
  thermalPointerSetter = cb;
}

void WebUI::initWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);
}

String WebUI::htmlHeader(const String& title) {
  String s;
  s += "<!DOCTYPE html><html><head><meta charset='utf-8'>";
  s += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  s += "<title>" + title + "</title>";
  s += "<style>";
  s += "body{font-family:Arial;background:#111;color:#eee;margin:0;padding:16px;}";
  s += "h1,h2{margin:8px 0 12px 0;}";
  s += ".nav{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:16px;}";
  s += ".nav a,.btn{background:#1f6feb;color:#fff;padding:10px 14px;border-radius:8px;text-decoration:none;border:none;display:inline-block;}";
  s += ".card{background:#1b1b1b;border:1px solid #333;border-radius:12px;padding:14px;margin-bottom:14px;}";
  s += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:10px;}";
  s += ".metric{background:#161616;border:1px solid #333;border-radius:10px;padding:12px;}";
  s += "table{width:100%;border-collapse:collapse;}";
  s += "th,td{border-bottom:1px solid #333;padding:8px;text-align:left;}";
  s += "canvas{width:100%;height:auto;background:#0d0d0d;border:1px solid #333;border-radius:10px;display:block;}";
  s += ".legend{display:flex;gap:14px;flex-wrap:wrap;font-size:14px;margin:8px 0 0 0;}";
  s += ".dot{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:6px;}";
  s += ".muted{color:#aaa;font-size:14px;}";
  s += "</style></head><body>";
  s += "<div class='nav'>";
  s += "<a href='/overview'>Overview</a>";
  s += "<a href='/live/vibration'>Vibration</a>";
  s += "<a href='/live/vibration_fft'>Vibration FFT</a>";
  s += "<a href='/live/temperature'>Temperature</a>";
  s += "<a href='/live/thermal'>Thermal</a>";
  s += "<a class='btn' href='/live/sound'>Sound</a>";
  s += "<a href='/live/sound_fft'>Sound FFT</a>";
  s += "<a href='/files'>Sessions</a>";
  s += "</div>";
  return s;
}

String WebUI::htmlFooter() {
  return "</body></html>";
}

void WebUI::initRoutes() {
  server.on("/", HTTP_GET, [this]() { handleRoot(); });
  server.on("/overview", HTTP_GET, [this]() { handleOverviewPage(); });
  server.on("/live", HTTP_GET, [this]() { handleLivePage(); });
  server.on("/live/vibration", HTTP_GET, [this]() { handleLiveVibrationPage(); });
  server.on("/live/vibration_fft", HTTP_GET, [this]() { handleLiveVibrationFFTPage(); });
  server.on("/live/temperature", HTTP_GET, [this]() { handleLiveTemperaturePage(); });
  server.on("/live/thermal", HTTP_GET, [this]() { handleLiveThermalPage(); });
  server.on("/live/sound", HTTP_GET, [this]() { handleSoundPage(); });
  server.on("/live/sound_fft", HTTP_GET, [this]() { handleLiveSoundFFTPage(); });
  server.on("/sessions", HTTP_GET, [this]() { handleSessions(); });
  server.on("/files", HTTP_GET, [this]() { handleFiles(); });
  server.on("/download", HTTP_GET, [this]() { handleDownload(); });
  server.on("/delete", HTTP_GET, [this]() { handleDelete(); });
  server.on("/delete_session", HTTP_GET, [this]() { handleDeleteSession(); });
  server.on("/edit_session", HTTP_GET, [this]() { handleEditSessionPage(); });
  server.on("/save_session_meta", HTTP_POST, [this]() { handleSaveSessionMeta(); });
  server.on("/compare", HTTP_GET, [this]() { handleComparePage(); });
  server.on("/record/start", HTTP_GET, [this]() { handleRecordStart(); });
  server.on("/record/stop", HTTP_GET, [this]() { handleRecordStop(); });
  server.on("/set_vibration_fft_axis", HTTP_GET, [this]() { handleSetVibrationFFTaxis(); });
  server.on("/set_manual_override", HTTP_GET, [this]() { handleSetManualOverride(); });
  server.on("/set_thermal_range_mode", HTTP_GET, [this]() { handleSetThermalRangeMode(); });
  server.on("/set_thermal_zoom", HTTP_GET, [this]() { handleSetThermalZoom(); });
  server.on("/set_thermal_center", HTTP_GET, [this]() { handleSetThermalCenter(); });
  server.on("/set_thermal_palette", HTTP_GET, [this]() { handleSetThermalPalette(); });
  server.on("/set_thermal_hotspot_mode", HTTP_GET, [this]() { handleSetThermalHotspotMode(); });
  server.on("/set_thermal_hotspot_point", HTTP_GET, [this]() { handleSetThermalHotspotPoint(); });
  server.on("/set_thermal_threshold", HTTP_GET, [this]() { handleSetThermalThreshold(); });
  server.on("/save_thermal_snapshot", HTTP_GET, [this]() { handleSaveThermalSnapshot(); });
  server.on("/set_thermal_pointer", HTTP_GET, [this]() { handleSetThermalPointer(); });
  server.on("/api/status", HTTP_GET, [this]() { handleStatusJson(); });
  server.on("/api/live", HTTP_GET, [this]() { handleLiveJson(); });
  server.on("/api/vibration", HTTP_GET, [this]() { handleVibrationJson(); });
  server.on("/api/sound", HTTP_GET, [this]() { handleSoundJson(); });
  server.on("/api/analysis", HTTP_GET, [this]() { handleAnalysisJson(); });
  server.on("/api/thermal_frame", HTTP_GET, [this]() { handleThermalFrameJson(); });
  server.on("/api/sound_fft", HTTP_GET, [this]() { handleSoundSpectrumJson(); });
  server.on("/api/vibration_fft", HTTP_GET, [this]() { handleVibrationSpectrumJson(); });
  server.on("/view", HTTP_GET, [this]() { handleViewPage(); });
  server.on("/api/file", HTTP_GET, [this]() { handleFileRaw(); });
}

void WebUI::handleRoot() {
  handleOverviewPage();
}

void WebUI::handleOverviewPage() {
  String s = htmlHeader("Overview");
  s += "<h1>Overview</h1>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>System</b><br>SD: <span id='ov_sd'>-</span><br>Recording: <span id='ov_rec'>-</span></div>";

  s += "<div class='metric'><b>Mount</b><br>";
  s += "State: <span id='ov_mount_state'>-</span><br>";
  s += "Confidence: <span id='ov_mount_conf'>-</span><br>";
  s += "Trusted: <span id='ov_mount_trust'>-</span></div>";

  s += "<div class='metric'><b>Recording State</b><br>";
  s += "Mode: <span id='ov_rec_mode'>-</span><br>";
  s += "Override: <span id='ov_override'>-</span><br>";
  s += "Status: <span id='ov_status'>-</span></div>";

  s += "<div class='metric'><b>Vibration</b><br>";
  s += "Current: <span id='ov_vcur'>-</span><br>";
  s += "Max: <span id='ov_vmax'>-</span><br>";
  s += "Axis: <span id='ov_vaxis'>-</span><br>";
  s += "Alert: <span id='ov_valert'>-</span></div>";

  s += "<div class='metric'><b>Temperature</b><br>";
  s += "Delta: <span id='ov_tcur'>-</span><br>";
  s += "Max: <span id='ov_tmax'>-</span><br>";
  s += "Rise: <span id='ov_trise'>-</span><br>";
  s += "Alert: <span id='ov_talert'>-</span></div>";

  s += "<div class='metric'><b>Sound</b><br>";
  s += "dB: <span id='ov_scur'>-</span><br>";
  s += "Max: <span id='ov_smax'>-</span><br>";
  s += "Hz: <span id='ov_shz'>-</span><br>";
  s += "Alert: <span id='ov_salert'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Quick Health View</h2>";
  s += "<div class='grid'>";
  s += "<div class='metric'><b>Overall</b><br><span id='ov_overall'>-</span></div>";
  s += "<div class='metric'><b>Interpretation</b><br><span id='ov_text'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Mini Trends</h2>";
  s += "<div style='display:grid;grid-template-columns:1fr;gap:16px;'>";
  s += "<div><div class='muted'>Total Vibration</div><canvas id='ov_vib' width='900' height='120'></canvas></div>";
  s += "<div><div class='muted'>Delta Temperature</div><canvas id='ov_temp' width='900' height='120'></canvas></div>";
  s += "<div><div class='muted'>Sound dB</div><canvas id='ov_sound' width='900' height='120'></canvas></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Quick Actions</h2>";
  s += "<a class='btn' href='/record/start'>Start / Arm Recording</a> ";
  s += "<a class='btn' href='/record/stop'>Stop Recording</a> ";
  s += "<a class='btn' href='/live/vibration'>Vibration</a> ";
  s += "<a class='btn' href='/live/temperature'>Temperature</a> ";
  s += "<a class='btn' href='/live/sound'>Sound</a>";
  s += "</div>";

  s += "<div class='card'><h2>Mount / Recording Control</h2>";
  s += "<a class='btn' href='/record/start'>Start / Arm Recording</a> ";
  s += "<a class='btn' href='/record/stop'>Stop</a> ";
  s += "<a class='btn' href='/set_manual_override?enabled=1'>Override ON</a> ";
  s += "<a class='btn' href='/set_manual_override?enabled=0'>Override OFF</a>";
  s += "</div>";

  if (statusMessagePtr && statusMessagePtr->length()) {
    s += "<div class='card'><b>Status</b><br>" + *statusMessagePtr + "</div>";
  }

  s += "<div class='card'><h2>Current File</h2>";
  s += (live.system.currentBaseName.length() ? live.system.currentBaseName : String("None"));
  s += "</div>";

  s += "<script>";
  s += "const MAX_POINTS=100;const ovV=[],ovT=[],ovS=[];";
  s += "function push(a,v){a.push(v);if(a.length>MAX_POINTS)a.shift();}";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "function rangeOf(series,opts={}){let all=[];series.forEach(a=>all=all.concat(a.filter(v=>Number.isFinite(v))));if(!all.length)return{min:0,max:1};let min=Math.min(...all),max=Math.max(...all);if(opts.absoluteFloor!==undefined)max=Math.max(max,opts.absoluteFloor);if(opts.zeroMin)min=0;if(max===min)max=min+1;const span=max-min,pad=span*(opts.paddingFrac||0.12);min-=opts.zeroMin?0:pad;max+=pad;if(opts.zeroMin&&min<0)min=0;return{min,max};}";
  s += "function drawSpark(canvasId,arr,color,opts={}){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#2d2d2d';for(let i=1;i<3;i++){let y=h*i/3;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}const rg=rangeOf([arr],opts);function py(v){return h-((v-rg.min)/(rg.max-rg.min))*h;}if(arr.length>1){ctx.strokeStyle=color;ctx.lineWidth=2;ctx.beginPath();for(let i=0;i<arr.length;i++){let x=(i/(arr.length-1))*w;let y=py(arr[i]);if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();}ctx.fillStyle='#aaa';ctx.font='11px Arial';ctx.fillText('min:'+rg.min.toFixed(3),8,h-6);ctx.fillText('max:'+rg.max.toFixed(3),8,12);}";
  s += "function mountStateName(v){if(v===3)return'MONITORING';if(v===2)return'STABILIZING';if(v===1)return'MOVING';return'UNKNOWN';}";
  s += "function recModeName(v){if(v===2)return'ACTIVE';if(v===1)return'ARMED';return'IDLE';}";
  s += "function overallAlert(v,t,s){if(v==='High'||t==='High'||s==='High')return'High';if(v==='Watch'||t==='Watch'||s==='Watch')return'Watch';return'Normal';}";
  s += "function interpretation(v,t,s,axis,rising){let parts=[];if(v==='High')parts.push('Mechanical vibration is elevated, strongest on '+axis+'.');else if(v==='Watch')parts.push('Vibration is above normal watch level.');else parts.push('Vibration looks stable.');if(t==='High')parts.push('Thermal delta is significantly elevated.');else if(t==='Watch')parts.push('Temperature delta is above normal watch level.');else parts.push('Temperature looks stable.');if(rising)parts.push('Temperature is rising faster than recent average.');if(s==='High')parts.push('Sound level is high and may indicate roughness or abnormal load.');else if(s==='Watch')parts.push('Sound level is above normal watch level.');else parts.push('Sound trend looks normal.');return parts.join(' ');}";
  s += "async function poll(){const r=await fetch('/api/analysis');const d=await r.json();setText('ov_sd',d.sd?'OK':'FAIL');setText('ov_rec',d.recording?'ON':'OFF');setText('ov_mount_state',mountStateName(d.mount.state));setText('ov_mount_conf',d.mount.confidence.toFixed(1)+'%');setText('ov_mount_trust',d.mount.trusted?'Yes':'No');setText('ov_rec_mode',recModeName(d.recordingMode));setText('ov_override',d.manualOverride?'ON':'OFF');setText('ov_status',d.statusText||'-');setText('ov_vcur',d.vtot.toFixed(5)+' in/s');setText('ov_vmax',d.vibration.maxTotal.toFixed(5));setText('ov_vaxis',d.vibration.dominantAxis);setText('ov_valert',d.vibration.alert);setText('ov_tcur',d.dTF.toFixed(2)+' F');setText('ov_tmax',d.temperature.maxDelta.toFixed(2));setText('ov_trise',d.temperature.risingFast?'Yes':'No');setText('ov_talert',d.temperature.alert);setText('ov_scur',d.db.toFixed(2));setText('ov_smax',d.sound.maxDb.toFixed(2));setText('ov_shz',d.hz.toFixed(1));setText('ov_salert',d.sound.alert);const overall=overallAlert(d.vibration.alert,d.temperature.alert,d.sound.alert);setText('ov_overall',d.mount.trusted||d.manualOverride?overall:'Waiting');if(!d.mount.trusted&&!d.manualOverride)setText('ov_text','Device is not yet stably mounted. Diagnostic interpretation is paused until stable contact is confirmed.');else setText('ov_text',interpretation(d.vibration.alert,d.temperature.alert,d.sound.alert,d.vibration.dominantAxis,d.temperature.risingFast));push(ovV,d.vtot);push(ovT,d.dTF);push(ovS,d.db);drawSpark('ov_vib',ovV,'#ffd54a',{zeroMin:true,absoluteFloor:0.002,paddingFrac:0.15});drawSpark('ov_temp',ovT,'#ff4d9d',{zeroMin:true,absoluteFloor:1,paddingFrac:0.15});drawSpark('ov_sound',ovS,'#3ddc84',{zeroMin:true,absoluteFloor:10,paddingFrac:0.15});}";
  s += "setInterval(poll,500);poll();";
  s += "</script>";

  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleLivePage() {
  String s = htmlHeader("Live Charts");

  s += "<h1>Live Charts</h1>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>SD</b><br><span id='sdStatus'>-</span></div>";
  s += "<div class='metric'><b>Recording</b><br><span id='recStatus'>-</span></div>";
  s += "<div class='metric'><b>Total Vib</b><br><span id='vtot'>-</span></div>";
  s += "<div class='metric'><b>Object Temp</b><br><span id='objF'>-</span></div>";
  s += "<div class='metric'><b>Sound dB</b><br><span id='db'>-</span></div>";
  s += "<div class='metric'><b>Frequency</b><br><span id='hz'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Vibration</h2>";
  s += "<canvas id='vibChart' width='900' height='260'></canvas>";
  s += "<div class='legend'>";
  s += "<span><span class='dot' style='background:#ff4d4d'></span>X</span>";
  s += "<span><span class='dot' style='background:#3ddc84'></span>Y</span>";
  s += "<span><span class='dot' style='background:#4da3ff'></span>Z</span>";
  s += "<span><span class='dot' style='background:#ffd54a'></span>TOT</span>";
  s += "</div></div>";

  s += "<div class='card'><h2>Temperature</h2>";
  s += "<canvas id='tempChart' width='900' height='260'></canvas>";
  s += "<div class='legend'>";
  s += "<span><span class='dot' style='background:#ffd54a'></span>Object</span>";
  s += "<span><span class='dot' style='background:#4da3ff'></span>Reference</span>";
  s += "<span><span class='dot' style='background:#ff8c42'></span>Ambient</span>";
  s += "<span><span class='dot' style='background:#ff4d9d'></span>Delta</span>";
  s += "</div></div>";

  s += "<div class='card'><h2>Sound</h2>";
  s += "<canvas id='soundChart' width='900' height='260'></canvas>";
  s += "<div class='legend'>";
  s += "<span><span class='dot' style='background:#3ddc84'></span>dB</span>";
  s += "<span><span class='dot' style='background:#ffd54a'></span>Hz/100</span>";
  s += "</div></div>";

  s += "<script>";
  s += "const MAX_POINTS=120;";
  s += "const vib={x:[],y:[],z:[],t:[]};";
  s += "const temp={obj:[],ref:[],amb:[],dt:[]};";
  s += "const sound={db:[],hzScaled:[]};";
  s += "function push(arr,val){arr.push(val); if(arr.length>MAX_POINTS) arr.shift();}";
  s += "function setText(id,val){document.getElementById(id).textContent=val;}";
  s += "function drawChart(canvasId, series, colors, options={}){";
  s += "const c=document.getElementById(canvasId),ctx=c.getContext('2d');";
  s += "const w=c.width,h=c.height;";
  s += "ctx.clearRect(0,0,w,h); ctx.fillStyle='#0d0d0d'; ctx.fillRect(0,0,w,h);";
  s += "ctx.strokeStyle='#2d2d2d'; ctx.lineWidth=1;";
  s += "for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}";
  s += "for(let i=1;i<6;i++){let x=w*i/6;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}";
  s += "let all=[]; series.forEach(a=>all=all.concat(a));";
  s += "let max=Math.max(...all, options.minMax||0.001), min=0;";
  s += "if(options.allowNegative){min=Math.min(...all,-0.001); max=Math.max(...all,0.001);} if(max===min)max=min+1;";
  s += "function py(v){return h-((v-min)/(max-min))*h;}";
  s += "series.forEach((arr,i)=>{ if(arr.length<2)return; ctx.strokeStyle=colors[i]; ctx.lineWidth=2; ctx.beginPath(); for(let k=0;k<arr.length;k++){ let x=(k/(MAX_POINTS-1))*w; let y=py(arr[k]); if(k===0)ctx.moveTo(x,y); else ctx.lineTo(x,y);} ctx.stroke(); });";
  s += "ctx.fillStyle='#aaa'; ctx.font='12px Arial'; ctx.fillText('min:'+min.toFixed(3),8,h-8); ctx.fillText('max:'+max.toFixed(3),8,14);";
  s += "}";
  s += "async function poll(){ try{ const r=await fetch('/api/live'); const d=await r.json();";
  s += "setText('sdStatus', d.sd ? 'OK':'FAIL');";
  s += "setText('recStatus', d.recording ? 'ON':'OFF');";
  s += "setText('vtot', d.vtot.toFixed(5)+' in/s');";
  s += "setText('objF', d.objF.toFixed(1)+' F');";
  s += "setText('db', d.db.toFixed(1));";
  s += "setText('hz', d.hz.toFixed(0));";
  s += "push(vib.x,d.vx); push(vib.y,d.vy); push(vib.z,d.vz); push(vib.t,d.vtot);";
  s += "push(temp.obj,d.objF); push(temp.ref,d.refF); push(temp.amb,d.ambF); push(temp.dt,d.dTF);";
  s += "push(sound.db,d.db); push(sound.hzScaled,d.hz/100.0);";
  s += "drawChart('vibChart',[vib.x,vib.y,vib.z,vib.t],['#ff4d4d','#3ddc84','#4da3ff','#ffd54a'],{minMax:0.001});";
  s += "drawChart('tempChart',[temp.obj,temp.ref,temp.amb,temp.dt],['#ffd54a','#4da3ff','#ff8c42','#ff4d9d'],{minMax:1});";
  s += "drawChart('soundChart',[sound.db,sound.hzScaled],['#3ddc84','#ffd54a'],{minMax:1});";
  s += "}catch(e){console.log(e);} }";
  s += "setInterval(poll,400); poll();";
  s += "</script>";

  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleLiveVibrationPage() {
  String s = htmlHeader("Live Vibration");
  s += "<h1>Live Vibration</h1>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>X</b><br><span id='vx'>-</span></div>";
  s += "<div class='metric'><b>Y</b><br><span id='vy'>-</span></div>";
  s += "<div class='metric'><b>Z</b><br><span id='vz'>-</span></div>";
  s += "<div class='metric'><b>Total</b><br><span id='vtot'>-</span></div>";
  s += "<div class='metric'><b>Max Total</b><br><span id='vmax'>-</span></div>";
  s += "<div class='metric'><b>Average</b><br><span id='vavg'>-</span></div>";
  s += "<div class='metric'><b>Dominant Axis</b><br><span id='vaxis'>-</span></div>";
  s += "<div class='metric'><b>Dominant Hz</b><br><span id='fftHz'>-</span></div>";
  s += "<div class='metric'><b>FFT Status</b><br><span id='fftStatus'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>XYZ + Total Trend</h2><canvas id='vibChart' width='900' height='260'></canvas></div>";
  s += "<div class='card'><h2>Total Trend + Moving Average</h2><canvas id='vibTotalChart' width='900' height='220'></canvas></div>";
  s += "<div class='card'><h2>Axis Magnitudes</h2><canvas id='vibBars' width='900' height='220'></canvas></div>";
  s += "<div class='card'><h2>Vibration FFT</h2><canvas id='fftCanvas' width='900' height='240'></canvas></div>";

  s += "<script>";
  s += "const MAX_POINTS=140; const vx=[],vy=[],vz=[],vt=[];";
  s += "function push(a,v){a.push(v); if(a.length>MAX_POINTS) a.shift();}";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "function movingAverage(arr,win=8){const out=[]; for(let i=0;i<arr.length;i++){let s=0,n=0; for(let j=Math.max(0,i-win+1);j<=i;j++){s+=arr[j];n++;} out.push(n?s/n:0);} return out;}";
  s += "function rangeOf(series,opts={}){let all=[]; series.forEach(a=>all=all.concat(a.filter(v=>Number.isFinite(v)))); if(!all.length)return{min:0,max:1}; let min=Math.min(...all),max=Math.max(...all); if(opts.absoluteFloor!==undefined)max=Math.max(max,opts.absoluteFloor); if(opts.zeroMin)min=0; if(opts.centerZero){const m=Math.max(Math.abs(min),Math.abs(max),opts.absoluteFloor||0.001);min=-m;max=m;} if(max===min)max=min+1; const span=max-min,pad=span*(opts.paddingFrac||0.12); min-=opts.zeroMin?0:pad; max+=pad; if(opts.zeroMin&&min<0)min=0; return{min,max};}";
  s += "function drawAdvancedChart(canvasId,series,colors,opts={}){const c=document.getElementById(canvasId),ctx=c.getContext('2d'); const w=c.width,h=c.height; ctx.clearRect(0,0,w,h); ctx.fillStyle='#0d0d0d'; ctx.fillRect(0,0,w,h); ctx.strokeStyle='#2d2d2d'; ctx.lineWidth=1; for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();} for(let i=1;i<6;i++){let x=w*i/6;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();} const rg=rangeOf(series,opts); function py(v){return h-((v-rg.min)/(rg.max-rg.min))*h;} if(rg.min<0&&rg.max>0){let zy=py(0);ctx.strokeStyle='#555';ctx.beginPath();ctx.moveTo(0,zy);ctx.lineTo(w,zy);ctx.stroke();} series.forEach((arr,i)=>{if(!arr||arr.length<2)return;ctx.strokeStyle=colors[i];ctx.lineWidth=2;ctx.beginPath();for(let k=0;k<arr.length;k++){let x=(k/(arr.length-1))*w;let y=py(arr[k]);if(k===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();}); ctx.fillStyle='#aaa';ctx.font='12px Arial';ctx.fillText('min:'+rg.min.toFixed(4),8,h-8);ctx.fillText('max:'+rg.max.toFixed(4),8,14);}";
  s += "function drawBars(canvasId,labels,values,colors,maxFloor=1){const c=document.getElementById(canvasId),ctx=c.getContext('2d'); const w=c.width,h=c.height; ctx.clearRect(0,0,w,h); ctx.fillStyle='#0d0d0d'; ctx.fillRect(0,0,w,h); const maxVal=Math.max(...values,maxFloor); const barW=Math.floor(w/values.length)-20; values.forEach((v,i)=>{const x=20+i*Math.floor(w/values.length); const bh=(v/maxVal)*(h-50); const y=h-bh-25; ctx.fillStyle=colors[i]; ctx.fillRect(x,y,barW,bh); ctx.fillStyle='#ddd'; ctx.font='12px Arial'; ctx.fillText(labels[i],x,h-8); ctx.fillText(v.toFixed(4),x,y-6);});}";
  s += "function drawFFT(spec){const c=document.getElementById('fftCanvas'),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#2d2d2d';for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}if(!spec||!spec.valid||!spec.mag||!spec.mag.length){setText('fftStatus','warming up...');setText('fftHz','-');return;}setText('fftStatus','active');setText('fftHz',spec.dominantHz.toFixed(1)+' Hz');const maxMag=Math.max(...spec.mag,0.001);const maxHz=Math.max(...(spec.hz||[]),1);for(let i=1;i<spec.mag.length;i++){const x=spec.hz&&spec.hz[i]?(spec.hz[i]/maxHz)*w:(i/spec.mag.length)*w;const bh=(spec.mag[i]/maxMag)*(h-2);ctx.strokeStyle='#ffd54a';ctx.beginPath();ctx.moveTo(x,h-1);ctx.lineTo(x,h-1-bh);ctx.stroke();}ctx.fillStyle='#aaa';ctx.font='12px Arial';ctx.fillText(maxHz.toFixed(1)+' Hz',w-80,h-8);ctx.fillText('max: '+maxMag.toFixed(4),8,14);}";
  s += "async function poll(){const r=await fetch('/api/vibration'); const d=await r.json(); const vib=d.vibration||{}; const spec=d.vibrationSpectrum||{}; setText('vx',vib.vx.toFixed(5)); setText('vy',vib.vy.toFixed(5)); setText('vz',vib.vz.toFixed(5)); setText('vtot',vib.vt.toFixed(5)); setText('vmax',(vib.maxTotal||0).toFixed(5)); setText('vavg',(vib.avgTotal||0).toFixed(5)); setText('vaxis',vib.dominantAxis||'-'); push(vx,vib.vx); push(vy,vib.vy); push(vz,vib.vz); push(vt,vib.vt); drawAdvancedChart('vibChart',[vx,vy,vz,vt],['#ff4d4d','#3ddc84','#4da3ff','#ffd54a'],{centerZero:true,absoluteFloor:0.002,paddingFrac:0.15}); drawAdvancedChart('vibTotalChart',[vt,movingAverage(vt,10)],['#ffd54a','#ffffff'],{zeroMin:true,absoluteFloor:0.002,paddingFrac:0.15}); drawBars('vibBars',['|X|','|Y|','|Z|','TOT'],[Math.abs(vib.vx),Math.abs(vib.vy),Math.abs(vib.vz),Math.abs(vib.vt)],['#ff4d4d','#3ddc84','#4da3ff','#ffd54a'],0.01); drawFFT(spec);}";
  s += "setInterval(poll,400); poll();";
  s += "</script>";
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleLiveVibrationFFTPage() {
  String s = htmlHeader("Live Vibration FFT");
  s += "<h1>Live Vibration FFT</h1>";

  s += "<div class='card'><h2>Axis Selection</h2>";
  s += "<div style='display:flex;gap:8px;flex-wrap:wrap;'>";
  s += "<a class='btn' href='/set_vibration_fft_axis?axis=X'>Axis X</a>";
  s += "<a class='btn' href='/set_vibration_fft_axis?axis=Y'>Axis Y</a>";
  s += "<a class='btn' href='/set_vibration_fft_axis?axis=Z'>Axis Z</a>";
  s += "<a class='btn' href='/set_vibration_fft_axis?axis=MAG'>Magnitude</a>";
  s += "</div></div>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>Dominant Hz</b><br><span id='vfft_dom_hz'>-</span></div>";
  s += "<div class='metric'><b>Dominant Mag</b><br><span id='vfft_dom_mag'>-</span></div>";
  s += "<div class='metric'><b>Source Axis</b><br><span id='vfft_axis'>-</span></div>";
  s += "<div class='metric'><b>Character</b><br><span id='vfft_character'>-</span></div>";
  s += "<div class='metric'><b>H2</b><br><span id='vfft_h2'>-</span></div>";
  s += "<div class='metric'><b>H3</b><br><span id='vfft_h3'>-</span></div>";
  s += "<div class='metric'><b>Low/Mid/High</b><br><span id='vfft_bands'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Top Peaks</h2><div class='grid'>";
  s += "<div class='metric'><b>Peak 1</b><br><span id='vp1'>-</span></div>";
  s += "<div class='metric'><b>Peak 2</b><br><span id='vp2'>-</span></div>";
  s += "<div class='metric'><b>Peak 3</b><br><span id='vp3'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Vibration Spectrum</h2><canvas id='vfftChart' width='900' height='280'></canvas></div>";
  s += "<div class='card'><h2>Band Balance</h2><canvas id='vfftBands' width='900' height='220'></canvas></div>";

  s += "<script>";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "function drawSpectrum(canvasId,hz,mag,peaks){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#2d2d2d';for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}if(!mag.length)return;const maxMag=Math.max(...mag,0.001);const maxHz=Math.max(...hz,1);for(let i=0;i<mag.length;i++){const x=(hz[i]/maxHz)*w;const bh=(mag[i]/maxMag)*(h-20);ctx.fillStyle='#4da3ff';ctx.fillRect(x,h-bh,Math.max(2,w/mag.length-2),bh);}ctx.strokeStyle='#ff4d4d';ctx.fillStyle='#ff4d4d';ctx.font='12px Arial';peaks.forEach((p,i)=>{if(!p||!p.hz)return;const x=(p.hz/maxHz)*w;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();ctx.fillText('P'+(i+1)+': '+p.hz.toFixed(1)+'Hz',Math.min(x+4,w-120),16+i*14);});ctx.fillStyle='#aaa';ctx.fillText('0 Hz',8,h-8);ctx.fillText(maxHz.toFixed(1)+' Hz',w-80,h-8);ctx.fillText('max mag: '+maxMag.toFixed(4),8,14);}";
  s += "function drawBars(canvasId,labels,values,colors,maxFloor=1){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);const maxVal=Math.max(...values,maxFloor);const barW=Math.floor(w/values.length)-30;values.forEach((v,i)=>{const x=25+i*Math.floor(w/values.length);const bh=(v/maxVal)*(h-50);const y=h-bh-25;ctx.fillStyle=colors[i];ctx.fillRect(x,y,barW,bh);ctx.fillStyle='#ddd';ctx.font='12px Arial';ctx.fillText(labels[i],x,h-8);ctx.fillText(v.toFixed(4),x,y-6);});}";
  s += "async function poll(){const r=await fetch('/api/vibration_fft');const d=await r.json();setText('vfft_dom_hz',d.dominantHz.toFixed(2));setText('vfft_dom_mag',d.dominantMag.toFixed(4));setText('vfft_axis',d.sourceAxis);setText('vfft_character',d.character);setText('vfft_h2',d.harmonic2?'Yes':'No');setText('vfft_h3',d.harmonic3?'Yes':'No');setText('vfft_bands',d.lowBand.toFixed(3)+' / '+d.midBand.toFixed(3)+' / '+d.highBand.toFixed(3));setText('vp1',d.peaks[0].hz.toFixed(2)+' Hz | '+d.peaks[0].mag.toFixed(4));setText('vp2',d.peaks[1].hz.toFixed(2)+' Hz | '+d.peaks[1].mag.toFixed(4));setText('vp3',d.peaks[2].hz.toFixed(2)+' Hz | '+d.peaks[2].mag.toFixed(4));drawSpectrum('vfftChart',d.hz,d.mag,d.peaks);drawBars('vfftBands',['Low','Mid','High'],[d.lowBand,d.midBand,d.highBand],['#4da3ff','#3ddc84','#ff4d4d'],0.001);}";
  s += "setInterval(poll,500);poll();";
  s += "</script>";

  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleLiveTemperaturePage() {
  String s = htmlHeader("Live Temperature");
  s += "<h1>Live Temperature</h1>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>Object</b><br><span id='objF'>-</span></div>";
  s += "<div class='metric'><b>Reference</b><br><span id='refF'>-</span></div>";
  s += "<div class='metric'><b>Ambient</b><br><span id='ambF'>-</span></div>";
  s += "<div class='metric'><b>Delta</b><br><span id='dTF'>-</span></div>";
  s += "<div class='metric'><b>Max Delta</b><br><span id='dmax'>-</span></div>";
  s += "<div class='metric'><b>Average Delta</b><br><span id='davg'>-</span></div>";
  s += "<div class='metric'><b>Rising Fast</b><br><span id='rise'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Temperature Trend</h2><canvas id='tempChart' width='900' height='260'></canvas></div>";
  s += "<div class='card'><h2>Delta Trend</h2><canvas id='deltaChart' width='900' height='220'></canvas></div>";
  s += "<div class='card'><h2>Current Thermal Snapshot</h2><canvas id='tempBars' width='900' height='220'></canvas></div>";

  s += "<script>";
  s += "const MAX_POINTS=140; const obj=[],ref=[],amb=[],dt=[];";
  s += "function push(a,v){a.push(v); if(a.length>MAX_POINTS) a.shift();}";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "function movingAverage(arr,win=8){const out=[]; for(let i=0;i<arr.length;i++){let s=0,n=0; for(let j=Math.max(0,i-win+1);j<=i;j++){s+=arr[j];n++;} out.push(n?s/n:0);} return out;}";
  s += "function rangeOf(series,opts={}){let all=[];series.forEach(a=>all=all.concat(a.filter(v=>Number.isFinite(v))));if(!all.length)return{min:0,max:1};let min=Math.min(...all),max=Math.max(...all);if(opts.absoluteFloor!==undefined)max=Math.max(max,opts.absoluteFloor);if(opts.zeroMin)min=0;if(max===min)max=min+1;const span=max-min,pad=span*(opts.paddingFrac||0.12);min-=opts.zeroMin?0:pad;max+=pad;if(opts.zeroMin&&min<0)min=0;return{min,max};}";
  s += "function drawAdvancedChart(canvasId,series,colors,opts={}){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#2d2d2d';for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}for(let i=1;i<6;i++){let x=w*i/6;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}const rg=rangeOf(series,opts);function py(v){return h-((v-rg.min)/(rg.max-rg.min))*h;}series.forEach((arr,i)=>{if(!arr||arr.length<2)return;ctx.strokeStyle=colors[i];ctx.lineWidth=2;ctx.beginPath();for(let k=0;k<arr.length;k++){let x=(k/(arr.length-1))*w;let y=py(arr[k]);if(k===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();});ctx.fillStyle='#aaa';ctx.font='12px Arial';ctx.fillText('min:'+rg.min.toFixed(2),8,h-8);ctx.fillText('max:'+rg.max.toFixed(2),8,14);}";
  s += "function drawBars(canvasId,labels,values,colors,maxFloor=1){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);const maxVal=Math.max(...values,maxFloor);const barW=Math.floor(w/values.length)-20;values.forEach((v,i)=>{const x=20+i*Math.floor(w/values.length);const bh=(v/maxVal)*(h-50);const y=h-bh-25;ctx.fillStyle=colors[i];ctx.fillRect(x,y,barW,bh);ctx.fillStyle='#ddd';ctx.font='12px Arial';ctx.fillText(labels[i],x,h-8);ctx.fillText(v.toFixed(2),x,y-6);});}";
  s += "async function poll(){const r=await fetch('/api/analysis');const d=await r.json();setText('objF',d.objF.toFixed(2));setText('refF',d.refF.toFixed(2));setText('ambF',d.ambF.toFixed(2));setText('dTF',d.dTF.toFixed(2));setText('dmax',d.temperature.maxDelta.toFixed(2));setText('davg',d.temperature.avgDelta.toFixed(2));setText('rise',d.temperature.risingFast?'Yes':'No');push(obj,d.objF);push(ref,d.refF);push(amb,d.ambF);push(dt,d.dTF);drawAdvancedChart('tempChart',[obj,ref,amb],['#ffd54a','#4da3ff','#ff8c42'],{absoluteFloor:80,paddingFrac:0.08});drawAdvancedChart('deltaChart',[dt,movingAverage(dt,10)],['#ff4d9d','#ffffff'],{zeroMin:true,absoluteFloor:1,paddingFrac:0.15});drawBars('tempBars',['Object','Reference','Ambient','Delta'],[d.objF,d.refF,d.ambF,Math.max(0,d.dTF)],['#ffd54a','#4da3ff','#ff8c42','#ff4d9d'],1);}";
  s += "setInterval(poll,500); poll();";
  s += "</script>";
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleLiveThermalPage() {
  String s = htmlHeader("Live Thermal");
  s += "<h1>Live Thermal</h1>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>Hotspot</b><br><span id='hotF'>-</span></div>";
  s += "<div class='metric'><b>Pointer</b><br><span id='ptrF'>-</span></div>";
  s += "<div class='metric'><b>Pointer XY</b><br><span id='ptrXY'>-</span></div>";
  s += "<div class='metric'><b>Range</b><br><span id='rngF'>-</span></div>";
  s += "<div class='metric'><b>Hotspot XY</b><br><span id='hotXY'>-</span></div>";
  s += "<div class='metric'><b>Mode</b><br><span id='rangeMode'>-</span></div>";
  s += "<div class='metric'><b>Zoom</b><br><span id='zoomMode'>-</span></div>";
  s += "<div class='metric'><b>Center</b><br><span id='centerXY'>-</span></div>";
  s += "<div class='metric'><b>Palette</b><br><span id='paletteName'>-</span></div>";
  s += "<div class='metric'><b>Hotspot Mode</b><br><span id='hotMode'>-</span></div>";
  s += "<div class='metric'><b>Threshold</b><br><span id='thrMode'>-</span></div>";
  s += "<div class='metric'><b>Hot Pixels</b><br><span id='thrPixels'>-</span></div>";
  s += "<div class='metric'><b>Hot Area %</b><br><span id='thrPct'>-</span></div>";
  s += "<div class='metric'><b>Hot Avg</b><br><span id='thrAvg'>-</span></div>";
  s += "<div class='metric'><b>Hot Max</b><br><span id='thrMax'>-</span></div>";
  s += "<div class='metric'><b>Hot Box</b><br><span id='thrBox'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Thermal Range Control</h2>";
  s += "<a class='btn' href='/set_thermal_range_mode?mode=auto'>Auto Range</a> ";
  s += "<a class='btn' href='/set_thermal_range_mode?mode=fixed&minF=70&maxF=120'>Fixed 70-120F</a> ";
  s += "<a class='btn' href='/set_thermal_range_mode?mode=fixed&minF=80&maxF=140'>Fixed 80-140F</a> ";
  s += "<a class='btn' href='/set_thermal_range_mode?mode=fixed&minF=100&maxF=200'>Fixed 100-200F</a>";
  s += "</div>";

  s += "<div class='card'><h2>Thermal Zoom</h2>";
  s += "<a class='btn' href='/set_thermal_zoom?z=1'>1x</a> ";
  s += "<a class='btn' href='/set_thermal_zoom?z=2'>2x</a> ";
  s += "<a class='btn' href='/set_thermal_zoom?z=3'>3x</a>";
  s += "<div class='muted' style='margin-top:8px;'>Click the thermal image to center the zoom view.</div>";
  s += "</div>";

  s += "<div class='card'><h2>Palette</h2>";
  s += "<a class='btn' href='/set_thermal_palette?p=iron'>Iron</a> ";
  s += "<a class='btn' href='/set_thermal_palette?p=rainbow'>Rainbow</a> ";
  s += "<a class='btn' href='/set_thermal_palette?p=grayscale'>Grayscale</a>";
  s += "</div>";

  s += "<div class='card'><h2>Hotspot Mode</h2>";
  s += "<a class='btn' href='/set_thermal_hotspot_mode?mode=auto'>Auto</a> ";
  s += "<a class='btn' href='/set_thermal_hotspot_mode?mode=locked'>Locked</a>";
  s += "<div class='muted' style='margin-top:8px;'>When locked, click the thermal image to move the hotspot lock point.</div>";
  s += "</div>";

  s += "<div class='card'><h2>Threshold Highlight</h2>";
  s += "<a class='btn' href='/set_thermal_threshold?mode=off'>Off</a> ";
  s += "<a class='btn' href='/set_thermal_threshold?mode=above&f=90'>90F+</a> ";
  s += "<a class='btn' href='/set_thermal_threshold?mode=above&f=100'>100F+</a> ";
  s += "<a class='btn' href='/set_thermal_threshold?mode=above&f=120'>120F+</a>";
  s += "</div>";

  s += "<div class='card'><canvas id='thermalCanvas' width='640' height='480' style='max-width:100%;border:1px solid #333;'></canvas></div>";

  s += "<script>";
  s += "const canvas=document.getElementById('thermalCanvas');";
  s += "const ctx=canvas.getContext('2d');";
  s += "let lastFrame=null;";
  s += "let pointerX=16,pointerY=12,pointerReady=false,lastPointerSendMs=0;";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "function colorMap(v,minV,maxV,palette='iron'){let t=(v-minV)/Math.max(0.001,maxV-minV);if(t<0)t=0;if(t>1)t=1;let r=0,g=0,b=0;if(palette==='grayscale'){const vv=Math.floor(255*t);r=g=b=vv;}else if(palette==='rainbow'){if(t<0.25){let u=t/0.25;r=0;g=Math.floor(255*u);b=255;}else if(t<0.5){let u=(t-0.25)/0.25;r=0;g=255;b=Math.floor(255*(1-u));}else if(t<0.75){let u=(t-0.5)/0.25;r=Math.floor(255*u);g=255;b=0;}else{let u=(t-0.75)/0.25;r=255;g=Math.floor(255*(1-u));b=0;}}else{if(t<0.33){let u=t/0.33;r=Math.floor(80*u);g=0;b=Math.floor(40+100*u);}else if(t<0.66){let u=(t-0.33)/0.33;r=Math.floor(80+120*u);g=Math.floor(40*u);b=Math.floor(140*(1-u));}else{let u=(t-0.66)/0.34;r=255;g=Math.floor(60+195*u);b=Math.floor(20*(1-u));}}return `rgb(${r},${g},${b})`;}";
  s += "function bilinearSample(arr,w,h,x,y){const x0=Math.floor(x),y0=Math.floor(y);const x1=Math.min(w-1,x0+1),y1=Math.min(h-1,y0+1);const tx=x-x0,ty=y-y0;const q00=arr[y0*w+x0],q10=arr[y0*w+x1],q01=arr[y1*w+x0],q11=arr[y1*w+x1];const a=q00*(1-tx)+q10*tx;const b=q01*(1-tx)+q11*tx;return a*(1-ty)+b*ty;}";
  s += "function cropWindow(d){const zoom=d.zoom||1.0;const cropW=d.w/zoom,cropH=d.h/zoom;let x0=(d.centerX??((d.w-1)*0.5))-(cropW-1)*0.5;let y0=(d.centerY??((d.h-1)*0.5))-(cropH-1)*0.5;if(x0<0)x0=0;if(y0<0)y0=0;if(x0+cropW>d.w)x0=d.w-cropW;if(y0+cropH>d.h)y0=d.h-cropH;if(x0<0)x0=0;if(y0<0)y0=0;return{x0,y0,cropW,cropH};}";
  s += "function drawThermal(d){const drawW=";
  s += String(THERMAL_DRAW_W_WEB);
  s += ",drawH=";
  s += String(THERMAL_DRAW_H_WEB);
  s += ";const canvasW=canvas.width,canvasH=canvas.height,scaleW=52,imgWpx=canvasW-scaleW-8,imgHpx=canvasH;const cellW=imgWpx/drawW,cellH=imgHpx/drawH;ctx.clearRect(0,0,canvasW,canvasH);const imageMinF=d.rangeMode==='auto'?d.minF:d.fixedMinF;const imageMaxF=d.rangeMode==='auto'?d.maxF:d.fixedMaxF;const scaleMinF=d.minF,scaleMaxF=d.maxF,palette=d.palette||'iron';const effectiveHotX=(d.hotspotMode==='locked')?d.lockedHotspotX:d.hotspotX;const effectiveHotY=(d.hotspotMode==='locked')?d.lockedHotspotY:d.hotspotY;const effectiveHotF=(d.hotspotMode==='locked')?d.lockedHotspotF:d.hotspotF;const c=cropWindow(d);for(let yy=0;yy<drawH;yy++){for(let xx=0;xx<drawW;xx++){const sx=c.x0+(xx/(drawW-1))*(c.cropW-1);const sy=c.y0+(yy/(drawH-1))*(c.cropH-1);const v=bilinearSample(d.pixelsF,d.w,d.h,sx,sy);ctx.fillStyle=colorMap(v,imageMinF,imageMaxF,palette);ctx.fillRect(xx*cellW,yy*cellH,cellW+1,cellH+1);}}if((d.thresholdMode||'off')==='above'){const thr=d.thresholdF||100.0;for(let yy=0;yy<drawH;yy++){for(let xx=0;xx<drawW;xx++){const sx=c.x0+(xx/(drawW-1))*(c.cropW-1);const sy=c.y0+(yy/(drawH-1))*(c.cropH-1);const v=bilinearSample(d.pixelsF,d.w,d.h,sx,sy);if(v>=thr){ctx.fillStyle='rgba(255,255,255,0.22)';ctx.fillRect(xx*cellW,yy*cellH,cellW+1,cellH+1);}}}}if(d.thresholdRegion&&d.thresholdRegion.valid){const bx0=((d.thresholdRegion.minX-c.x0)/Math.max(0.001,c.cropW-1))*imgWpx;const by0=((d.thresholdRegion.minY-c.y0)/Math.max(0.001,c.cropH-1))*imgHpx;const bx1=((d.thresholdRegion.maxX-c.x0)/Math.max(0.001,c.cropW-1))*imgWpx;const by1=((d.thresholdRegion.maxY-c.y0)/Math.max(0.001,c.cropH-1))*imgHpx;if(bx1>=0&&by1>=0&&bx0<=imgWpx&&by0<=imgHpx){ctx.strokeStyle='yellow';ctx.lineWidth=2;ctx.strokeRect(bx0,by0,Math.max(1,bx1-bx0),Math.max(1,by1-by0));}}ctx.strokeStyle='#fff';ctx.lineWidth=1;ctx.strokeRect(0,0,imgWpx,imgHpx);const hx=((effectiveHotX-c.x0)/Math.max(0.001,c.cropW-1))*imgWpx;const hy=((effectiveHotY-c.y0)/Math.max(0.001,c.cropH-1))*imgHpx;if(hx>=0&&hx<=imgWpx&&hy>=0&&hy<=imgHpx){ctx.strokeStyle='red';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(hx-10,hy);ctx.lineTo(hx+10,hy);ctx.moveTo(hx,hy-10);ctx.lineTo(hx,hy+10);ctx.stroke();}const px=((pointerX-c.x0)/Math.max(0.001,c.cropW-1))*imgWpx;const py=((pointerY-c.y0)/Math.max(0.001,c.cropH-1))*imgHpx;if(px>=0&&px<=imgWpx&&py>=0&&py<=imgHpx){ctx.strokeStyle='white';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(px-8,py);ctx.lineTo(px+8,py);ctx.moveTo(px,py-8);ctx.lineTo(px,py+8);ctx.stroke();}const scaleX=imgWpx+8,scaleY=10,scaleH=canvasH-20,barW=16;for(let i=0;i<scaleH;i++){const f=scaleMaxF-(i/Math.max(1,scaleH-1))*(scaleMaxF-scaleMinF);ctx.fillStyle=colorMap(f,scaleMinF,scaleMaxF,palette);ctx.fillRect(scaleX,scaleY+i,barW,1);}ctx.strokeStyle='#fff';ctx.lineWidth=1;ctx.strokeRect(scaleX-1,scaleY-1,barW+2,scaleH+2);ctx.fillStyle='#fff';ctx.font='12px Arial';ctx.fillText(scaleMaxF.toFixed(1)+'F',scaleX+22,scaleY+10);ctx.fillText(scaleMinF.toFixed(1)+'F',scaleX+22,scaleY+scaleH);if((d.thresholdMode||'off')==='above'){const thr=d.thresholdF||100.0;const thrY=scaleY+((scaleMaxF-thr)/Math.max(0.001,scaleMaxF-scaleMinF))*scaleH;if(thrY>=scaleY&&thrY<=scaleY+scaleH){ctx.strokeStyle='#ffff00';ctx.beginPath();ctx.moveTo(scaleX-8,thrY);ctx.lineTo(scaleX+barW+8,thrY);ctx.stroke();ctx.fillStyle='#ffff00';ctx.fillText('T',scaleX+24,thrY-4);}}const hotY=scaleY+((scaleMaxF-effectiveHotF)/Math.max(0.001,scaleMaxF-scaleMinF))*scaleH;if(hotY>=scaleY&&hotY<=scaleY+scaleH){ctx.fillStyle='red';ctx.beginPath();ctx.moveTo(scaleX+barW+8,hotY);ctx.lineTo(scaleX+barW+1,hotY-5);ctx.lineTo(scaleX+barW+1,hotY+5);ctx.closePath();ctx.fill();ctx.strokeStyle='red';ctx.beginPath();ctx.moveTo(scaleX-1,hotY);ctx.lineTo(scaleX+barW+2,hotY);ctx.stroke();ctx.fillText('H',scaleX+24,hotY-4);}const rawP=d.pixelsF[pointerY*d.w+pointerX];const pTemp=(typeof d.pointerDisplayF==='number'&&d.pointerX===pointerX&&d.pointerY===pointerY)?d.pointerDisplayF:rawP;const ptrY=scaleY+((scaleMaxF-pTemp)/Math.max(0.001,scaleMaxF-scaleMinF))*scaleH;if(ptrY>=scaleY&&ptrY<=scaleY+scaleH){ctx.fillStyle='white';ctx.beginPath();ctx.moveTo(scaleX-8,ptrY);ctx.lineTo(scaleX-1,ptrY-5);ctx.lineTo(scaleX-1,ptrY+5);ctx.closePath();ctx.fill();ctx.strokeStyle='white';ctx.beginPath();ctx.moveTo(scaleX-1,ptrY);ctx.lineTo(scaleX+barW+2,ptrY);ctx.stroke();ctx.fillText('P',scaleX+24,ptrY-4);}setText('hotF',effectiveHotF.toFixed(2)+' F');setText('ptrF',pTemp.toFixed(2)+' F');setText('ptrXY','('+pointerX+','+pointerY+')');setText('rngF',scaleMinF.toFixed(1)+' - '+scaleMaxF.toFixed(1)+' F');setText('hotXY','('+effectiveHotX+','+effectiveHotY+')');setText('rangeMode',d.rangeMode==='auto'?'AUTO':('FIXED '+d.fixedMinF.toFixed(0)+'-'+d.fixedMaxF.toFixed(0)+' F'));setText('zoomMode',(d.zoom||1.0).toFixed(1)+'x');setText('centerXY','('+d.centerX.toFixed(1)+','+d.centerY.toFixed(1)+')');setText('paletteName',palette.toUpperCase());setText('hotMode',(d.hotspotMode||'auto').toUpperCase());setText('thrMode',(d.thresholdMode||'off')==='off'?'OFF':('ABOVE '+d.thresholdF.toFixed(1)+' F'));if(d.thresholdRegion&&d.thresholdRegion.valid){setText('thrPixels',d.thresholdRegion.pixelCount.toString());setText('thrPct',d.thresholdRegion.percentOfFrame.toFixed(1)+'%');setText('thrAvg',d.thresholdRegion.avgF.toFixed(1)+' F');setText('thrMax',d.thresholdRegion.maxF.toFixed(1)+' F');setText('thrBox','('+d.thresholdRegion.minX+','+d.thresholdRegion.minY+')-('+d.thresholdRegion.maxX+','+d.thresholdRegion.maxY+')');}else{setText('thrPixels','0');setText('thrPct','0.0%');setText('thrAvg','-');setText('thrMax','-');setText('thrBox','-');}}";
  s += "canvas.addEventListener('mousemove',(e)=>{if(!lastFrame)return;const rect=canvas.getBoundingClientRect();const x=(e.clientX-rect.left)*canvas.width/rect.width;const y=(e.clientY-rect.top)*canvas.height/rect.height;const imgWpx=canvas.width-52-8;if(x>imgWpx)return;const c=cropWindow(lastFrame);pointerX=Math.round(c.x0+(x/imgWpx)*(c.cropW-1));pointerY=Math.round(c.y0+(y/canvas.height)*(c.cropH-1));pointerX=Math.max(0,Math.min(lastFrame.w-1,pointerX));pointerY=Math.max(0,Math.min(lastFrame.h-1,pointerY));drawThermal(lastFrame);const now=performance.now();if(now-lastPointerSendMs>120){lastPointerSendMs=now;fetch(`/set_thermal_pointer?x=${pointerX}&y=${pointerY}`).catch(()=>{});}});";
  s += "canvas.addEventListener('click',async(e)=>{if(!lastFrame)return;const rect=canvas.getBoundingClientRect();const x=(e.clientX-rect.left)*canvas.width/rect.width;const y=(e.clientY-rect.top)*canvas.height/rect.height;const imgWpx=canvas.width-52-8;if(x>imgWpx)return;const c=cropWindow(lastFrame);const tx=c.x0+(x/imgWpx)*(c.cropW-1);const ty=c.y0+(y/canvas.height)*(c.cropH-1);fetch(`/set_thermal_center?x=${tx.toFixed(2)}&y=${ty.toFixed(2)}`).catch(()=>{});if((lastFrame.hotspotMode||'auto')==='locked'){fetch(`/set_thermal_hotspot_point?x=${tx.toFixed(0)}&y=${ty.toFixed(0)}`).catch(()=>{});}});";
  s += "async function poll(){try{const r=await fetch('/api/thermal_frame');const txt=await r.text();const d=JSON.parse(txt);if(!d.valid)return;lastFrame=d;if(!pointerReady){pointerX=d.pointerX;pointerY=d.pointerY;pointerReady=true;}drawThermal(d);}catch(err){console.error('Thermal fetch/parse error:',err);}}";
  s += "setInterval(poll,400);poll();";
  s += "</script>";

  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleSoundPage() {
  String s = htmlHeader("Live Sound");
  s += "<h1>Live Sound</h1>";
  s += "<div class='card'>SNDv2</div>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>Level</b><br><span id='sndLevel'>-</span></div>";
  s += "<div class='metric'><b>Peak Hz</b><br><span id='sndPeak'>-</span></div>";
  s += "<div class='metric'><b>FFT Status</b><br><span id='sndStatus'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Sound FFT</h2><canvas id='sndCanvas' width='900' height='260'></canvas></div>";

  s += "<script>";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "const sndCanvas=document.getElementById('sndCanvas');";
  s += "const sndCtx=sndCanvas.getContext('2d');";
  s += "function drawSndFFT(spec){sndCtx.clearRect(0,0,sndCanvas.width,sndCanvas.height);sndCtx.fillStyle='#0d0d0d';sndCtx.fillRect(0,0,sndCanvas.width,sndCanvas.height);sndCtx.strokeStyle='#2d2d2d';for(let i=1;i<4;i++){let y=sndCanvas.height*i/4;sndCtx.beginPath();sndCtx.moveTo(0,y);sndCtx.lineTo(sndCanvas.width,y);sndCtx.stroke();}sndCtx.strokeStyle='#ffffff';sndCtx.strokeRect(0,0,sndCanvas.width,sndCanvas.height);if(!spec||!spec.valid||!spec.bins||!spec.bins.length){setText('sndStatus','warming up...');return;}setText('sndStatus','active');let maxBin=0.001;for(let i=1;i<spec.bins.length;i++){if(spec.bins[i]>maxBin)maxBin=spec.bins[i];}for(let i=1;i<spec.bins.length;i++){const x=(i/spec.bins.length)*sndCanvas.width;const h=(spec.bins[i]/maxBin)*(sndCanvas.height-2);sndCtx.beginPath();sndCtx.moveTo(x,sndCanvas.height-1);sndCtx.lineTo(x,sndCanvas.height-1-h);sndCtx.strokeStyle='cyan';sndCtx.stroke();}sndCtx.fillStyle='#aaa';sndCtx.font='12px Arial';sndCtx.fillText('max: '+maxBin.toFixed(3),8,14);}";
  s += "async function poll(){try{const r=await fetch('/api/sound');const d=await r.json();setText('sndLevel',d.sound.dbRel.toFixed(1)+' dB rel');setText('sndPeak',d.soundSpectrum.valid?d.soundSpectrum.dominantHz.toFixed(1)+' Hz':'-');drawSndFFT(d.soundSpectrum);}catch(err){console.error('sound fetch error',err);}}";
  s += "setInterval(poll,300); poll();";
  s += "</script>";
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleLiveSoundPage() {
  handleSoundPage();
}

void WebUI::handleLiveSoundFFTPage() {
  String s = htmlHeader("Live Sound FFT");
  s += "<h1>Live Sound FFT</h1>";

  s += "<div class='card'><div class='grid'>";
  s += "<div class='metric'><b>Dominant Hz</b><br><span id='fft_dom_hz'>-</span></div>";
  s += "<div class='metric'><b>Centroid Hz</b><br><span id='fft_centroid'>-</span></div>";
  s += "<div class='metric'><b>Character</b><br><span id='fft_character'>-</span></div>";
  s += "<div class='metric'><b>H2</b><br><span id='fft_h2'>-</span></div>";
  s += "<div class='metric'><b>H3</b><br><span id='fft_h3'>-</span></div>";
  s += "<div class='metric'><b>Low/Mid/High</b><br><span id='fft_bands'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Top Peaks</h2><div class='grid'>";
  s += "<div class='metric'><b>Peak 1</b><br><span id='p1'>-</span></div>";
  s += "<div class='metric'><b>Peak 2</b><br><span id='p2'>-</span></div>";
  s += "<div class='metric'><b>Peak 3</b><br><span id='p3'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Spectrum</h2><canvas id='fftChart' width='900' height='280'></canvas></div>";
  s += "<div class='card'><h2>Band Balance</h2><canvas id='fftBands' width='900' height='220'></canvas></div>";

  s += "<script>";
  s += "function setText(id,v){document.getElementById(id).textContent=v;}";
  s += "function drawSpectrum(canvasId,hz,mag,peaks){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#2d2d2d';for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}if(!mag.length)return;const maxMag=Math.max(...mag,0.001);const maxHz=Math.max(...hz,1);for(let i=0;i<mag.length;i++){const x=(hz[i]/maxHz)*w;const bh=(mag[i]/maxMag)*(h-20);ctx.fillStyle='#ffd54a';ctx.fillRect(x,h-bh,Math.max(2,w/mag.length-2),bh);}ctx.strokeStyle='#ff4d4d';ctx.fillStyle='#ff4d4d';ctx.font='12px Arial';peaks.forEach((p,i)=>{if(!p||!p.hz)return;const x=(p.hz/maxHz)*w;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();ctx.fillText('P'+(i+1)+': '+p.hz.toFixed(1)+'Hz',Math.min(x+4,w-120),16+i*14);});ctx.fillStyle='#aaa';ctx.fillText('0 Hz',8,h-8);ctx.fillText(maxHz.toFixed(0)+' Hz',w-70,h-8);ctx.fillText('max mag: '+maxMag.toFixed(3),8,14);}";
  s += "function drawBars(canvasId,labels,values,colors,maxFloor=1){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);const maxVal=Math.max(...values,maxFloor);const barW=Math.floor(w/values.length)-30;values.forEach((v,i)=>{const x=25+i*Math.floor(w/values.length);const bh=(v/maxVal)*(h-50);const y=h-bh-25;ctx.fillStyle=colors[i];ctx.fillRect(x,y,barW,bh);ctx.fillStyle='#ddd';ctx.font='12px Arial';ctx.fillText(labels[i],x,h-8);ctx.fillText(v.toFixed(3),x,y-6);});}";
  s += "async function poll(){const r=await fetch('/api/sound_fft');const d=await r.json();setText('fft_dom_hz',d.dominantHz.toFixed(1));setText('fft_centroid',d.centroidHz.toFixed(1));setText('fft_character',d.character);setText('fft_h2',d.harmonic2?'Yes':'No');setText('fft_h3',d.harmonic3?'Yes':'No');setText('fft_bands',d.lowBand.toFixed(2)+' / '+d.midBand.toFixed(2)+' / '+d.highBand.toFixed(2));setText('p1',d.peaks[0].hz.toFixed(1)+' Hz | '+d.peaks[0].mag.toFixed(3));setText('p2',d.peaks[1].hz.toFixed(1)+' Hz | '+d.peaks[1].mag.toFixed(3));setText('p3',d.peaks[2].hz.toFixed(1)+' Hz | '+d.peaks[2].mag.toFixed(3));drawSpectrum('fftChart',d.hz,d.mag,d.peaks);drawBars('fftBands',['Low','Mid','High'],[d.lowBand,d.midBand,d.highBand],['#4da3ff','#3ddc84','#ff4d4d'],0.1);}";
  s += "setInterval(poll,500);poll();";
  s += "</script>";

  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleSessions() {
  String s = htmlHeader("Sessions");
  s += "<h1>Sessions</h1>";
  s += sd.listSessionsCardsHtml();
  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleFiles() {
  String s = htmlHeader("Sessions");
  s += "<h1>Sessions</h1>";
  std::vector<SessionInfo> sessions = sd.listSessions();

  s += "<div class='card'>";
  s += "<h2>Compare Sessions</h2>";
  s += "<form method='GET' action='/compare'>";
  s += "<label>Session A CSV</label><br>";
  s += "<select name='a' style='width:100%;padding:8px;margin:8px 0 16px 0;'>";
  for (size_t i = 0; i < sessions.size(); i++) {
    if (sessions[i].hasCsv) {
      String csvName = sessions[i].baseName + ".csv";
      s += "<option value='" + htmlEscapeLocal(csvName) + "'>" + htmlEscapeLocal(csvName) + "</option>";
    }
  }
  s += "</select>";
  s += "<label>Session B CSV</label><br>";
  s += "<select name='b' style='width:100%;padding:8px;margin:8px 0 16px 0;'>";
  for (size_t i = 0; i < sessions.size(); i++) {
    if (sessions[i].hasCsv) {
      String csvName = sessions[i].baseName + ".csv";
      s += "<option value='" + htmlEscapeLocal(csvName) + "'>" + htmlEscapeLocal(csvName) + "</option>";
    }
  }
  s += "</select>";
  s += "<button class='btn' type='submit'>Compare</button>";
  s += "</form>";
  s += "</div>";
  s += sd.listSessionsCardsHtml();
  s += htmlFooter();
  server.send(200, "text/html", s);
}

static String contentTypeFromName(const String& name) {
  if (name.endsWith(".wav")) return "audio/wav";
  if (name.endsWith(".csv")) return "text/csv";
  return "application/octet-stream";
}

void WebUI::handleDownload() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing file name");
    return;
  }

  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;

  File f = sd.openRead(name);
  if (!f) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(f, contentTypeFromName(name));
  f.close();
}

void WebUI::handleDelete() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing file name");
    return;
  }

  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;

  if (!sd.removeFile(name)) {
    server.send(500, "text/plain", "Delete failed");
    return;
  }

  server.sendHeader("Location", "/files");
  server.send(303);
}

void WebUI::handleDeleteSession() {
  if (!server.hasArg("base")) {
    server.send(400, "text/plain", "Missing session base");
    return;
  }

  String base = server.arg("base");
  if (!base.startsWith("/")) base = "/" + base;

  String csvName = base + ".csv";
  String wavName = base + ".wav";
  String metaName = base + ".meta.json";

  bool ok = true;
  if (sd.exists(csvName)) ok = sd.removeFile(csvName) && ok;
  if (sd.exists(wavName)) ok = sd.removeFile(wavName) && ok;
  if (sd.exists(metaName)) ok = sd.removeFile(metaName) && ok;

  if (!ok) {
    server.send(500, "text/plain", "Failed to delete session");
    return;
  }

  server.sendHeader("Location", "/files");
  server.send(303);
}

void WebUI::handleEditSessionPage() {
  if (!server.hasArg("base")) {
    server.send(400, "text/plain", "Missing session base");
    return;
  }

  String base = server.arg("base");
  if (!base.startsWith("/")) base = "/" + base;

  SessionInfo info;
  info.baseName = base;
  sd.loadSessionMeta(base, info);

  String s = htmlHeader("Edit Session");
  s += "<h1>Edit Session</h1>";
  s += "<div class='card'>";
  s += "<form method='POST' action='/save_session_meta'>";
  s += "<input type='hidden' name='base' value='" + htmlEscapeLocal(base) + "'>";
  s += "<label>Tag</label><br>";
  s += "<input type='text' name='tag' value='" + htmlEscapeLocal(info.tag) + "' style='width:100%;padding:8px;margin:8px 0 16px 0;'><br>";
  s += "<label>Status</label><br>";
  s += "<input type='text' name='status' value='" + htmlEscapeLocal(info.status) + "' style='width:100%;padding:8px;margin:8px 0 16px 0;'><br>";
  s += "<label>Note</label><br>";
  s += "<textarea name='note' style='width:100%;height:120px;padding:8px;margin:8px 0 16px 0;'>" + htmlEscapeLocal(info.note) + "</textarea><br>";
  s += "<button class='btn' type='submit'>Save</button>";
  s += "</form>";
  s += "</div>";
  s += htmlFooter();

  server.send(200, "text/html", s);
}

void WebUI::handleSaveSessionMeta() {
  if (!server.hasArg("base")) {
    server.send(400, "text/plain", "Missing session base");
    return;
  }

  String base = server.arg("base");
  String tag = server.hasArg("tag") ? server.arg("tag") : "";
  String status = server.hasArg("status") ? server.arg("status") : "";
  String note = server.hasArg("note") ? server.arg("note") : "";

  if (!sd.saveSessionMeta(base, tag, status, note)) {
    server.send(500, "text/plain", "Failed to save metadata");
    return;
  }

  server.sendHeader("Location", "/files");
  server.send(303);
}

void WebUI::handleComparePage() {
  if (!server.hasArg("a") || !server.hasArg("b")) {
    server.send(400, "text/plain", "Missing compare files");
    return;
  }

  String a = server.arg("a");
  String b = server.arg("b");
  if (!a.startsWith("/")) a = "/" + a;
  if (!b.startsWith("/")) b = "/" + b;

  String s = htmlHeader("Compare Sessions");
  s += "<h1>Compare Sessions</h1>";
  s += "<div class='card'><div class='muted'>A: " + htmlEscapeLocal(a) + "</div><div class='muted'>B: " + htmlEscapeLocal(b) + "</div></div>";
  s += "<div class='card'><h2>Total Vibration</h2><canvas id='vibChart' width='900' height='260'></canvas></div>";
  s += "<div class='card'><h2>Delta Temperature</h2><canvas id='tempChart' width='900' height='260'></canvas></div>";
  s += "<div class='card'><h2>Sound dB</h2><canvas id='soundChart' width='900' height='260'></canvas></div>";
  s += "<script>";
  s += "function drawChart(canvasId,series,colors){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);ctx.strokeStyle='#2d2d2d';for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}let all=[];series.forEach(a=>all=all.concat(a));let max=Math.max(...all.map(v=>Math.abs(v)),0.001);function py(v){return h-(Math.abs(v)/max)*h;}series.forEach((arr,i)=>{if(arr.length<2)return;ctx.strokeStyle=colors[i];ctx.lineWidth=2;ctx.beginPath();for(let k=0;k<arr.length;k++){let x=(k/(arr.length-1))*w;let y=py(arr[k]);if(k===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();});}";
  s += "async function loadCsv(name){const r=await fetch('/api/file?name='+encodeURIComponent(name));const txt=await r.text();const lines=txt.trim().split(/\\r?\\n/);const out={vt:[],dt:[],db:[]};const hasTrust=lines[0]&&lines[0].indexOf('trusted')>=0;for(let i=1;i<lines.length;i++){const p=lines[i].split(',');if(hasTrust){if(p.length<15)continue;const trusted=parseInt(p[1]||0,10);if(!trusted)continue;out.vt.push(parseFloat(p[7]||0));out.dt.push(parseFloat(p[11]||0));out.db.push(parseFloat(p[13]||0));}else{if(p.length<12)continue;out.vt.push(parseFloat(p[4]||0));out.dt.push(parseFloat(p[8]||0));out.db.push(parseFloat(p[10]||0));}}return out;}";
  s += "async function run(){const a=await loadCsv('" + a + "');const b=await loadCsv('" + b + "');drawChart('vibChart',[a.vt,b.vt],['#4da3ff','#ff8c42']);drawChart('tempChart',[a.dt,b.dt],['#4da3ff','#ff8c42']);drawChart('soundChart',[a.db,b.db],['#4da3ff','#ff8c42']);}";
  s += "run();";
  s += "</script>";
  s += htmlFooter();

  server.send(200, "text/html", s);
}

void WebUI::handleRecordStart() {
  if (startRecordingCb) startRecordingCb();
  server.sendHeader("Location", "/");
  server.send(303);
}

void WebUI::handleRecordStop() {
  if (stopRecordingCb) stopRecordingCb();
  server.sendHeader("Location", "/");
  server.send(303);
}

void WebUI::handleSetVibrationFFTaxis() {
  if (!vibrationFFTaxisPtr || !server.hasArg("axis")) {
    server.send(400, "text/plain", "Missing axis");
    return;
  }

  String axis = server.arg("axis");
  if (axis == "X") *vibrationFFTaxisPtr = VIB_AXIS_X;
  else if (axis == "Y") *vibrationFFTaxisPtr = VIB_AXIS_Y;
  else if (axis == "Z") *vibrationFFTaxisPtr = VIB_AXIS_Z;
  else if (axis == "MAG") *vibrationFFTaxisPtr = VIB_AXIS_MAG;

  server.sendHeader("Location", "/live/vibration_fft");
  server.send(303);
}

void WebUI::handleSetManualOverride() {
  if (!manualOverridePtr || !server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing override parameter");
    return;
  }

  String enabled = server.arg("enabled");
  *manualOverridePtr = (enabled == "1");

  server.sendHeader("Location", "/overview");
  server.send(303);
}

void WebUI::handleSetThermalRangeMode() {
  if (!thermalRangeModePtr || !server.hasArg("mode")) {
    server.send(400, "text/plain", "Missing mode");
    return;
  }

  String mode = server.arg("mode");
  if (mode == "auto") {
    *thermalRangeModePtr = THERMAL_RANGE_AUTO;
  } else if (mode == "fixed") {
    *thermalRangeModePtr = THERMAL_RANGE_FIXED;
  } else {
    server.send(400, "text/plain", "Invalid mode");
    return;
  }

  if (thermalFixedMinPtr && server.hasArg("minF")) {
    *thermalFixedMinPtr = server.arg("minF").toFloat();
  }
  if (thermalFixedMaxPtr && server.hasArg("maxF")) {
    *thermalFixedMaxPtr = server.arg("maxF").toFloat();
  }
  if (thermalFixedMinPtr && thermalFixedMaxPtr && *thermalFixedMaxPtr <= *thermalFixedMinPtr) {
    *thermalFixedMaxPtr = *thermalFixedMinPtr + 1.0f;
  }

  server.sendHeader("Location", "/live/thermal");
  server.send(303);
}

void WebUI::handleSetThermalZoom() {
  if (!thermalZoomPtr || !server.hasArg("z")) {
    server.send(400, "text/plain", "Missing zoom");
    return;
  }

  float z = server.arg("z").toFloat();
  if (z < 1.0f) z = 1.0f;
  if (z > 3.0f) z = 3.0f;

  if (z < 1.5f) z = 1.0f;
  else if (z < 2.5f) z = 2.0f;
  else z = 3.0f;

  *thermalZoomPtr = z;

  if (z == 1.0f && thermalCenterXPtr && thermalCenterYPtr) {
    *thermalCenterXPtr = (THERMAL_W - 1) * 0.5f;
    *thermalCenterYPtr = (THERMAL_H - 1) * 0.5f;
  }

  server.sendHeader("Location", "/live/thermal");
  server.send(303);
}

void WebUI::handleSetThermalCenter() {
  if (!thermalCenterXPtr || !thermalCenterYPtr || !server.hasArg("x") || !server.hasArg("y")) {
    server.send(400, "text/plain", "Missing x/y");
    return;
  }

  float x = server.arg("x").toFloat();
  float y = server.arg("y").toFloat();

  if (x < 0.0f) x = 0.0f;
  if (x > THERMAL_W - 1) x = THERMAL_W - 1;
  if (y < 0.0f) y = 0.0f;
  if (y > THERMAL_H - 1) y = THERMAL_H - 1;

  *thermalCenterXPtr = x;
  *thermalCenterYPtr = y;

  server.send(200, "text/plain", "OK");
}

void WebUI::handleSetThermalPalette() {
  if (!thermalPalettePtr || !server.hasArg("p")) {
    server.send(400, "text/plain", "Missing palette");
    return;
  }

  String p = server.arg("p");
  if (p == "rainbow") *thermalPalettePtr = THERMAL_PALETTE_RAINBOW;
  else if (p == "grayscale") *thermalPalettePtr = THERMAL_PALETTE_GRAYSCALE;
  else *thermalPalettePtr = THERMAL_PALETTE_IRON;

  server.sendHeader("Location", "/live/thermal");
  server.send(303);
}

void WebUI::handleSetThermalHotspotMode() {
  if (!thermalHotspotModePtr || !server.hasArg("mode")) {
    server.send(400, "text/plain", "Missing mode");
    return;
  }

  String mode = server.arg("mode");
  if (mode == "locked") *thermalHotspotModePtr = THERMAL_HOTSPOT_LOCKED;
  else *thermalHotspotModePtr = THERMAL_HOTSPOT_AUTO;

  server.sendHeader("Location", "/live/thermal");
  server.send(303);
}

void WebUI::handleSetThermalHotspotPoint() {
  if (!thermalHotspotXPtr || !thermalHotspotYPtr || !server.hasArg("x") || !server.hasArg("y")) {
    server.send(400, "text/plain", "Missing x/y");
    return;
  }

  int x = server.arg("x").toInt();
  int y = server.arg("y").toInt();

  if (x < 0) x = 0;
  if (x >= THERMAL_W) x = THERMAL_W - 1;
  if (y < 0) y = 0;
  if (y >= THERMAL_H) y = THERMAL_H - 1;

  *thermalHotspotXPtr = x;
  *thermalHotspotYPtr = y;

  server.send(200, "text/plain", "OK");
}

void WebUI::handleSetThermalThreshold() {
  if (!thermalThresholdModePtr || !thermalThresholdFPtr || !server.hasArg("mode")) {
    server.send(400, "text/plain", "Missing threshold args");
    return;
  }

  String mode = server.arg("mode");
  if (mode == "off") {
    *thermalThresholdModePtr = THERMAL_THRESHOLD_OFF;
  } else {
    *thermalThresholdModePtr = THERMAL_THRESHOLD_ABOVE;
  }

  if (server.hasArg("f")) {
    *thermalThresholdFPtr = server.arg("f").toFloat();
  }

  server.sendHeader("Location", "/live/thermal");
  server.send(303);
}

void WebUI::handleSetThermalPointer() {
  if (!server.hasArg("x") || !server.hasArg("y")) {
    server.send(400, "text/plain", "Missing x/y");
    return;
  }

  int x = server.arg("x").toInt();
  int y = server.arg("y").toInt();

  if (thermalPointerSetter) {
    thermalPointerSetter(x, y);
  }

  server.send(200, "text/plain", "OK");
}

void WebUI::handleSaveThermalSnapshot() {
  if (thermalSnapshotCallback) {
    thermalSnapshotCallback();
  }
  server.sendHeader("Location", "/live/thermal");
  server.send(303);
}

void WebUI::handleStatusJson() {
  String s = "{";
  s += "\"sd\":" + String(live.system.sdOK ? "true" : "false") + ",";
  s += "\"recording\":" + String(live.system.recording ? "true" : "false") + ",";
  s += "\"vtot\":" + String(live.vibration.total_in_s, 5) + ",";
  s += "\"objF\":" + String(live.temperature.objF, 1) + ",";
  s += "\"refF\":" + String(live.temperature.refF, 1) + ",";
  s += "\"ambF\":" + String(live.temperature.ambF, 1) + ",";
  s += "\"db\":" + String(live.sound.db, 1) + ",";
  s += "\"hz\":" + String(live.sound.hz, 0);
  s += "}";
  server.send(200, "application/json", s);
}

void WebUI::handleLiveJson() {
  String s = "{";
  s += "\"sd\":" + String(live.system.sdOK ? "true" : "false") + ",";
  s += "\"recording\":" + String(live.system.recording ? "true" : "false") + ",";
  s += "\"vx\":" + String(live.vibration.x_in_s, 5) + ",";
  s += "\"vy\":" + String(live.vibration.y_in_s, 5) + ",";
  s += "\"vz\":" + String(live.vibration.z_in_s, 5) + ",";
  s += "\"vtot\":" + String(live.vibration.total_in_s, 5) + ",";
  s += "\"objF\":" + String(live.temperature.objF, 2) + ",";
  s += "\"refF\":" + String(live.temperature.refF, 2) + ",";
  s += "\"ambF\":" + String(live.temperature.ambF, 2) + ",";
  s += "\"dTF\":" + String(live.temperature.deltaF, 2) + ",";
  s += "\"db\":" + String(live.sound.db, 2) + ",";
  s += "\"rms\":" + String(live.sound.rms, 2) + ",";
  s += "\"hz\":" + String(live.sound.hz, 2);
  s += "}";
  server.send(200, "application/json", s);
}

void WebUI::handleVibrationJson() {
  String s = "{";

  s += "\"vibration\":{";
  s += "\"vx\":" + String(live.vibration.vx, 5) + ",";
  s += "\"vy\":" + String(live.vibration.vy, 5) + ",";
  s += "\"vz\":" + String(live.vibration.vz, 5) + ",";
  s += "\"vt\":" + String(live.vibration.vt, 5) + ",";
  s += "\"maxTotal\":" + String(live.vibration.maxTotal, 5) + ",";
  s += "\"avgTotal\":" + String(live.vibration.avgTotal, 5) + ",";
  s += "\"ax_g\":" + String(live.vibration.ax_g, 5) + ",";
  s += "\"ay_g\":" + String(live.vibration.ay_g, 5) + ",";
  s += "\"az_g\":" + String(live.vibration.az_g, 5) + ",";
  s += "\"dominantAxis\":\"" + live.analysis.vibration.dominantAxis + "\"";
  s += "},";

  s += "\"vibrationSpectrum\":{";
  s += "\"valid\":" + String(live.vibrationSpectrum.valid ? "true" : "false") + ",";
  s += "\"dominantHz\":" + String(live.vibrationSpectrum.dominantHz, 2) + ",";
  s += "\"dominantAmp\":" + String(live.vibrationSpectrum.dominantMag, 6) + ",";
  s += "\"dominantMag\":" + String(live.vibrationSpectrum.dominantMag, 6) + ",";
  s += "\"sourceAxis\":\"" + live.vibrationSpectrum.sourceAxis + "\",";
  s += "\"binCount\":" + String(live.vibrationSpectrum.bins) + ",";
  s += "\"hz\":[";
  for (int i = 0; i < live.vibrationSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.vibrationSpectrum.hz[i], 2);
  }
  s += "],";
  s += "\"mag\":[";
  for (int i = 0; i < live.vibrationSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.vibrationSpectrum.mag[i], 6);
  }
  s += "],";
  s += "\"bins\":[";
  for (int i = 0; i < live.vibrationSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.vibrationSpectrum.mag[i], 6);
  }
  s += "]";
  s += "}";

  s += "}";
  server.send(200, "application/json", s);
}

void WebUI::handleSoundJson() {
  Serial.print("WEB sound json: peak=");
  Serial.print(live.soundSpectrum.dominantHz, 1);
  Serial.print(" valid=");
  Serial.println(live.soundSpectrum.valid ? "T" : "F");

  String s = "{";

  s += "\"sound\":{";
  s += "\"dbRel\":" + String(live.sound.dbRel, 2) + ",";
  s += "\"peakHz\":" + String(live.sound.peakHz, 2) + ",";
  s += "\"db\":" + String(live.sound.db, 2) + ",";
  s += "\"rms\":" + String(live.sound.rms, 2);
  s += "},";

  s += "\"soundSpectrum\":{";
  s += "\"valid\":" + String(live.soundSpectrum.valid ? "true" : "false") + ",";
  s += "\"sampleRateHz\":" + String(live.soundSpectrum.sampleRateHz, 2) + ",";
  s += "\"dominantHz\":" + String(live.soundSpectrum.dominantHz, 2) + ",";
  s += "\"dominantAmp\":" + String(live.soundSpectrum.dominantAmp, 6) + ",";
  s += "\"binCount\":" + String(live.soundSpectrum.bins) + ",";
  s += "\"bins\":[";
  for (int i = 0; i < SoundSpectrumData::BINS; i++) {
    if (i) s += ",";
    float v = (i < live.soundSpectrum.bins) ? live.soundSpectrum.mag[i] : 0.0f;
    s += String(v, 6);
  }
  s += "]";
  s += "}";

  s += "}";
  server.send(200, "application/json", s);
}

void WebUI::handleAnalysisJson() {
  String s = "{";
  s += "\"sd\":" + String(live.system.sdOK ? "true" : "false") + ",";
  s += "\"recording\":" + String(live.system.recording ? "true" : "false") + ",";
  s += "\"recordingMode\":" + String((int)live.system.recordingMode) + ",";
  s += "\"manualOverride\":" + String(live.system.manualOverride ? "true" : "false") + ",";
  s += "\"statusText\":\"" + live.system.statusText + "\",";
  s += "\"vx\":" + String(live.vibration.x_in_s, 5) + ",";
  s += "\"vy\":" + String(live.vibration.y_in_s, 5) + ",";
  s += "\"vz\":" + String(live.vibration.z_in_s, 5) + ",";
  s += "\"vtot\":" + String(live.vibration.total_in_s, 5) + ",";
  s += "\"objF\":" + String(live.temperature.objF, 2) + ",";
  s += "\"refF\":" + String(live.temperature.refF, 2) + ",";
  s += "\"ambF\":" + String(live.temperature.ambF, 2) + ",";
  s += "\"dTF\":" + String(live.temperature.deltaF, 2) + ",";
  s += "\"db\":" + String(live.sound.db, 2) + ",";
  s += "\"rms\":" + String(live.sound.rms, 2) + ",";
  s += "\"hz\":" + String(live.sound.hz, 2) + ",";
  s += "\"peak\":" + String(live.sound.peak) + ",";

  s += "\"mount\":{";
  s += "\"state\":" + String((int)live.mount.state) + ",";
  s += "\"confidence\":" + String(live.mount.confidence, 1) + ",";
  s += "\"dynamicG\":" + String(live.mount.dynamicG, 4) + ",";
  s += "\"driftG\":" + String(live.mount.driftG, 4) + ",";
  s += "\"stableMs\":" + String(live.mount.stableMs) + ",";
  s += "\"trusted\":" + String(live.mount.analysisTrusted ? "true" : "false");
  s += "},";

  s += "\"vibration\":{";
  s += "\"currentTotal\":" + String(live.vibration.total_in_s, 5) + ",";
  s += "\"maxTotal\":" + String(live.vibration.maxTotal, 5) + ",";
  s += "\"avgTotal\":" + String(live.vibration.avgTotal, 5) + ",";
  s += "\"dominantAxis\":\"" + live.analysis.vibration.dominantAxis + "\",";
  s += "\"alert\":\"" + live.analysis.vibration.alert + "\"},";

  s += "\"temperature\":{";
  s += "\"currentDelta\":" + String(live.analysis.temperature.currentDelta, 2) + ",";
  s += "\"maxDelta\":" + String(live.analysis.temperature.maxDelta, 2) + ",";
  s += "\"avgDelta\":" + String(live.analysis.temperature.avgDelta, 2) + ",";
  s += "\"risingFast\":" + String(live.analysis.temperature.risingFast ? "true" : "false") + ",";
  s += "\"alert\":\"" + live.analysis.temperature.alert + "\"},";

  s += "\"sound\":{";
  s += "\"currentDb\":" + String(live.analysis.sound.currentDb, 2) + ",";
  s += "\"maxDb\":" + String(live.analysis.sound.maxDb, 2) + ",";
  s += "\"avgDb\":" + String(live.analysis.sound.avgDb, 2) + ",";
  s += "\"dominantHz\":" + String(live.analysis.sound.dominantHz, 2) + ",";
  s += "\"alert\":\"" + live.analysis.sound.alert + "\"}";
  s += "}";

  server.send(200, "application/json", s);
}

void WebUI::handleThermalFrameJson() {
  String s = "{";
  s += "\"valid\":";
  s += live.thermal.valid ? "true" : "false";
  s += ",";
  s += "\"w\":32,";
  s += "\"h\":24,";
  s += "\"rangeMode\":\"";
  s += (live.thermalDisplay.rangeMode == THERMAL_RANGE_AUTO) ? "auto" : "fixed";
  s += "\",";
  s += "\"fixedMinF\":" + String(live.thermalDisplay.fixedMinF, 2) + ",";
  s += "\"fixedMaxF\":" + String(live.thermalDisplay.fixedMaxF, 2) + ",";
  s += "\"zoom\":" + String(live.thermalDisplay.zoom, 1) + ",";
  s += "\"centerX\":" + String(live.thermalDisplay.centerX, 2) + ",";
  s += "\"centerY\":" + String(live.thermalDisplay.centerY, 2) + ",";
  s += "\"palette\":\"";
  switch (live.thermalDisplay.palette) {
    case THERMAL_PALETTE_RAINBOW: s += "rainbow"; break;
    case THERMAL_PALETTE_GRAYSCALE: s += "grayscale"; break;
    default: s += "iron"; break;
  }
  s += "\",";
  s += "\"hotspotMode\":\"";
  s += (live.thermalDisplay.hotspotMode == THERMAL_HOTSPOT_AUTO) ? "auto" : "locked";
  s += "\",";
  s += "\"lockedHotspotX\":" + String(live.thermalDisplay.lockedHotspotX) + ",";
  s += "\"lockedHotspotY\":" + String(live.thermalDisplay.lockedHotspotY) + ",";
  s += "\"lockedHotspotF\":" + String(isfinite(live.thermalDisplay.lockedHotspotF) ? live.thermalDisplay.lockedHotspotF : 0.0f, 2) + ",";
  s += "\"thresholdMode\":\"";
  s += (live.thermalDisplay.thresholdMode == THERMAL_THRESHOLD_OFF) ? "off" : "above";
  s += "\",";
  s += "\"thresholdF\":" + String(live.thermalDisplay.thresholdF, 2) + ",";
  s += "\"minF\":" + String(isfinite(live.thermal.minF) ? live.thermal.minF : 0.0f, 2) + ",";
  s += "\"maxF\":" + String(isfinite(live.thermal.maxF) ? live.thermal.maxF : 0.0f, 2) + ",";
  s += "\"hotspotF\":" + String(isfinite(live.thermal.hotspotF) ? live.thermal.hotspotF : 0.0f, 2) + ",";
  s += "\"hotspotX\":" + String(live.thermal.hotspotX) + ",";
  s += "\"hotspotY\":" + String(live.thermal.hotspotY) + ",";
  s += "\"pointerF\":" + String(isfinite(live.thermal.pointerF) ? live.thermal.pointerF : 0.0f, 2) + ",";
  s += "\"pointerDisplayF\":" + String(isfinite(live.thermal.pointerDisplayF) ? live.thermal.pointerDisplayF : 0.0f, 2) + ",";
  s += "\"pointerX\":" + String(live.thermal.pointerX) + ",";
  s += "\"pointerY\":" + String(live.thermal.pointerY) + ",";
  s += "\"centerF\":" + String(isfinite(live.thermal.centerF) ? live.thermal.centerF : 0.0f, 2) + ",";
  s += "\"ambientF\":" + String(isfinite(live.thermal.ambientF) ? live.thermal.ambientF : 0.0f, 2) + ",";
  s += "\"thresholdRegion\":{";
  s += "\"valid\":";
  s += live.thermal.thresholdRegion.valid ? "true" : "false";
  s += ",";
  s += "\"pixelCount\":" + String(live.thermal.thresholdRegion.pixelCount) + ",";
  s += "\"percentOfFrame\":" + String(isfinite(live.thermal.thresholdRegion.percentOfFrame) ? live.thermal.thresholdRegion.percentOfFrame : 0.0f, 2) + ",";
  s += "\"avgF\":" + String(isfinite(live.thermal.thresholdRegion.avgF) ? live.thermal.thresholdRegion.avgF : 0.0f, 2) + ",";
  s += "\"maxF\":" + String(isfinite(live.thermal.thresholdRegion.maxF) ? live.thermal.thresholdRegion.maxF : 0.0f, 2) + ",";
  s += "\"minX\":" + String(live.thermal.thresholdRegion.minX) + ",";
  s += "\"minY\":" + String(live.thermal.thresholdRegion.minY) + ",";
  s += "\"maxX\":" + String(live.thermal.thresholdRegion.maxX) + ",";
  s += "\"maxY\":" + String(live.thermal.thresholdRegion.maxY);
  s += "},";
  s += "\"pixelsF\":[";
  for (int i = 0; i < THERMAL_PIXELS; i++) {
    if (i) s += ",";
    float v = live.thermal.pixelsF[i];
    if (!isfinite(v)) v = 0.0f;
    s += String(v, 2);
  }
  s += "]}";
  server.send(200, "application/json", s);
}

void WebUI::handleSoundSpectrumJson() {
  String s = "{";
  s += "\"valid\":" + String(live.soundSpectrum.valid ? "true" : "false") + ",";
  s += "\"sampleRateHz\":" + String(live.soundSpectrum.sampleRateHz, 2) + ",";
  s += "\"dominantHz\":" + String(live.soundSpectrum.dominantHz, 2) + ",";
  s += "\"dominantMag\":" + String(live.soundSpectrum.dominantMag, 4) + ",";
  s += "\"dominantAmp\":" + String(live.soundSpectrum.dominantAmp, 6) + ",";
  s += "\"centroidHz\":" + String(live.soundSpectrum.centroidHz, 2) + ",";
  s += "\"lowBand\":" + String(live.soundSpectrum.lowBand, 4) + ",";
  s += "\"midBand\":" + String(live.soundSpectrum.midBand, 4) + ",";
  s += "\"highBand\":" + String(live.soundSpectrum.highBand, 4) + ",";
  s += "\"character\":\"" + live.soundSpectrum.character + "\",";
  s += "\"harmonic2\":" + String(live.soundSpectrum.harmonic2 ? "true" : "false") + ",";
  s += "\"harmonic3\":" + String(live.soundSpectrum.harmonic3 ? "true" : "false") + ",";
  s += "\"peaks\":[";
  for (int i = 0; i < 3; i++) {
    if (i) s += ",";
    s += "{";
    s += "\"hz\":" + String(live.soundSpectrum.peaks[i].hz, 2) + ",";
    s += "\"mag\":" + String(live.soundSpectrum.peaks[i].mag, 4);
    s += "}";
  }
  s += "],";
  s += "\"bins\":" + String(live.soundSpectrum.bins) + ",";
  s += "\"hz\":[";
  for (int i = 0; i < live.soundSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.soundSpectrum.hz[i], 2);
  }
  s += "],";
  s += "\"mag\":[";
  for (int i = 0; i < live.soundSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.soundSpectrum.mag[i], 4);
  }
  s += "]";
  s += "}";

  server.send(200, "application/json", s);
}

void WebUI::handleVibrationSpectrumJson() {
  String s = "{";
  s += "\"dominantHz\":" + String(live.vibrationSpectrum.dominantHz, 2) + ",";
  s += "\"dominantMag\":" + String(live.vibrationSpectrum.dominantMag, 4) + ",";
  s += "\"lowBand\":" + String(live.vibrationSpectrum.lowBand, 4) + ",";
  s += "\"midBand\":" + String(live.vibrationSpectrum.midBand, 4) + ",";
  s += "\"highBand\":" + String(live.vibrationSpectrum.highBand, 4) + ",";
  s += "\"sourceAxis\":\"" + live.vibrationSpectrum.sourceAxis + "\",";
  s += "\"character\":\"" + live.vibrationSpectrum.character + "\",";
  s += "\"harmonic2\":" + String(live.vibrationSpectrum.harmonic2 ? "true" : "false") + ",";
  s += "\"harmonic3\":" + String(live.vibrationSpectrum.harmonic3 ? "true" : "false") + ",";
  s += "\"peaks\":[";
  for (int i = 0; i < 3; i++) {
    if (i) s += ",";
    s += "{";
    s += "\"hz\":" + String(live.vibrationSpectrum.peaks[i].hz, 2) + ",";
    s += "\"mag\":" + String(live.vibrationSpectrum.peaks[i].mag, 4);
    s += "}";
  }
  s += "],";
  s += "\"bins\":" + String(live.vibrationSpectrum.bins) + ",";
  s += "\"hz\":[";
  for (int i = 0; i < live.vibrationSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.vibrationSpectrum.hz[i], 2);
  }
  s += "],";
  s += "\"mag\":[";
  for (int i = 0; i < live.vibrationSpectrum.bins; i++) {
    if (i) s += ",";
    s += String(live.vibrationSpectrum.mag[i], 4);
  }
  s += "]";
  s += "}";

  server.send(200, "application/json", s);
}

void WebUI::handleViewPage() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing CSV file name");
    return;
  }

  String csvName = server.arg("name");
  if (!csvName.startsWith("/")) csvName = "/" + csvName;

  String wavName = csvName;
  if (wavName.endsWith(".csv")) {
    wavName.remove(wavName.length() - 4);
    wavName += ".wav";
  }

  String baseName = csvName;
  if (baseName.endsWith(".csv")) {
    baseName.remove(baseName.length() - 4);
  }

  SessionInfo info;
  info.baseName = baseName;
  sd.loadSessionMeta(baseName, info);

  bool wavExists = sd.exists(wavName);

  String s = htmlHeader("Session Review");
  s += "<h1>Session Review</h1>";

  s += "<div class='card'>";
  s += "<div class='muted'><b>Session:</b> " + htmlEscapeLocal(baseName) + "</div>";
  s += "<div class='muted'><b>CSV:</b> " + htmlEscapeLocal(csvName) + "</div>";
  s += "<div class='muted'><b>WAV:</b> " + htmlEscapeLocal(wavName) + "</div>";
  if (info.tag.length()) s += "<div class='muted'><b>Tag:</b> " + htmlEscapeLocal(info.tag) + "</div>";
  if (info.status.length()) s += "<div class='muted'><b>Status:</b> " + htmlEscapeLocal(info.status) + "</div>";
  if (info.note.length()) s += "<div class='muted'><b>Note:</b> " + htmlEscapeLocal(info.note) + "</div>";
  s += "</div>";

  s += "<div class='card'><h2>Inspection Report</h2>";
  s += "<div class='grid'>";
  s += "<div class='metric'><b>Overall Alert</b><br><span id='rep_overall'>-</span></div>";
  s += "<div class='metric'><b>Vibration Alert</b><br><span id='rep_vib_alert'>-</span></div>";
  s += "<div class='metric'><b>Temp Alert</b><br><span id='rep_temp_alert'>-</span></div>";
  s += "<div class='metric'><b>Sound Alert</b><br><span id='rep_sound_alert'>-</span></div>";
  s += "</div>";
  s += "<div style='margin-top:14px;'><b>Interpretation</b><br><div id='rep_text' class='muted'>Loading...</div></div>";
  s += "<div style='margin-top:14px;'><b>Suggested Focus</b><br><div id='rep_focus' class='muted'>Loading...</div></div>";
  s += "</div>";

  s += "<div class='card'><h2>Session Summary</h2>";
  s += "<div class='grid'>";
  s += "<div class='metric'><b>Max Total Vib</b><br><span id='sum_max_vib'>-</span></div>";
  s += "<div class='metric'><b>Avg Total Vib</b><br><span id='sum_avg_vib'>-</span></div>";
  s += "<div class='metric'><b>Dominant Axis</b><br><span id='sum_axis'>-</span></div>";
  s += "<div class='metric'><b>Samples</b><br><span id='sum_samples'>-</span></div>";
  s += "<div class='metric'><b>Trusted Samples</b><br><span id='sum_trusted_samples'>-</span></div>";
  s += "<div class='metric'><b>Trusted Ratio</b><br><span id='sum_trusted_ratio'>-</span></div>";
  s += "<div class='metric'><b>Valid Start</b><br><span id='sum_valid_start'>-</span></div>";
  s += "<div class='metric'><b>Valid End</b><br><span id='sum_valid_end'>-</span></div>";
  s += "<div class='metric'><b>Valid Duration</b><br><span id='sum_valid_duration'>-</span></div>";
  s += "<div class='metric'><b>Max Delta Temp</b><br><span id='sum_max_dt'>-</span></div>";
  s += "<div class='metric'><b>Avg Delta Temp</b><br><span id='sum_avg_dt'>-</span></div>";
  s += "<div class='metric'><b>Max dB</b><br><span id='sum_max_db'>-</span></div>";
  s += "<div class='metric'><b>Avg dB</b><br><span id='sum_avg_db'>-</span></div>";
  s += "<div class='metric'><b>Max Hz</b><br><span id='sum_max_hz'>-</span></div>";
  s += "</div></div>";

  s += "<div class='card'><h2>Mount Trust Timeline</h2>";
  s += "<canvas id='trustChart' width='900' height='90'></canvas>";
  s += "</div>";

  s += "<div class='card'><h2>Audio Playback</h2>";
  bool activeRecording = live.system.recording && live.system.currentBaseName.length() && wavName.startsWith(live.system.currentBaseName);
  if (activeRecording) {
    s += "<div class='muted'>WAV finalizes when recording stops.</div>";
  } else if (wavExists) {
    s += "<audio id='sessionAudio' controls style='width:100%;'>";
    s += "<source src='/api/file?name=" + wavName + "' type='audio/wav'>";
    s += "Your browser does not support WAV playback.";
    s += "</audio>";
    s += "<div class='muted' style='margin-top:8px;'>Playback cursor is synchronized with charts.</div>";
    s += "<div style='margin-top:10px;'>";
    s += "<a class='btn' href='/download?name=" + wavName + "'>Download WAV</a>";
    s += "</div>";
  } else {
    s += "<div class='muted'>Matching WAV not found.</div>";
  }
  s += "</div>";

  s += "<div class='card'><h2>Vibration</h2>";
  s += "<canvas id='vibChart' width='900' height='260'></canvas>";
  s += "<div class='legend'>";
  s += "<span><span class='dot' style='background:#ff4d4d'></span>X</span>";
  s += "<span><span class='dot' style='background:#3ddc84'></span>Y</span>";
  s += "<span><span class='dot' style='background:#4da3ff'></span>Z</span>";
  s += "<span><span class='dot' style='background:#ffd54a'></span>TOT</span>";
  s += "</div></div>";

  s += "<div class='card'><h2>Temperature</h2>";
  s += "<canvas id='tempChart' width='900' height='260'></canvas>";
  s += "<div class='legend'>";
  s += "<span><span class='dot' style='background:#ffd54a'></span>Object</span>";
  s += "<span><span class='dot' style='background:#4da3ff'></span>Reference</span>";
  s += "<span><span class='dot' style='background:#ff8c42'></span>Ambient</span>";
  s += "<span><span class='dot' style='background:#ff4d9d'></span>Delta</span>";
  s += "</div></div>";

  s += "<div class='card'><h2>Sound</h2>";
  s += "<canvas id='soundChart' width='900' height='260'></canvas>";
  s += "<div class='legend'>";
  s += "<span><span class='dot' style='background:#3ddc84'></span>dB</span>";
  s += "<span><span class='dot' style='background:#ffd54a'></span>Hz/100</span>";
  s += "</div></div>";

  s += "<script>";
  s += "const reviewState={vib:{x:[],y:[],z:[],t:[]},temp:{obj:[],ref:[],amb:[],dt:[]},sound:{db:[],hz:[]},ms:[],trusted:[],cursorRatio:null,hoverIndex:null,sampleCount:0};";
  s += "function setText(id,val){document.getElementById(id).textContent=val;}";
  s += "function avg(arr){if(!arr.length)return 0;return arr.reduce((a,b)=>a+b,0)/arr.length;}";
  s += "function maxv(arr){if(!arr.length)return 0;return Math.max(...arr);}";
  s += "function estimateDuration(){const audio=document.getElementById('sessionAudio');if(audio&&audio.duration&&Number.isFinite(audio.duration)&&audio.duration>0)return audio.duration;return reviewState.sampleCount*0.05;}";
  s += "function ratioToIndex(r){if(reviewState.sampleCount<=1)return 0;const idx=Math.round(r*(reviewState.sampleCount-1));return Math.max(0,Math.min(reviewState.sampleCount-1,idx));}";
  s += "function indexToTime(idx){const dur=estimateDuration();if(reviewState.sampleCount<=1)return 0;return (idx/(reviewState.sampleCount-1))*dur;}";
  s += "function formatTime(sec){sec=Math.max(0,sec||0);const m=Math.floor(sec/60);const ss=sec%60;return m+':'+(ss<10?'0':'')+ss.toFixed(1);}";
  s += "function drawChart(canvasId,series,colors,allowNegative=false,cursorRatio=null,tooltipBuilder=null){";
  s += "const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;";
  s += "ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);";
  s += "ctx.strokeStyle='#2d2d2d';ctx.lineWidth=1;";
  s += "for(let i=1;i<4;i++){let y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}";
  s += "for(let i=1;i<6;i++){let x=w*i/6;ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}";
  s += "let all=[];series.forEach(a=>all=all.concat(a.filter(v=>Number.isFinite(v))));";
  s += "let min=allowNegative?Math.min(...all,-0.001):0;let max=Math.max(...all,0.001);if(max===min)max=min+1;";
  s += "function py(v){return h-((v-min)/(max-min))*h;}";
  s += "series.forEach((arr,i)=>{if(arr.length<2)return;ctx.strokeStyle=colors[i];ctx.lineWidth=2;ctx.beginPath();for(let k=0;k<arr.length;k++){let x=(k/(arr.length-1))*w;let y=py(arr[k]);if(k===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}ctx.stroke();});";
  s += "if(cursorRatio!==null&&Number.isFinite(cursorRatio)){const cx=Math.max(0,Math.min(w,cursorRatio*w));ctx.strokeStyle='#ffffff';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(cx,0);ctx.lineTo(cx,h);ctx.stroke();}";
  s += "if(reviewState.hoverIndex!==null&&reviewState.sampleCount>1){const hx=(reviewState.hoverIndex/(reviewState.sampleCount-1))*w;ctx.strokeStyle='#ff66ff';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(hx,0);ctx.lineTo(hx,h);ctx.stroke();if(tooltipBuilder){const lines=tooltipBuilder(reviewState.hoverIndex);const pad=6,lineH=14;ctx.font='12px Arial';let boxW=0;lines.forEach(line=>{boxW=Math.max(boxW,ctx.measureText(line).width);});const boxH=lines.length*lineH+pad*2;let bx=hx+10;if(bx+boxW+pad*2>w)bx=hx-boxW-pad*2-10;let by=10;ctx.fillStyle='rgba(0,0,0,0.82)';ctx.fillRect(bx,by,boxW+pad*2,boxH);ctx.strokeStyle='#888';ctx.strokeRect(bx,by,boxW+pad*2,boxH);ctx.fillStyle='#fff';lines.forEach((line,i)=>ctx.fillText(line,bx+pad,by+pad+12+i*lineH));}}";
  s += "ctx.fillStyle='#aaa';ctx.font='12px Arial';ctx.fillText('min:'+min.toFixed(3),8,h-8);ctx.fillText('max:'+max.toFixed(3),8,14);";
  s += "}";
  s += "function drawTrustChart(canvasId,trustedArr,cursorRatio=null){const c=document.getElementById(canvasId),ctx=c.getContext('2d');const w=c.width,h=c.height;ctx.clearRect(0,0,w,h);ctx.fillStyle='#0d0d0d';ctx.fillRect(0,0,w,h);if(!trustedArr.length)return;const barW=w/trustedArr.length;for(let i=0;i<trustedArr.length;i++){ctx.fillStyle=trustedArr[i]?'#238636':'#b42318';ctx.fillRect(i*barW,0,Math.max(1,barW),h);}if(cursorRatio!==null&&Number.isFinite(cursorRatio)){const cx=Math.max(0,Math.min(w,cursorRatio*w));ctx.strokeStyle='#ffffff';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(cx,0);ctx.lineTo(cx,h);ctx.stroke();}ctx.fillStyle='#fff';ctx.font='12px Arial';ctx.fillText('Red = untrusted, Green = trusted',8,14);}";
  s += "function redrawReviewCharts(){";
  s += "drawTrustChart('trustChart',reviewState.trusted,reviewState.cursorRatio);";
  s += "drawChart('vibChart',[reviewState.vib.x,reviewState.vib.y,reviewState.vib.z,reviewState.vib.t],['#ff4d4d','#3ddc84','#4da3ff','#ffd54a'],true,reviewState.cursorRatio,(idx)=>['t='+formatTime(indexToTime(idx)),'X='+reviewState.vib.x[idx].toFixed(5),'Y='+reviewState.vib.y[idx].toFixed(5),'Z='+reviewState.vib.z[idx].toFixed(5),'TOT='+reviewState.vib.t[idx].toFixed(5)]);";
  s += "drawChart('tempChart',[reviewState.temp.obj,reviewState.temp.ref,reviewState.temp.amb,reviewState.temp.dt],['#ffd54a','#4da3ff','#ff8c42','#ff4d9d'],false,reviewState.cursorRatio,(idx)=>['t='+formatTime(indexToTime(idx)),'Obj='+reviewState.temp.obj[idx].toFixed(2),'Ref='+reviewState.temp.ref[idx].toFixed(2),'Amb='+reviewState.temp.amb[idx].toFixed(2),'dT='+reviewState.temp.dt[idx].toFixed(2)]);";
  s += "drawChart('soundChart',[reviewState.sound.db,reviewState.sound.hz.map(v=>v/100.0)],['#3ddc84','#ffd54a'],false,reviewState.cursorRatio,(idx)=>['t='+formatTime(indexToTime(idx)),'dB='+reviewState.sound.db[idx].toFixed(2),'Hz='+reviewState.sound.hz[idx].toFixed(1)]);";
  s += "}";

  s += "function makeInterpretation(vibAlert,tempAlert,soundAlert,axis){";
  s += "let parts=[];";
  s += "if(vibAlert==='High')parts.push('Vibration is elevated with strongest activity on axis '+axis+'.');";
  s += "else if(vibAlert==='Watch')parts.push('Vibration is above baseline watch level, especially on axis '+axis+'.');";
  s += "else parts.push('Vibration remains within normal range for this session.');";
  s += "if(tempAlert==='High')parts.push('Temperature delta is significantly elevated.');";
  s += "else if(tempAlert==='Watch')parts.push('Temperature delta is above normal watch level.');";
  s += "else parts.push('Temperature behavior appears stable.');";
  s += "if(soundAlert==='High')parts.push('Acoustic level is high and may indicate roughness or abnormal load.');";
  s += "else if(soundAlert==='Watch')parts.push('Sound level is above normal watch level.');";
  s += "else parts.push('Sound trend appears normal.');";
  s += "return parts.join(' ');";
  s += "}";

  s += "function makeFocus(vibAlert,tempAlert,soundAlert,axis){";
  s += "if(vibAlert==='High')return 'Inspect mechanical condition first: mounting, imbalance, looseness, or axis '+axis+' related components.';";
  s += "if(tempAlert==='High')return 'Inspect thermal causes: friction, overload, cooling, or contact condition.';";
  s += "if(soundAlert==='High')return 'Review acoustic signature and listen to WAV for roughness, impacts, or abnormal tonal behavior.';";
  s += "if(vibAlert==='Watch'||tempAlert==='Watch'||soundAlert==='Watch')return 'Monitor trend and compare against prior sessions before maintenance decision.';";
  s += "return 'No major issue flagged in this recording. Use comparison mode against prior sessions for confirmation.';";
  s += "}";
  s += "function bindAudioCursor(){";
  s += "const audio=document.getElementById('sessionAudio');if(!audio)return;";
  s += "function updateCursor(){if(!audio.duration||!Number.isFinite(audio.duration)||audio.duration<=0){reviewState.cursorRatio=null;}else{reviewState.cursorRatio=audio.currentTime/audio.duration;}redrawReviewCharts();}";
  s += "audio.addEventListener('timeupdate',updateCursor);audio.addEventListener('play',updateCursor);audio.addEventListener('pause',updateCursor);audio.addEventListener('seeked',updateCursor);audio.addEventListener('loadedmetadata',updateCursor);";
  s += "audio.addEventListener('ended',()=>{reviewState.cursorRatio=1;redrawReviewCharts();});";
  s += "}";
  s += "function bindChartSeek(canvasId){";
  s += "const c=document.getElementById(canvasId);const audio=document.getElementById('sessionAudio');if(!c)return;";
  s += "c.addEventListener('mousemove',(e)=>{const rect=c.getBoundingClientRect();const ratio=Math.max(0,Math.min(1,(e.clientX-rect.left)/rect.width));reviewState.hoverIndex=ratioToIndex(ratio);redrawReviewCharts();});";
  s += "c.addEventListener('mouseleave',()=>{reviewState.hoverIndex=null;redrawReviewCharts();});";
  s += "c.addEventListener('click',(e)=>{if(!audio||!audio.duration||!Number.isFinite(audio.duration)||audio.duration<=0)return;const rect=c.getBoundingClientRect();const ratio=Math.max(0,Math.min(1,(e.clientX-rect.left)/rect.width));audio.currentTime=ratio*audio.duration;reviewState.cursorRatio=ratio;redrawReviewCharts();});";
  s += "}";

  s += "async function loadCsv(){";
  s += "const r=await fetch('/api/file?name=" + csvName + "');";
  s += "const txt=await r.text();";
  s += "const lines=txt.trim().split(/\\r?\\n/);";
  s += "if(lines.length<2) return;";
  s += "const hasTrust=lines[0]&&lines[0].indexOf('trusted')>=0;";
  s += "reviewState.sampleCount=0;";

  s += "for(let i=1;i<lines.length;i++){";
  s += "const p=lines[i].split(',');";
  s += "if(hasTrust){if(p.length<15)continue;const trusted=parseInt(p[1]||0,10);reviewState.ms.push(parseFloat(p[0]||0));reviewState.trusted.push(trusted);reviewState.vib.x.push(parseFloat(p[4]||0));reviewState.vib.y.push(parseFloat(p[5]||0));reviewState.vib.z.push(parseFloat(p[6]||0));reviewState.vib.t.push(parseFloat(p[7]||0));reviewState.temp.obj.push(parseFloat(p[8]||0));reviewState.temp.ref.push(parseFloat(p[9]||0));reviewState.temp.amb.push(parseFloat(p[10]||0));reviewState.temp.dt.push(parseFloat(p[11]||0));reviewState.sound.db.push(parseFloat(p[13]||0));reviewState.sound.hz.push(parseFloat(p[14]||0));}";
  s += "else{if(p.length<12)continue;reviewState.ms.push(parseFloat(p[0]||0));reviewState.trusted.push(1);reviewState.vib.x.push(parseFloat(p[1]||0));reviewState.vib.y.push(parseFloat(p[2]||0));reviewState.vib.z.push(parseFloat(p[3]||0));reviewState.vib.t.push(parseFloat(p[4]||0));reviewState.temp.obj.push(parseFloat(p[5]||0));reviewState.temp.ref.push(parseFloat(p[6]||0));reviewState.temp.amb.push(parseFloat(p[7]||0));reviewState.temp.dt.push(parseFloat(p[8]||0));reviewState.sound.db.push(parseFloat(p[10]||0));reviewState.sound.hz.push(parseFloat(p[11]||0));}";
  s += "reviewState.sampleCount++;";
  s += "}";

  s += "const trustedIdx=[];for(let i=0;i<reviewState.trusted.length;i++){if(reviewState.trusted[i])trustedIdx.push(i);}";
  s += "const firstTrustedIndex=trustedIdx.length?trustedIdx[0]:null;";
  s += "const lastTrustedIndex=trustedIdx.length?trustedIdx[trustedIdx.length-1]:null;";
  s += "const validStartSec=firstTrustedIndex!==null?(reviewState.ms[firstTrustedIndex]/1000.0):0;";
  s += "const validEndSec=lastTrustedIndex!==null?(reviewState.ms[lastTrustedIndex]/1000.0):0;";
  s += "const validDurationSec=Math.max(0,validEndSec-validStartSec);";
  s += "const trustedRatio=reviewState.sampleCount>0?(100.0*trustedIdx.length/reviewState.sampleCount):0;";
  s += "function pickTrusted(arr){if(!trustedIdx.length)return hasTrust?[]:arr;return trustedIdx.map(i=>arr[i]);}";
  s += "const vibXTrusted=pickTrusted(reviewState.vib.x),vibYTrusted=pickTrusted(reviewState.vib.y),vibZTrusted=pickTrusted(reviewState.vib.z),vibTTrusted=pickTrusted(reviewState.vib.t);";
  s += "const tempDTTrusted=pickTrusted(reviewState.temp.dt),soundDbTrusted=pickTrusted(reviewState.sound.db),soundHzTrusted=pickTrusted(reviewState.sound.hz);";
  s += "const ax=vibXTrusted.reduce((m,v)=>Math.max(m,Math.abs(v)),0);";
  s += "const ay=vibYTrusted.reduce((m,v)=>Math.max(m,Math.abs(v)),0);";
  s += "const az=vibZTrusted.reduce((m,v)=>Math.max(m,Math.abs(v)),0);";
  s += "let axis='X';if(ay>=ax&&ay>=az)axis='Y';else if(az>=ax&&az>=ay)axis='Z';";
  s += "const absV=vibTTrusted.map(v=>Math.abs(v));";
  s += "const maxV=maxv(absV),avgV=avg(absV);";
  s += "const maxDT=maxv(tempDTTrusted),avgDT=avg(tempDTTrusted);";
  s += "const maxDB=maxv(soundDbTrusted),avgDB=avg(soundDbTrusted),maxHZ=maxv(soundHzTrusted);";
  s += "let vibAlert='Normal';if(maxV>=0.15)vibAlert='High';else if(maxV>=0.05)vibAlert='Watch';";
  s += "let tempAlert='Normal';if(maxDT>=15)tempAlert='High';else if(maxDT>=5)tempAlert='Watch';";
  s += "let soundAlert='Normal';if(maxDB>=55)soundAlert='High';else if(maxDB>=40)soundAlert='Watch';";
  s += "let overall='Normal';";
  s += "if(vibAlert==='High'||tempAlert==='High'||soundAlert==='High')overall='High';";
  s += "else if(vibAlert==='Watch'||tempAlert==='Watch'||soundAlert==='Watch')overall='Watch';";
  s += "if(trustedIdx.length<10){setText('rep_overall','Insufficient valid data');setText('rep_vib_alert','-');setText('rep_temp_alert','-');setText('rep_sound_alert','-');setText('rep_text','This session contains too little trusted mounted data for a reliable diagnostic interpretation.');setText('rep_focus','Repeat the recording after stable magnetic mounting and allow the device to reach monitoring state.');}";
  s += "else{setText('rep_overall',overall);setText('rep_vib_alert',vibAlert);setText('rep_temp_alert',tempAlert);setText('rep_sound_alert',soundAlert);setText('rep_text',makeInterpretation(vibAlert,tempAlert,soundAlert,axis));setText('rep_focus',makeFocus(vibAlert,tempAlert,soundAlert,axis));}";
  s += "setText('sum_max_vib',maxV.toFixed(5)+' in/s');";
  s += "setText('sum_avg_vib',avgV.toFixed(5)+' in/s');";
  s += "setText('sum_axis',axis);";
  s += "setText('sum_max_dt',maxDT.toFixed(2)+' F');";
  s += "setText('sum_avg_dt',avgDT.toFixed(2)+' F');";
  s += "setText('sum_samples',reviewState.sampleCount.toString());";
  s += "setText('sum_trusted_samples',trustedIdx.length.toString());";
  s += "setText('sum_trusted_ratio',trustedRatio.toFixed(1)+'%');";
  s += "setText('sum_valid_start',firstTrustedIndex!==null?formatTime(validStartSec):'-');";
  s += "setText('sum_valid_end',lastTrustedIndex!==null?formatTime(validEndSec):'-');";
  s += "setText('sum_valid_duration',formatTime(validDurationSec));";
  s += "setText('sum_max_db',maxDB.toFixed(2));";
  s += "setText('sum_avg_db',avgDB.toFixed(2));";
  s += "setText('sum_max_hz',maxHZ.toFixed(1));";
  s += "redrawReviewCharts();";
  s += "bindAudioCursor();";
  s += "bindChartSeek('vibChart');";
  s += "bindChartSeek('tempChart');";
  s += "bindChartSeek('soundChart');";
  s += "}";

  s += "loadCsv();";
  s += "</script>";

  s += htmlFooter();
  server.send(200, "text/html", s);
}

void WebUI::handleFileRaw() {
  if (!server.hasArg("name")) {
    server.send(400, "text/plain", "Missing file name");
    return;
  }

  String name = server.arg("name");
  if (!name.startsWith("/")) name = "/" + name;

  File f = sd.openRead(name);
  if (!f) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.streamFile(f, contentTypeFromName(name));
  f.close();
}
