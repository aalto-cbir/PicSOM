// -*- JavaScript -*-  $Id: picsom.js,v 1.22 2014/04/22 15:18:12 jorma Exp $
// 
// Copyright 1998-2014 PicSOM Development Group <picsom@ics.aalto.fi>
// Aalto University School of Science
// PO Box 15400, FI-00076 Aalto, FINLAND
// 

function write_log(msg) {
  // this doesn't relly work...

  //var aConsoleService = Components.classes["@mozilla.org/consoleservice;1"].
  //                    getService(Components.interfaces.nsIConsoleService);
  //aConsoleService.logStringMessage("picsom.js : "+msg);
  //Components.utils.reportError("picsom.js : "+msg);
  // dump("picsom.js : "+msg+"\n");
}

// function erfcallback(msg) {
//   msg=msg.replace(/</g,'&lt;');
//   msg=msg.replace(/>/g,'&gt;');
//   document.getElementById("xmlstuff").innerHTML=msg;
// }

function erfcallback_gotopage(msg) {
  if (window && window.window && window.window.location)
    window.window.location.href = msg;
}

function erfcallback_setaspect(msg) {
  var a = msg.split(" ");
  var img = window.content.document.getElementById('obj'+a[0]);
  write_log("erfcallback_setaspect("+msg+") img="+img);
  if (img) {
    var c = a[2]=="1" ? "red" : "white";
    write_log(" c="+c);
    img.style.borderColor = c;
  }
}

function erfcallback_changetext(msg) {
  var a = msg.split(" ");
  var txt = window.content.document.getElementById('txt'+a[0]);
  write_log("erfcallback_changetext("+msg+") txt="+txt+" \""+a[1]+"\"");
  if (txt) {
    txt.innerHTML = a[1];
    txt.style.color = "black";
  }
}

function toggle(el) {
  if ( el.style.display != 'none' ) {
    el.style.display = 'none';
  } else {
    el.style.display = '';
  }
}

function toggle_short_long(el) {
  if (el.innerHTML != 'LESS') {
    el.innerHTML = 'LESS';
  } else {
    el.innerHTML = 'MORE';
  } 
  el = el.parentNode.previousSibling; // used with <form><button></button></form>
  //el = el.previousSibling;      // used with <button></button> without <form></form>
  toggle(el.previousSibling);
  toggle(el);
}

function toggle_table(el) {
  if (el.innerHTML != 'hide') {
    el.innerHTML = 'hide';
  } else {
    el.innerHTML = 'show';
  } 
  el = el.parentNode.nextSibling; 
  toggle(el);
}

function toggle_annotations_on(el) {
  toggle(el);

  var ielems = document.getElementsByClassName("annotation");
  for (i in ielems) {      
    var el = ielems[i];
    toggle(el);
  }
}

function toggle_annotations_off(el) {
  var ielems = document.getElementsByClassName("noannotation");
  for (i in ielems) {      
    var el = ielems[i];
    toggle(el);
  }
}

function set_nplusimages() {
  set_count(count_objects('obj-picsom:plusobjectlist'), 'pluscount');
}

function set_nminusimages() {
  set_count(count_objects('obj-picsom:minusobjectlist'), 'minuscount');
}

function set_nzeroimages() {
  set_count(count_objects('obj-picsom:zeroobjectlist'), 'zerocount');
}

function set_nshowimages() {
  set_count(count_objects('obj-picsom:showobjectlist'), 'showcount');
}

function set_nquestionimages() {
  set_count(count_objects('obj-picsom:questionobjectlist'), 'questioncount');
}

function set_count(n, targetid) {
  var pc = document.getElementById(targetid);
  if (pc)
    pc.innerHTML = n;
}

function count_objects(elname) {
  var x=document.getElementsByName(elname);
  return x.length;
}

function showimages() {
  if (count_objects('obj-picsom:plusobjectlist') > 0 || 
      count_objects('obj-picsom:minusobjectlist') > 0 || 
      count_objects('obj-picsom:zeroobjectlist') > 0 || 
      count_objects('obj-picsom:showobjectlist') > 0 || 
      count_objects('obj-picsom:questionobjectlist') > 0) {
    var il = document.getElementById('objectlists');
    il.style.display = '';
    var fl = document.getElementById('featurelist');
    fl.style.display = 'none';
    //var al = document.getElementById('algorithmlist');
    //al.style.display = 'none';
  }
}

