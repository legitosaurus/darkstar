var connector = {};
var zmq = require('zmq'),
$r = zmq.socket('router');

connector.init = function(app){
    $r.port = 'tcp://'+app.nodeConfig.node.ip+':'+app.nodeConfig.node.nodePort;
    $r.identity = 'nodeServer'; // + process.pid;
$r.bind($r.port, function(err) {
    if (err) throw err;
    console.log('Listening on: '+$r.port);

    // setInterval(function() {
    //   var value = Math.floor(Math.random()*100);

    //   console.log($r.identity + ': asking ' + value);
    //   $r.send(value);
    // }, 100);


    $r.on('message', function(envelope, data) {
      //console.dir(data);
      var recvData = JSON.parse(data.toString());
      console.dir(recvData);
      app.io.emit('ls:msg', recvData);
      //console.log($r.identity + ': received ' + envelope + ' - ' + data.toString());
    });
});
var $d = zmq.socket('dealer');
$d.identity = 'client' + process.pid;
$d.connect('tcp://'+app.nodeConfig.node.ip+':'+app.nodeConfig.node.zmqPort);
console.log('connected!');
setInterval(function() {
	//var value = Math.floor(Math.random()*100
	 var value = ['linkshell', 'Foodz', 'dega'];
	//var value = "hello";
	$d.send(value);
	console.log($d.identity + ': asking ' + value);
}, 5000);
$d.on('message', function(data) {
	console.log($d.identity + ': received ' + data.toString());
});
  console.log('connector init');
}


module.exports = connector;