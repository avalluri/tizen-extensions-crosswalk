
var _listener = null;

function sendSyncMessage(msg) {
  return JSON.parse(extension.internal.sendSyncMessage(JSON.stringify(msg)));
};

var postMessage = function(msg) {
  console.log('Posting Message: ' + msg);
  extension.postMessage(JSON.stringify(msg));
};

extension.setMessageListener(function(json) {
  var msg = JSON.parse(json);
  if (_listener)
    _listener(msg['reply']);
});

exports.start = function(timeout, cb) {
 _listener = cb;
  postMessage({'request': 'start',
     'timeout': timeout
  });
};

exports.stop = function() {
  //_listener = null;
  postMessage({'request': 'stop'});
};