function set_all_checkboxes(el, val) {
  for (var i = 0; i < el.parentNode.elements.length; i++) {
    var e = el.parentNode.elements[i];
    if ((e.type == 'checkbox') &&
        (e.className == 'picsom:questionobjectlist')) {
      e.checked = val;
    }
  }
}

function select_all_checkboxes(el) {
  set_all_checkboxes(el, 1);
}

function unselect_all_checkboxes(el) {
  set_all_checkboxes(el, 0);
}

function set_features(flist) {
  var features = flist.split(',');
  var val = -1;
  for (var i = 0; i < features.length; i++) {
    var f = features[i];
    var fcb = document.getElementById(f);
    if (val < 0) {
      val = 1-fcb.checked;
    }
    fcb.checked = val; 
  }
}

function toggleInput(prefix) {
  var ielems = document.getElementsByTagName("input");
  for (i in ielems) {
    var el = ielems[i];
    var name = el.name;
    if (name && name.indexOf("paramsel::")==0) {
      var fpref = "paramsel::"+prefix;
      if (name.indexOf(fpref+":")==0 || name==fpref) {
        el.disabled = false;
      } else {
        el.disabled = true;
      }
    }
  }
}

/*
  Returns timestamp giving the current time.
*/
function timeNow() {
  var d = new Date();
  var timeGMT = d.toGMTString(); 
  var milliSec = d.getMilliseconds();
  // append three-digit millisecond after second 
  if (milliSec<100 && milliSec>=10) {
      milliSec="0"+milliSec;
  } else if (milliSec<10) {
      milliSec="00"+milliSec; 
  }
  var time_Now = timeGMT.replace (/ GMT/,"."+milliSec+" GMT");
  return time_Now;
}

function sendajax_objecttag(object, tag) {
  var server = window.content.document.location.href.toString();
  var p = server.indexOf("/query/");
  if (p!=-1) {
      server = server.substr(0, p)+"/ajax/"+server.substr(p+7);
  }

  //window.content.document.getElementById("debugtag").innerHTML="["+server+"]";
  //alert(server);

  var doc = window.content.document.implementation.createDocument(null, "", null);
  var start = doc.createElement("stream.start"); 
  doc.appendChild(start);
 
  var piElem = doc.createProcessingInstruction("xml", 'version="1.0"');
  doc.insertBefore(piElem, start);

  var erf = doc.createElement("erf");
  start.appendChild(erf);
  
  erf.setAttribute("href",    window.content.document.location);
  erf.setAttribute("type",    "special");
  erf.setAttribute("scheme",  "pinview/1.0");
  erf.setAttribute("version",  "1.xx");

  var data = doc.createElement("data"); 
  start.appendChild(data);
  
  var objecttag = doc.createElement("objecttag");
  data.appendChild(objecttag);
  
  var timestamp = doc.createElement("timestamp");
  objecttag.appendChild(timestamp);
  var value = doc.createTextNode(timeNow());
  timestamp.appendChild(value);

  var objectel = doc.createElement("object");
  objectel.appendChild(doc.createTextNode(object));
  objecttag.appendChild(objectel);

  var tagel = doc.createElement("tag");
  tagel.appendChild(doc.createTextNode(tag));
  objecttag.appendChild(tagel);

  var data = (new XMLSerializer()).serializeToString(doc);

  //window.content.document.getElementById("debugtag").innerHTML="["+data+"]";
  //alert(data);

  try {
    var req = new XMLHttpRequest();
    req.overrideMimeType('text/xml');
    //var self = this;
    //req.onreadystatechange = function() 
    //  { self.processResponse(req, self.server[0]); }
    req.open('POST', server, true);
    req.setRequestHeader("Content-Type", "text/xml");
    req.setRequestHeader("Content-Length", data.length);
    //req.setRequestHeader("Connection", "close");
    req.send(data);
    //alert("OK");
    return req;
  }
  catch (e) {
      //alert("fail");
    return null;
  }
}

refresh_stopped = false;

function timed_refresh(ms) {
    if (!refresh_stopped) {
	timeout_id = setTimeout("location.reload(true);", ms);
    }
}

function stop_refreshing(el) {
    window.clearTimeout(timeout_id);
    refresh_stopped = true;
}
