ffxiApp.controller('indexController', ['$scope', '$global', function ($scope, $global) {
  $scope.items = [];
  console.log('indexController ready!');
}]);

ffxiApp.controller('linkshellController', ['$scope', '$global', function ($scope, $global) {
  $scope.lsMessages = [];
  $scope.$on('ls:msg', function(event, data){
  	console.dir(event);
  	console.dir(data);
  	$scope.lsMessages.push(data);
  });
  console.log('linkshellController ready!');
}]);