function humanFileSize(bytes, si) {
  var thresh = si ? 1000 : 1024;
  if(Math.abs(bytes) < thresh) {
      return bytes + ' B';
  }
  var units = si
    ? ['kB','MB','GB','TB','PB','EB','ZB','YB']
    : ['KiB','MiB','GiB','TiB','PiB','EiB','ZiB','YiB'];
  var u = -1;
  do {
    bytes /= thresh;
    ++u;
  } while(Math.abs(bytes) >= thresh && u < units.length - 1);
  return bytes.toFixed(1)+' '+units[u];
}

(function poll(){
  setTimeout(function(){
    $.ajax({url: "/api/stats", success: function(data){
      //console.log(data);
      $('#stats_recv').text(humanFileSize(data.DATA_RECV, true));
      $('#stats_sent').text(humanFileSize(data.DATA_SENT, true));
      $('#stats_req').text(data.REQ_RECV);
      $('#stats_res').text(data.RES_SENT);
      $('#stats_conn').text(data.NO_CONN);
      $('#stats_active').text(data.ACTIVE_CONN);
      $('#stats_mem').text(humanFileSize(data.MEM_USAGE, true));

      poll();
    }, dataType: "json"});
  }, 1000);
})();
