var socket = io();
var ffxiApp = angular.module('ffxiApp', ['socket-io']);
ffxiApp.factory('$wSock', function (socketFactory) {
  var wSock = socketFactory();
  wSock.forward('error');
  return wSock;
});
ffxiApp.service('$global', ['$rootScope', '$wSock', function($rootScope, $wSock){
    var self = this;
    $wSock.on('ls:msg', function(data){
    	console.log('linkshell message.');
    	console.dir(data);
    })
    // this.items = {
    //     list: [
    //     ];
    // }
}]);