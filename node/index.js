"use strict";
var zmq = require('zmq'),
    config = require('../conf/node_conf');

console.dir(config);
//router = server
var $r = zmq.socket('router');
    $r.port = 'tcp://'+config.node.ip+':'+config.node.nodePort;
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
    	var recvData = JSON.parse(data.toString());
    	console.dir(recvData);
      //console.log($r.identity + ': received ' + envelope + ' - ' + data.toString());
    });
});

//dealer = client
var $d = zmq.socket('dealer');
$d.identity = 'client' + process.pid;
$d.connect('tcp://'+config.node.ip+':'+config.node.zmqPort);
console.log('connected!');
setInterval(function() {
//var value = Math.floor(Math.random()*100
 var value = Math.floor(Math.random()*100);
$d.send(value);
console.log($d.identity + ': asking ' + value);
}, 100);
$d.on('message', function(data) {
console.log($d.identity + ': received ' + data.toString());
});